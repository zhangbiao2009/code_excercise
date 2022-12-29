package btreedb

import (
	"btreedb/utils"
	"bufio"
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"os"

	"github.com/edsrzf/mmap-go"
	flatbuffers "github.com/google/flatbuffers/go"
)

const (
	BLOCK_SIZE       = 4096
	BLOCK_MAGIC      = "BLKSTART"
	BLOCK_MAGIC_SIZE = len(BLOCK_MAGIC)
)

type Offset int64

type DB struct {
	metaBlock  *utils.MetaBlock
	maxKeySize int
	file       *os.File
	mmap       mmap.MMap
	nUsedBlock int
}

// 创建DB文件，并且格式化
func CreateDB(fileName string, maxKeySize int, degree int) (*DB, error) {
	defaultDBSize := 20 * 1024 * 1024                                       // 20M
	f, err := os.OpenFile(fileName, os.O_CREATE|os.O_TRUNC|os.O_RDWR, 0644) // 打开的同时，如果file存在的话，O_TRUNC会清空file
	if err != nil {
		return nil, err
	}
	err = f.Truncate(int64(defaultDBSize)) // 为简化编程，预先分配好空间，不再扩容
	if err != nil {
		f.Close()
		return nil, err
	}
	mmap, err := mmap.Map(f, mmap.RDWR, 0)
	if err != nil {
		f.Close()
		return nil, err
	}
	db := &DB{
		maxKeySize: maxKeySize,
		file:       f,
		mmap:       mmap,
	}
	db.Init(degree)
	return db, nil
}

func OpenDB(filePath string) (*DB, error) {
	f, err := os.OpenFile("test.db", os.O_RDWR, 0644)
	if err != nil {
		return nil, err
	}
	mmap, err := mmap.Map(f, mmap.RDWR, 0)
	if err != nil {
		f.Close()
		return nil, err
	}
	db := &DB{
		file: f,
		mmap: mmap,
	}
	db.loadMetaBlock()
	return db, nil
}

// 初始化DB，构造meta block和root block
func (db *DB) Init(degree int) {
	db.initMetaBlock(degree)
	root := newLeafNode(db) // for btree root node
	_ = root
}

func (db *DB) Insert(key, val []byte) error {
	meta := db.metaBlock
	rootBlockId := int(*meta.RootBlockId())
	promotedKey, rightSibling, err := db.insert(rootBlockId, key, val)
	if err != nil {
		return err
	}
	if rightSibling == nil {
		return nil
	}
	newRoot := newInternalNode(db)
	newRoot.insertKeyInPos(0, promotedKey)
	newRoot.setNKeys(1)
	newRoot.setChildBlockId(0, rootBlockId)
	newRoot.setChildBlockId(1, rightSibling.blockId)
	meta.MutateRootBlockId(uint32(newRoot.blockId))
	return nil
}

func (db *DB) Find(key []byte) ([]byte, error) {
	return db.find(int(*db.metaBlock.RootBlockId()), key)
}

func (db *DB) PrintDotGraph(w io.Writer) {
	fmt.Fprintln(w, "digraph g {")
	fmt.Fprintln(w, "node [shape = record,height=.1];")
	db.printDotGraph(w, int(*db.metaBlock.RootBlockId()))
	fmt.Fprintln(w, "}")
}

func (db *DB) PrintDotGraph2(fileName string) {
	f, err := os.Create(fileName)
	if err != nil {
		log.Fatal(err)
	}
	bw := bufio.NewWriter(f)
	db.PrintDotGraph(bw)
	bw.Flush()
	f.Close()
}

func (db *DB) printDotGraph(w io.Writer, blockId int) {
	node := loadNode(db, blockId)
	if node.isLeaf() {
		node.printDotGraphLeaf(w)
		return
	}
	// print internal node
	fmt.Fprintf(w, "node%d [label = \"", node.blockId)
	fmt.Fprintf(w, "<f0> ") // for the ptr[0]
	for i := 0; i < node.nKeys(); i++ {
		fmt.Fprintf(w, "|<f%d> %v", 2*i+1, string(node.getKey(i))) // for key i
		fmt.Fprintf(w, "|<f%d> ", 2*(i+1))                         // for ptr i+1
	}
	fmt.Fprintln(w, "\"];")

	for i := 0; i <= node.nKeys(); i++ {
		childBlockId := node.getChildBlockId(i)
		fid := 2 * i
		fmt.Fprintf(w, "\"node%d\":f%d -> \"node%d\";\n", node.blockId, fid, childBlockId) // for ptr edges
		db.printDotGraph(w, childBlockId)
	}
}

