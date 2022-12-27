package btreedb

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"os"

	"github.com/edsrzf/mmap-go"
)

const (
	BLOCK_SIZE       = 4096
	BLOCK_MAGIC      = "BLKSTART"
	BLOCK_MAGIC_SIZE = len(BLOCK_MAGIC)
)

type Offset int64

type DB struct {
	maxKeySize  int
	degree      int // btree node degree
	rootBlockId int
	nKeysInDB   int64
	file        *os.File
	mmap        mmap.MMap
	nUsedBlock  int // 当前用了多少block，分配新的block时需要
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
		degree:     degree,
		file:       f,
		mmap:       mmap,
	}
	db.Init()
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
	db.readMetaBlockInfo()
	return db, nil
}

// 初始化DB，构造meta block和root block
func (db *DB) Init() {
	db.nUsedBlock = 0
	db.rootBlockId = 1 // 初始时block id为1的是root block
	db.initMetaBlock()
}

func (db *DB) Insert(key, val []byte) error {
	promotedKey, rightSibling, err := db.insert(db.rootBlockId, key, val)
	if err != nil {
		return err
	}
	if rightSibling == nil {
		return nil
	}
	newRoot := db.newInternalNodeBlock()
	newRoot.insertKeyInPos(0, promotedKey)
	newRoot.nKeys = 1
	newRoot.setNKeys(uint16(newRoot.nKeys))
	newRoot.setChildBlockId(0, db.rootBlockId)
	newRoot.setChildBlockId(1, rightSibling.blockId)
	db.rootBlockId = newRoot.blockId
	rootBlockIdOffset := BLOCK_MAGIC_SIZE + 4
	putUint32(db.mmap[rootBlockIdOffset:], uint32(db.rootBlockId))
	return nil
}

func (db *DB) Find(key []byte) ([]byte, error) {
	return db.find(db.rootBlockId, key)
}

