package btreedb

import (
	"btreedb/utils"

	flatbuffers "github.com/google/flatbuffers/go"
)

type Node struct {
	db        *DB
	blockId   int
	metaBlock *utils.MetaBlock
	*utils.NodeBlock
}

func (node *Node) isLeaf() bool {
	return *node.IsLeaf()
}

func (node *Node) degree() int {
	return int(*node.metaBlock.Degree())
}
func (node *Node) nKeys() int {
	res := int(*node.NodeBlock.Nkeys())
	return res
}

func (node *Node) needSplit() bool {
	return node.nKeys() == node.degree()
}

func (node *Node) setNKeys(nKeys int) {
	node.MutateNkeys(uint16(nKeys))
}

func (node *Node) getKey(i int) []byte {
	keyOffset := node.getKeyPtr(i)
	return getByteSlice(node.db.mmap[keyOffset:])
}

func (node *Node) getVal(i int) []byte {
	valOffset := node.getValPtr(i)
	return getByteSlice(node.db.mmap[valOffset:])
}

func (node *Node) getKeyPtr(i int) int {
	return int(node.KeyPtrArr(i)) + node.blockId*BLOCK_SIZE
}

func (node *Node) setKeyPtr(i int, val int) {
	node.MutateKeyPtrArr(i, uint16(val-node.blockId*BLOCK_SIZE))
}
func (node *Node) clearKeyPtr(i int) {
	node.setKeyPtr(i, 0)
}

func (node *Node) getValPtr(i int) int {
	return int(node.ValPtrArr(i)) + node.blockId*BLOCK_SIZE
}

func (node *Node) setValPtr(i int, val int) {
	node.MutateValPtrArr(i, uint16(val-node.blockId*BLOCK_SIZE))
}

func (node *Node) clearValPtr(i int) {
	node.setValPtr(i, 0)
}

func (node *Node) getChildBlockId(i int) int {
	return int(node.ChildNodeId(i))
}

func (node *Node) setChildBlockId(i int, blockId int) {
	node.MutateChildNodeId(i, uint32(blockId))
}

// TODO: 考虑去掉db参数
func loadNode(db *DB, blockId int) *Node {
	start := Offset(blockId*BLOCK_SIZE + BLOCK_MAGIC_SIZE) // skip block magic
	nodeBlock := utils.GetRootAsNodeBlock(db.mmap[start:], 0)
	return &Node{
		db:        db,
		blockId:   blockId,
		metaBlock: db.metaBlock,
		NodeBlock: nodeBlock,
	}
}

func newLeafNode(db *DB) *Node {
	return newNode(db, true)
}
func newInternalNode(db *DB) *Node {
	return newNode(db, false)
}

func newNode(db *DB, isLeaf bool) *Node {
	blockId, start := db.newBlock()
	nodeBlock := newNodeBlock(db.mmap[start:], int(*db.metaBlock.Degree()), isLeaf)
	return &Node{
		db:        db,
		blockId:   blockId,
		metaBlock: db.metaBlock,
		NodeBlock: nodeBlock,
	}
}

func newNodeBlock(outputBuf []byte, degree int, isLeaf bool) *utils.NodeBlock {
	builder := flatbuffers.NewBuilder(1024)

	var childBlockIdArr flatbuffers.UOffsetT
	var valPtrArr flatbuffers.UOffsetT
	if isLeaf {
		utils.NodeBlockStartValPtrArrVector(builder, degree)
		for i := degree - 1; i >= 0; i-- {
			builder.PrependUint16(0)
		}
		valPtrArr = builder.EndVector(degree)
	} else {
		utils.NodeBlockStartChildNodeIdVector(builder, degree+1)
		for i := degree; i >= 0; i-- {
			builder.PrependUint32(0)
		}
		childBlockIdArr = builder.EndVector(degree)
	}

	utils.NodeBlockStartKeyPtrArrVector(builder, degree)
	for i := degree - 1; i >= 0; i-- {
		builder.PrependUint16(0)
	}
	keyPtrArr := builder.EndVector(degree)

	utils.NodeBlockStart(builder)
	utils.NodeBlockAddIsLeaf(builder, isLeaf)
	utils.NodeBlockAddPadding(builder, 0)
	utils.NodeBlockAddNkeys(builder, 0)
	utils.NodeBlockAddKeyPtrArr(builder, keyPtrArr)
	if isLeaf {
		utils.NodeBlockAddValPtrArr(builder, valPtrArr)
	} else {
		utils.NodeBlockAddChildNodeId(builder, childBlockIdArr)
	}
	utils.NodeBlockAddUnusedMemOffset(builder, 0)
	builder.Finish(utils.NodeBlockEnd(builder))
	bytes := builder.FinishedBytes()
	nBytesUsed := uint16(len(bytes))

	copy(outputBuf, bytes) // 构造好后，copy到mmap中

	nodeBlock := utils.GetRootAsNodeBlock(outputBuf, 0)
	nodeBlock.MutateUnusedMemOffset(uint16(BLOCK_MAGIC_SIZE) + nBytesUsed) // set unused mem offset
	return nodeBlock
}