func (node *Node) printDotGraphLeaf(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.blockId)
	fmt.Fprintf(w, "<f0> ") // for prev ptr
	for i := 0; i < node.nKeys(); i++ {
		fmt.Fprintf(w, "|<f%d> %v", 2*i+1, string(node.getKey(i))) // for the keys[i]
		//fmt.Fprintf(w, "|<f%d> val:%d", 2*i+1, node.vals[i])	// for the vals[i]
		fmt.Fprintf(w, "|<f%d> v", 2*i+2) // for the vals[i]
	}
	nextPtrFid := 2*node.nKeys() + 1
	fmt.Fprintf(w, "|<f%d> ", nextPtrFid) // for next ptr
	fmt.Fprintln(w, "\"];")
	// 打印这两个指针会导致图形layout不好看，没找到好办法前暂时不打印
	/*
		if node.prev != nil {
			fid := 2*node.prev.nkeys + 1
			fmt.Fprintf(w, "\"node%d\":f0 -> \"node%d\":f%d;\n", node.id, node.prev.getNodeId(), fid) // for prev ptr edge
		}
		if node.next != nil {
			fmt.Fprintf(w, "\"node%d\":f%d -> \"node%d\":f0;\n", node.id, nextPtrFid, node.next.getNodeId()) // for next ptr edge
		}
	*/
}
func (db *DB) find(blockId int, key []byte) ([]byte, error) {
	node := loadNode(db, blockId)
	if node.isLeaf() {
		val, err := node.findByKey(key)
		return val, err
	}
	i := 0
	for ; i < node.nKeys(); i++ {
		k := node.getKey(i)
		cmp := bytes.Compare(key, k)
		if cmp < 0 {
			break
		}
	}
	childBlockId := node.getChildBlockId(i)
	return db.find(childBlockId, key)
}

func (db *DB) insert(blockId int, key, val []byte) (promotedKey []byte, newSiblingNode *Node, err error) {
	node := loadNode(db, blockId)
	if node.isLeaf() {
		return node.insertKVInLeaf(key, val)
	}
	// internal node
	i := 0
	for ; i < node.nKeys(); i++ {
		k := node.getKey(i)
		cmp := bytes.Compare(key, k)
		if cmp < 0 {
			break
		}
	}

	childBlockId := node.getChildBlockId(i)
	childPromtedKey, newChild, err := db.insert(childBlockId, key, val)
	if err != nil {
		return nil, nil, err
	}
	if newChild == nil { // no new child to insert
		return nil, nil, nil
	}
	// assert newChild != nil
	for l := node.nKeys() - 1; l >= i; l-- {
		node.setKeyPtr(l+1, node.getKeyPtr(l))
		node.setChildBlockId(l+2, node.getChildBlockId(l+1))
	}
	node.insertKeyInPos(i, childPromtedKey) // TODO: 因为key是通过key指针数组操作的，这里想要新分配一个key值，现在setKey还是insertKey语义有点不清晰，后续整理
	node.setChildBlockId(i+1, newChild.blockId)
	node.setNKeys(node.nKeys() + 1)

	if node.needSplit() {
		deg := node.degree()
		rightSibling := newInternalNode(db)
		nLeft := deg / 2
		nRight := deg - nLeft - 1
		l := node.nKeys() - 1

		for r := nRight - 1; r >= 0; r-- {
			rightSibling.insertKeyInPos(r, node.getKey(l))
			rightSibling.setChildBlockId(r+1, node.getChildBlockId(l+1))
			node.clearKeyPtr(l)
			node.setChildBlockId(l+1, 0)
			l--
		}
		rightSibling.setChildBlockId(0, node.getChildBlockId(l+1)) // 最左边的ptr
		node.setChildBlockId(l+1, 0)

		pKey := node.getKey(l)
		node.clearKeyPtr(l) // 清空对应的key指针
		node.setNKeys(nLeft)
		rightSibling.setNKeys(nRight)
		return pKey, rightSibling, nil
	}
	return nil, nil, nil
}

func (node *Node) findByKey(key []byte) ([]byte, error) {
	i := 0
	for ; i < node.nKeys(); i++ {
		k := node.getKey(i)
		cmp := bytes.Compare(key, k)
		if cmp == 0 { // found
			val := node.getVal(i)
			return val, nil
		}
		if cmp < 0 {
			return nil, errors.New("not found")
		}
	}
	return nil, errors.New("not found")
}