func (db *DB) PrintDotGraph(w io.Writer) {
	fmt.Fprintln(w, "digraph g {")
	fmt.Fprintln(w, "node [shape = record,height=.1];")
	db.printDotGraph(w, db.rootBlockId)
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

/*
leaf block:
8字节的block start magic
1字节，flag，是否是叶子节点
1字节，padding，作用待定
2字节，nkeys
2字节，unused_mem_offset
预先分配的key指针数组，每个key指针两个字节
预先分配的val指针数组，每个val指针两个字节
实际的key，value数据。。
*/
func (db *DB) newLeafNodeBlock() *Node {
	return db.newNodeBlock(true)
}
func (db *DB) newInternalNodeBlock() *Node {
	return db.newNodeBlock(false)
}
func (db *DB) newNodeBlock(isLeaf bool) *Node {
	blockId, start := db.newBlock()
	if isLeaf {
		db.mmap[start] = 1 // 是叶子节点
	}
	start++
	start++                       // padding字节
	putUint16(db.mmap[start:], 0) // 初始时nkeys是0
	start += 2

	var unusedMemOffset Offset
	// unusedMemOffset指向可分配使用的空闲空间，为节省空间，记录基于block首地址的偏移量，这样两个字节就够用了
	if isLeaf {
		unusedMemOffset = start + 2 + 4*Offset(db.degree) // 2是跳过它本身占用的空间，4*db.degree是跳过key和val指针数组
	} else {
		unusedMemOffset = start + 2 + 2*Offset(db.degree) + 4*Offset(db.degree+1) // 跳过key和ptr指针数组
	}
	unusedMemOffset -= Offset(blockId*BLOCK_SIZE) // 记录基于block首地址的偏移量，这样两个字节就够用了
	putUint16(db.mmap[start:], uint16(unusedMemOffset))
	// note: 实际的key val指针数组和key val值都是空的，不需要写入任何东西，ptr数组也一样
	node := &Node{
		db:           db,
		mmap:         db.mmap,
		degree:       db.degree,
		blockId:      blockId,
		isLeaf:       isLeaf,
		nKeys:        0,
		unusedOffset: uint16(unusedMemOffset),
	}
	return node
}

/*
internal node block:
8字节的block start magic
1字节，flag，是否是叶子节点
1字节，padding，作用待定
2字节，nkeys
2字节，unused_mem_offset
预先分配的key指针数组，每个key指针两个字节
预先分配的ptr指针数组，每个ptr存储一个子block id，4个字节
实际的key数据。。
*/

type Node struct {
	db           *DB
	mmap         mmap.MMap
	blockId      int
	degree       int
	isLeaf       bool
	nKeys        int
	unusedOffset uint16
	//note: key指针数组，val或ptr指针数组，实际的key val存储部分为了性能不反序列化，直接操作字节序列
}


func (db *DB) printDotGraph(w io.Writer, blockId int) {
	node := db.loadNode(blockId)
	if node.isLeaf {
		node.printDotGraphLeaf(w)
		return 
	} 	
	// print internal node
	fmt.Fprintf(w, "node%d [label = \"", node.blockId)
	fmt.Fprintf(w, "<f0> ") // for the ptr[0]
	for i := 0; i < node.nKeys; i++ {
		fmt.Fprintf(w, "|<f%d> %v", 2*i+1, string(node.getKey(i))) // for key i
		fmt.Fprintf(w, "|<f%d> ", 2*(i+1))               // for ptr i+1
	}
	fmt.Fprintln(w, "\"];")

	for i := 0; i <= node.nKeys; i++ {
		childBlockId := node.getChildBlockId(i)
		fid := 2 * i
		fmt.Fprintf(w, "\"node%d\":f%d -> \"node%d\";\n", node.blockId, fid, childBlockId) // for ptr edges
		db.printDotGraph(w, childBlockId)
	}
}

func (node *Node) printDotGraphLeaf(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.blockId)
	fmt.Fprintf(w, "<f0> ") // for prev ptr
	for i := 0; i < node.nKeys; i++ {
		fmt.Fprintf(w, "|<f%d> %v", 2*i+1, string(node.getKey(i))) // for the keys[i]
		//fmt.Fprintf(w, "|<f%d> val:%d", 2*i+1, node.vals[i])	// for the vals[i]
		fmt.Fprintf(w, "|<f%d> v", 2*i+2) // for the vals[i]
	}
	nextPtrFid := 2*node.nKeys + 1
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
	node := db.loadNode(blockId)
	if node.isLeaf {
		val, err := node.findByKey(key)
		return val, err
	} 	
	i := 0
	for ; i < node.nKeys; i++ {
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
	node := db.loadNode(blockId)
	if node.isLeaf {
		return node.insertKVInLeaf(key, val)
	}
	// internal node
	i := 0
	for ; i < node.nKeys; i++ {
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
	for l := node.nKeys - 1; l >= i; l-- {
		node.setKeyPtr(l+1, node.getKeyPtr(l))
		node.setChildBlockId(l+2, node.getChildBlockId(l+1))
	}
	node.insertKeyInPos(i, childPromtedKey) // TODO: 因为key是通过key指针数组操作的，这里想要新分配一个key值，现在setKey还是insertKey语义有点不清晰，后续整理
	node.setChildBlockId(i+1, newChild.blockId)
	node.nKeys++
	node.setNKeys(uint16(node.nKeys))

	if node.needSplit() {
		deg := node.degree
		rightSibling := db.newInternalNodeBlock()
		nLeft := deg / 2
		nRight := deg - nLeft - 1
		l := node.nKeys - 1

		for r := nRight - 1; r >= 0; r-- {
			rightSibling.insertKeyInPos(r, node.getKey(l))
			rightSibling.setChildBlockId(r+1, node.getChildBlockId(l+1))
			node.setKeyPtr(l, 0)
			node.setChildBlockId(l+1, 0)
			l--
		}
		rightSibling.setChildBlockId(0, node.getChildBlockId(l+1)) // 最左边的ptr
		node.setChildBlockId(l+1, 0)

		pKey := node.getKey(l)
		node.setKeyPtr(l, 0) // 清空对应的key指针
		node.nKeys = nLeft
		node.setNKeys(uint16(node.nKeys))
		rightSibling.nKeys = nRight
		rightSibling.setNKeys(uint16(rightSibling.nKeys))
		return pKey, rightSibling, nil
	}
	return nil, nil, nil
}

func (node *Node) needSplit() bool {
	return node.nKeys == node.degree
}

func (node *Node) findByKey(key []byte) ([]byte, error) {
	i := 0
	for ; i < node.nKeys; i++ {
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
	for ; i < node.nKeys; i++ {
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
	for l := node.nKeys - 1; l >= i; l-- {
		node.setKeyPtr(l+1, node.getKeyPtr(l))
		node.setValPtr(l+1, node.getValPtr(l))
	}
	node.insertKvInPos(i, key, val)
	node.nKeys++
	node.setNKeys(uint16(node.nKeys))
	if node.needSplit() {
		db := node.db
		deg := node.degree
		rightSibling := db.newLeafNodeBlock()
		nLeft := deg / 2
		nRight := deg - nLeft
		l := node.nKeys - 1
		for r := nRight - 1; r >= 0; r-- {
			k := node.getKey(l)
			v := node.getVal(l)
			rightSibling.insertKvInPos(r, k, v)
			node.setKeyPtr(l, 0)
			node.setValPtr(l, 0)
			l--
		}
		node.nKeys = nLeft
		node.setNKeys(uint16(node.nKeys))
		rightSibling.nKeys = nRight
		rightSibling.setNKeys(uint16(rightSibling.nKeys))
		return rightSibling.getKey(0), rightSibling, nil
	}
	return nil, nil, nil
}

func (node *Node) getKeyOffset(i int) int {
	keyPtrArray := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2 + 2
	KeyPtrIdxOffset := keyPtrArray + 2*i
	keyOffset := getUint16(node.mmap[KeyPtrIdxOffset:])
	return int(keyOffset)
}

func (node *Node) getValOffset(i int) int {
	keyPtrArray := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2 + 2
	valPtrArray := keyPtrArray + 2*node.degree
	valPtrIdxOffset := valPtrArray + 2*i
	valOffset := getUint16(node.mmap[valPtrIdxOffset:])
	return int(valOffset)
}
func (node *Node) getKeyPtrOffset(i int) int {
	/*
			8字节的block start magic
		1字节，flag，是否是叶子节点
		1字节，padding，作用待定
		2字节，nkeys
		2字节，unused_mem_offset
		预先分配的key指针数组，每个key指针两个字节
		预先分配的ptr指针数组，每个ptr存储一个子block id，4个字节
	*/
	keyPtrArray := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2 + 2
	return keyPtrArray + 2*i
}

func (node *Node) getValPtrOffset(i int) int {
	keyPtrArray := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2 + 2
	valPtrArray := keyPtrArray + 2*node.degree
	return valPtrArray + 2*i
}

func (node *Node) getChildBlockIdOffset(i int) int {
	keyPtrArray := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2 + 2
	childBlockIdArray := keyPtrArray + 2*node.degree
	return childBlockIdArray + 4*i
}

func (node *Node) getKey(i int) []byte {
	keyOffset := node.getKeyOffset(i)
	return getByteSlice(node.mmap[keyOffset:])
}

func (node *Node) getVal(i int) []byte {
	valOffset := node.getValOffset(i)
	return getByteSlice(node.mmap[valOffset:])
}

func (node *Node) getKeyPtr(i int) uint16 {
	keyPtrOffset := node.getKeyPtrOffset(i)
	return getUint16(node.mmap[keyPtrOffset:])
}

func (node *Node) setKeyPtr(i int, val uint16) {
	keyPtrOffset := node.getKeyPtrOffset(i)
	putUint16(node.mmap[keyPtrOffset:], val)
}
func (node *Node) getValPtr(i int) (offset uint16) {
	valPtrOffset := node.getValPtrOffset(i)
	return getUint16(node.mmap[valPtrOffset:])
}
func (node *Node) setValPtr(i int, val uint16) {
	valPtrOffset := node.getValPtrOffset(i)
	putUint16(node.mmap[valPtrOffset:], val)
}

func (node *Node) getChildBlockId(i int) int {
	offset := node.getChildBlockIdOffset(i)
	return int(getUint32(node.mmap[offset:]))
}

func (node *Node) setChildBlockId(i int, blockId int) {
	offset := node.getChildBlockIdOffset(i)
	putUint32(node.mmap[offset:], uint32(blockId))
}

func (node *Node) insertKvInPos(i int, key, val []byte) {
	node.insertKeyInPos(i, key)
	node.insertValInPos(i, val)
}

func (node *Node) insertKeyInPos(i int, key []byte) {
	newKeyOffset := node.unusedOffset + uint16(node.blockId)*BLOCK_SIZE
	size := putByteSlice(node.mmap[newKeyOffset:], key)
	node.setKeyPtr(i, newKeyOffset) // TODO: 写入了绝对地址！需要改成相对的
	node.unusedOffset += uint16(size)
	unuseOffsetAddr := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2
	putUint16(node.mmap[unuseOffsetAddr:], node.unusedOffset)
}

func (node *Node) insertValInPos(i int, val []byte) {
	newValOffset := node.unusedOffset + uint16(node.blockId)*BLOCK_SIZE
	size := putByteSlice(node.mmap[newValOffset:], val)
	node.setValPtr(i, uint16(newValOffset))		// TODO: 写入了绝对地址！需要改成相对的
	node.unusedOffset += uint16(size)
	unuseOffsetAddr := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1 + 2
	putUint16(node.mmap[unuseOffsetAddr:], node.unusedOffset)
}

func (node *Node) setNKeys(val uint16) {
	nkeysOffset := node.blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE + 1 + 1
	putUint16(node.mmap[nkeysOffset:], val)
}

func (node *Node) setVal(i int, val []byte) { // update val i
	// 可以先看val i原先所占的大小够不够，如果够就原地修改，如果不够就新分配空间写入，原来的val i所占的空间变为garbage
	// 为简化实现，直接append，原先空间作废
}

func (db *DB) loadNode(blockId int) *Node {
	/*
		8字节的block start magic
		1字节，flag，是否是叶子节点
		1字节，padding，作用待定
		2字节，nkeys
		2字节，unused_mem_offset
		预先分配的key指针数组，每个key指针两个字节
		预先分配的val指针数组，每个val指针两个字节
		实际的key，value数据。。
	*/
	start := Offset(blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE)
	var isLeaf bool
	if db.mmap[start] == 1 {
		isLeaf = true
	}
	start++
	start++ // skip padding
	nKeys := getUint16(db.mmap[start:])
	start += 2
	unusedMemOffset := getUint16(db.mmap[start:])

	node := &Node{
		db:           db,
		mmap:         db.mmap,
		degree:       db.degree,
		blockId:      blockId,
		isLeaf:       isLeaf,
		nKeys:        int(nKeys),
		unusedOffset: unusedMemOffset,
	}
	return node
}

func (db *DB) Delete(key []byte) {
}

/*
meta block的格式:
8个字节的block start Magic，仅用来辅助其他工具查看DB文件，程序中没多大用
## 16字节meta block的md5
4字节的btree degree
4个字节的btree root node的block id
8字节记录btree里key/val对的个数
4字节记录整个文件用掉了多少block
剩下的是padding。。。
*/
func (db *DB) initMetaBlock() {
	_, start := db.newBlock() // for metablock, block 0
	_ = db.newLeafNodeBlock() // for btree root node, block 1
	putUint32(db.mmap[start:], uint32(db.degree))
	start += 4
	putUint32(db.mmap[start:], uint32(db.rootBlockId))
	start += 4
	putUint64(db.mmap[start:], uint64(db.nKeysInDB)) // 当前key/val对的个数
	start += 8
	putUint32(db.mmap[start:], uint32(db.nUsedBlock)) // 当前用了多少个block
	start += 4
}

func (db *DB) readMetaBlockInfo() {
	start := 8 // skip block start magic
	db.degree = int(getUint32(db.mmap[start:]))
	start += 4
	db.rootBlockId = int(getUint32(db.mmap[start:]))
	start += 4
	db.nKeysInDB = int64(getUint64(db.mmap[start:]))
	start += 8
	db.nUsedBlock = int(getUint32(db.mmap[start:]))
	start += 4
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

func putUint64(b []byte, v uint64) {
	binary.BigEndian.PutUint64(b, v)
}

func putUint32(b []byte, v uint32) {
	binary.BigEndian.PutUint32(b, v)
}

func putUint16(b []byte, v uint16) {
	binary.BigEndian.PutUint16(b, v)
}

func getUint64(b []byte) uint64 {
	return binary.BigEndian.Uint64(b)
}

func getUint32(b []byte) uint32 {
	return binary.BigEndian.Uint32(b)
}

func getUint16(b []byte) uint16 {
	return binary.BigEndian.Uint16(b)
}