func (node *Node) insertKVInLeaf(key, val []byte) (promotedKey []byte, newSiblingNode *Node, err error) {
	i := 0
	for ; i < node.nKeys(); i++ {
		k := node.getKey(i)
		cmp := bytes.Compare(key, k)
		if cmp == 0 { // key already exists, update value only
			//node.setVal(i, val)
			return
		}
		if cmp < 0 {
			break
		}
	}
	// i is the plact to insert
	for l := node.nKeys() - 1; l >= i; l-- {
		node.setKeyPtr(l+1, node.getKeyPtr(l))
		node.setValPtr(l+1, node.getValPtr(l))
	}
	node.insertKvInPos(i, key, val)
	node.setNKeys(node.nKeys() + 1)
	if node.needSplit() {
		db := node.db
		deg := node.degree()
		rightSibling := newLeafNode(db)
		nLeft := deg / 2
		nRight := deg - nLeft
		l := node.nKeys() - 1
		for r := nRight - 1; r >= 0; r-- {
			k := node.getKey(l)
			v := node.getVal(l)
			rightSibling.insertKvInPos(r, k, v)
			node.clearKeyPtr(l)
			node.clearValPtr(l)
			l--
		}
		node.setNKeys(nLeft)
		rightSibling.setNKeys(nRight)
		return rightSibling.getKey(0), rightSibling, nil
	}
	return nil, nil, nil
}

func (node *Node) insertKvInPos(i int, key, val []byte) {
	node.insertKeyInPos(i, key)
	node.insertValInPos(i, val)
}

func (node *Node) insertKeyInPos(i int, key []byte) {
	ununsedOffset := int(*node.UnusedMemOffset())
	newKeyOffset := ununsedOffset + node.blockId*BLOCK_SIZE
	size := putByteSlice(node.db.mmap[newKeyOffset:], key)
	node.setKeyPtr(i, newKeyOffset)
	node.MutateUnusedMemOffset(uint16(ununsedOffset + size))
}

func (node *Node) insertValInPos(i int, val []byte) {
	ununsedOffset := int(*node.UnusedMemOffset())
	newValOffset := ununsedOffset + node.blockId*BLOCK_SIZE
	size := putByteSlice(node.db.mmap[newValOffset:], val)
	node.setValPtr(i, newValOffset)
	node.MutateUnusedMemOffset(uint16(ununsedOffset + size))
}

func (node *Node) setVal(i int, val []byte) { // update val i
	// 可以先看val i原先所占的大小够不够，如果够就原地修改，如果不够就新分配空间写入，原来的val i所占的空间变为garbage
	// 为简化实现，直接append，原先空间作废
}

func (db *DB) Delete(key []byte) {
}

/*
meta block的格式:
8个字节的block start Magic，仅用来辅助其他工具查看DB文件，程序中没多大用
剩下的在flatbuffers MetaBlock table里
*/
func (db *DB) initMetaBlock(degree int) {
	_, start := db.newBlock() // for metablock, block 0
	rootBlockId := 1          // root node id 为1，虽然还没有构造

	builder := flatbuffers.NewBuilder(1024)
	utils.MetaBlockStart(builder)
	utils.MetaBlockAddNusedBlocks(builder, int32(db.nUsedBlock))
	utils.MetaBlockAddNkeys(builder, 0)
	utils.MetaBlockAddRootBlockId(builder, uint32(rootBlockId))
	utils.MetaBlockAddDegree(builder, uint32(degree))
	builder.Finish(utils.MetaBlockEnd(builder))
	buf := builder.FinishedBytes()
	copy(db.mmap[start:], buf) // 把构造好的metablock数据copy到mmap内存中

	db.metaBlock = utils.GetRootAsMetaBlock(db.mmap[start:], 0)
}

func (db *DB) loadMetaBlock() {
	start := BLOCK_MAGIC_SIZE // skip block start magic
	db.metaBlock = utils.GetRootAsMetaBlock(db.mmap[start:], 0)
	db.nUsedBlock = int(*db.metaBlock.NusedBlocks())
}

func (db *DB) newBlock() (blockId int, offset Offset) {
	blockId = db.nUsedBlock
	var start Offset = Offset(blockId) * BLOCK_SIZE
	copy(db.mmap[start:], mmap.MMap(BLOCK_MAGIC))
	start += Offset(BLOCK_MAGIC_SIZE)
	db.nUsedBlock++ // Meta block要占用一个block
	return blockId, start
}

func (db *DB) Close() {
	db.mmap.Unmap()
	db.file.Close()
}

func putByteSlice(b, s []byte) int {
	slen := uint64(len(s))
	nbytes := binary.PutUvarint(b, slen)
	copy(b[nbytes:], s)
	return nbytes + len(s)
}

func getByteSlice(b []byte) []byte {
	slen, nbytes := binary.Uvarint(b)
	resSlice := make([]byte, slen)
	copy(resSlice[:slen], b[nbytes:nbytes+int(slen)])
	return resSlice
}
