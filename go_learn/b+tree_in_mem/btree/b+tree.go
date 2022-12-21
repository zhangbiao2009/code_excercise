package btree

import (
	"fmt"
	"io"
)

var id int

type BTreeNode interface {
	IsLeafNode() bool
	insertKV(key, val int) (keyPromoted int, newRightSibling BTreeNode)
	GetLeftMostKey() int
	delete(key int)
	degree() int
	getNodeId() int
	getNKeys() int
	needSplit() bool
	PrintDotGraph(w io.Writer)
}

type LeafNode struct {
	id    int
	prev  *LeafNode
	next  *LeafNode
	nkeys int
	keys  []int
	vals  []int
}

func NewLeafNode(degree int) *LeafNode {
	id++
	return &LeafNode{
		id:   id,
		keys: make([]int, degree), // 多分配一个，split的时候可以先插入再split，简化算法
		vals: make([]int, degree),
	}
}

func (node *LeafNode) IsLeafNode() bool {
	return true
}

func (node *LeafNode) degree() int {
	return len(node.keys)
}

func (node *LeafNode) needSplit() bool {
	return node.nkeys == len(node.keys)
}

func (node *LeafNode) GetLeftMostKey() int {
	return node.keys[0]
}

func (node *LeafNode) getNodeId() int {
	return node.id
}

func (node *LeafNode) getNKeys() int {
	return node.nkeys
}

func (node *LeafNode) print() {
	fmt.Printf("node id: %d\n", node.id)
	fmt.Println("keys")
	for i := 0; i < node.nkeys; i++ {
		fmt.Printf("%d ", node.keys[i])
	}
	fmt.Println("values")
	for i := 0; i < node.nkeys; i++ {
		fmt.Printf("%d ", node.vals[i])
	}
	fmt.Println()
}

// if split return right sibling else return null
func (node *LeafNode) insertKV(key, val int) (int, BTreeNode) {
	deg := node.degree()
	i := 0
	for ; i < node.nkeys; i++ {
		if node.keys[i] == key { // key already exists, update value only
			node.vals[i] = val
			return 0, nil
		}
		if key < node.keys[i] {
			break
		}
	}

	// i is the plact to insert
	for l := node.nkeys - 1; l >= i; l-- {
		node.keys[l+1] = node.keys[l]
		node.vals[l+1] = node.vals[l]
	}
	node.keys[i] = key
	node.vals[i] = val
	node.nkeys++
	if node.needSplit() {
		rightSibling := NewLeafNode(deg)
		nLeft := deg / 2
		nRight := deg - nLeft
		l := node.nkeys - 1
		for r := nRight - 1; r >= 0; r-- {
			rightSibling.keys[r] = node.keys[l]
			rightSibling.vals[r] = node.vals[l]
			node.keys[l] = 0
			node.vals[l] = 0
			l--
		}
		node.nkeys = nLeft
		rightSibling.nkeys = nRight
		rightSibling.prev = node
		node.next = rightSibling
		return rightSibling.GetLeftMostKey(), rightSibling
	}
	return 0, nil
}

func (node *LeafNode) delete(key int) {
	i := 0
	for ; i < node.nkeys; i++ {
		if key < node.keys[i] {
			return
		}
		if key == node.keys[i] {
			break
		}
	}
	if i == node.nkeys { // not found
		return
	}
	// assert key == node.keys[i], delete
	node.delKeyValByIndex(i)
}

func (node *LeafNode) delKeyValByIndex(i int) {
	for j := i + 1; j < node.nkeys; j++ {
		node.keys[j-1] = node.keys[j]
		node.vals[j-1] = node.vals[j]
	}
	node.keys[node.nkeys-1] = 0
	node.vals[node.nkeys-1] = 0
	node.nkeys--
}

func (node *LeafNode) removeMinKeyVal() (key, val int) {
	key = node.keys[0]
	val = node.vals[0]
	node.delKeyValByIndex(0)
	return
}

func (node *LeafNode) removeMaxKeyVal() (key, val int) {
	key = node.keys[node.nkeys-1]
	val = node.vals[node.nkeys-1]
	node.keys[node.nkeys-1] = 0
	node.vals[node.nkeys-1] = 0
	node.nkeys--
	return
}

func (node *LeafNode) appendMaxKeyVal(key, val int) {
	node.keys[node.nkeys] = key
	node.vals[node.nkeys] = val
	node.nkeys++
}

func (node *LeafNode) PrintDotGraph(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.id)
	fmt.Fprintf(w, "<f0> ") // for prev ptr
	for i := 0; i < node.nkeys; i++ {
		fmt.Fprintf(w, "|<f%d> %d", 2*i+1, node.keys[i]) // for the keys[i]
		//fmt.Fprintf(w, "|<f%d> val:%d", 2*i+1, node.vals[i])	// for the vals[i]
		fmt.Fprintf(w, "|<f%d> v", 2*i+2) // for the vals[i]
	}
	nextPtrFid := 2*node.nkeys + 1
	fmt.Fprintf(w, "|<f%d> ", nextPtrFid) // for next ptr
	fmt.Fprintln(w, "\"];")
	// 打印这两个指针会导致图形layout不好看，没找到好办法前暂时不打印
	/*
		if node.prev != nil {
			fid := 2*node.prev.nkeys+1
			fmt.Fprintf(w, "\"node%d\":f0 -> \"node%d\":f%d;\n", node.id, node.prev.getNodeId(), fid) // for prev ptr edge
		}
		if node.next != nil {
			fmt.Fprintf(w, "\"node%d\":f%d -> \"node%d\":f0;\n", node.id, nextPtrFid, node.next.getNodeId()) // for next ptr edge
		}
	*/
}

type InternalNode struct {
	id    int
	nkeys int
	keys  []int
	ptrs  []BTreeNode
}

func NewInternalNode(degree int) *InternalNode {
	id++
	return &InternalNode{
		id:   id,
		keys: make([]int, degree),
		ptrs: make([]BTreeNode, degree+1),
	}
}

func (node *InternalNode) degree() int {
	return len(node.keys)
}
func (node *InternalNode) getNKeys() int {
	return node.nkeys
}

func (node *InternalNode) IsLeafNode() bool {
	return false
}

func (node *InternalNode) needSplit() bool {
	return node.nkeys == len(node.keys)
}

func (node *InternalNode) GetLeftMostKey() int {
	return node.keys[0]
}

func (node *InternalNode) delete(key int) {
	i := 0
	for ; i < node.nkeys; i++ {
		if key < node.keys[i] {
			break
		}
	}
	cp := node.ptrs[i]
	cp.delete(key)

	minKeys := cp.degree() / 2
	if cp.getNKeys() < minKeys { // number of keys too small after deletion
		// try to steal from siblings
		// right sibling exists and can steal
		// TODO: 这些判断和逻辑有待合并
		if cp.IsLeafNode() {
			if i+1 <= node.nkeys && node.ptrs[i+1].getNKeys() > minKeys {
				midKey := stealFromRightLeaf(node.ptrs[i], node.ptrs[i+1])
				node.keys[i] = midKey
				return
			}
			// left sibling exists and can steal
			if i-1 >= 0 && node.ptrs[i-1].getNKeys() > minKeys {
				midKey := stealFromLeftLeaf(node.ptrs[i], node.ptrs[i-1])
				node.keys[i-1] = midKey
				return
			}
			if i+1 <= node.nkeys { // right sibling exists
				mergeWithRightLeaf(node.ptrs[i], node.ptrs[i+1])
				//因为right sibling不应该存在了，所以parent对应的key和ptr也删除；
				node.delKeyandPtrByIndex(i, i+1)
				return
			}

			if i-1 >= 0 { // left sibling exists
				// 合并到左边的sibling去
				mergeWithRightLeaf(node.ptrs[i-1], node.ptrs[i])
				node.delKeyandPtrByIndex(i-1, i)
			}
		} else { // 非叶子节点合并方式和叶子节点不同
			if i+1 <= node.nkeys && node.ptrs[i+1].getNKeys() > minKeys {
				midKey := stealFromRightInternal(node.ptrs[i], node.ptrs[i+1], node.keys[i])
				node.keys[i] = midKey
				return
			}
			// left sibling exists and can steal
			if i-1 >= 0 && node.ptrs[i-1].getNKeys() > minKeys {
				midKey := stealFromLeftInternal(node.ptrs[i], node.ptrs[i-1], node.keys[i-1])
				node.keys[i-1] = midKey
				return
			}

			if i+1 <= node.nkeys { // right sibling exists
				mergeWithRightInternal(node.ptrs[i], node.ptrs[i+1], node.keys[i])
				//因为right sibling不应该存在了，所以parent对应的key和ptr也删除；
				node.delKeyandPtrByIndex(i, i+1)
				return
			}

			if i-1 >= 0 { // left sibling exists
				// 合并到左边的sibling去
				mergeWithRightInternal(node.ptrs[i-1], node.ptrs[i], node.keys[i-1])
				node.delKeyandPtrByIndex(i-1, i)
			}
		}
	}
}

func stealFromRightLeaf(currp, rightp BTreeNode) (midKey int) {
	curr := currp.(*LeafNode)
	right := rightp.(*LeafNode)
	key, val := right.removeMinKeyVal()
	curr.appendMaxKeyVal(key, val)
	return right.GetLeftMostKey()
}

func stealFromRightInternal(currp, rightp BTreeNode, parentKey int) (midKey int) {
	/* 要拿到key来自于父节点，还要从right sibling那最小的一个指针过来，作为这边最大的指针，
	然后right sibling最小的key变为父节点的key；
	*/
	curr := currp.(*InternalNode)
	right := rightp.(*InternalNode)
	key, ptr := right.removeMinKeyAndPtr()
	curr.appendMaxKeyAndPtr(parentKey, ptr)
	return key
}

func (node *InternalNode) removeMinKeyAndPtr() (key int, ptr BTreeNode) {
	key = node.keys[0]
	ptr = node.ptrs[0]
	node.delKeyandPtrByIndex(0, 0)
	return
}

func (node *InternalNode) appendMaxKeyAndPtr(key int, ptr BTreeNode) {
	node.keys[node.nkeys] = key
	node.ptrs[node.nkeys+1] = ptr
	node.nkeys++
}

func (node *InternalNode) delKeyandPtrByIndex(keyStart, ptrStart int) {
	for j := keyStart + 1; j < node.nkeys; j++ {
		node.keys[j-1] = node.keys[j]
	}
	node.keys[node.nkeys-1] = 0
	for j := ptrStart + 1; j <= node.nkeys; j++ {
		node.ptrs[j-1] = node.ptrs[j]
	}
	node.ptrs[node.nkeys] = nil
	node.nkeys--
}

func stealFromLeftLeaf(currp, leftp BTreeNode) (midKey int) {
	curr := currp.(*LeafNode)
	left := leftp.(*LeafNode)
	key, val := left.removeMaxKeyVal()
	curr.insertKV(key, val) // TODO: 是不是专门写个函数会好一点，不调用insertKv；
	return key              // note: key is exactly the min key in curr
}

func stealFromLeftInternal(currp, leftp BTreeNode, parentKey int) (midKey int) {
	/* 要拿到key来自于父节点，还要从left sibling那最大的一个指针过来，作为这边最小的指针，
	然后left sibling的最大的key变为父节点的key；
	*/
	curr := currp.(*InternalNode)
	left := leftp.(*InternalNode)
	key, ptr := left.removeMaxKeyAndPtr()
	curr.appendMinKeyAndPtr(parentKey, ptr)
	return key
}

func mergeWithRightLeaf(leftp, rightp BTreeNode) {
	//merge的时候，把在右边的key和val都copy过来，
	left := leftp.(*LeafNode)
	right := rightp.(*LeafNode)
	j := left.nkeys
	for i := 0; i < right.nkeys; i++ {
		left.keys[j] = right.keys[i]
		left.vals[j] = right.vals[i]
		j++
	}
	left.nkeys += right.nkeys
	left.next = right.next
	if right.next != nil {
		right.next.prev = left
	}
}

func mergeWithRightInternal(leftp, rightp BTreeNode, parentKey int) {
	/* 和叶子节点的merge不同，需要先把parent的key copy过来，
	然后copy right sibling的key和ptr；然后删除parent的key和它相邻的右边的指针
	*/
	left := leftp.(*InternalNode)
	right := rightp.(*InternalNode)
	left.keys[left.nkeys] = parentKey
	left.nkeys++
	j := left.nkeys
	for i := 0; i < right.nkeys; i++ {
		left.keys[j] = right.keys[i]
		left.ptrs[j] = right.ptrs[i]
		j++
	}
	left.ptrs[j] = right.ptrs[right.nkeys]
	left.nkeys += right.nkeys
}

func (node *InternalNode) removeMaxKeyAndPtr() (key int, ptr BTreeNode) {
	key = node.keys[node.nkeys-1]
	node.keys[node.nkeys-1] = 0
	ptr = node.ptrs[node.nkeys]
	node.ptrs[node.nkeys] = nil
	node.nkeys--
	return
}

func (node *InternalNode) appendMinKeyAndPtr(key int, ptr BTreeNode) {
	node.nkeys++
	for j := node.nkeys - 1; j > 0; j-- {
		node.keys[j] = node.keys[j-1]
	}
	for j := node.nkeys; j > 0; j-- {
		node.ptrs[j] = node.ptrs[j-1]
	}
	node.keys[0] = key
	node.ptrs[0] = ptr
}

func (node *InternalNode) insertKV(key, val int) (int, BTreeNode) {
	i := 0
	for ; i < node.nkeys; i++ {
		if key < node.keys[i] {
			break
		}
	}
	// assert: ptrs[i] is the place to go, note when i == node.nkeys, ptrs[i] is still valid
	promotedKey, newChild := node.ptrs[i].insertKV(key, val)
	if newChild == nil {
		return 0, nil
	}

	// assert newChild != nil
	for l := node.nkeys - 1; l >= i; l-- {
		node.keys[l+1] = node.keys[l]
		node.ptrs[l+2] = node.ptrs[l+1]
	}
	node.keys[i] = promotedKey
	node.ptrs[i+1] = newChild
	node.nkeys++

	if node.needSplit() {
		deg := node.degree()
		rightSibling := NewInternalNode(deg)
		nLeft := deg / 2
		nRight := deg - nLeft - 1
		l := node.nkeys - 1

		for r := nRight - 1; r >= 0; r-- {
			rightSibling.keys[r] = node.keys[l]
			rightSibling.ptrs[r+1] = node.ptrs[l+1]
			node.keys[l] = 0
			node.ptrs[l+1] = nil
			l--
		}
		rightSibling.ptrs[0] = node.ptrs[l+1] // 最左边的ptr
		node.ptrs[l+1] = nil

		pKey := node.keys[l]
		node.keys[l] = 0
		node.nkeys = nLeft
		rightSibling.nkeys = nRight
		return pKey, rightSibling
	}

	return 0, nil
}

func (node *InternalNode) PrintDotGraph(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.id)
	fmt.Fprintf(w, "<f0> ") // for the ptr[0]
	for i := 0; i < node.nkeys; i++ {
		fmt.Fprintf(w, "|<f%d> %d", 2*i+1, node.keys[i]) // for key i
		fmt.Fprintf(w, "|<f%d> ", 2*(i+1))               // for ptr i+1
	}
	fmt.Fprintln(w, "\"];")

	for i := 0; i <= node.nkeys; i++ {
		child := node.ptrs[i]
		fid := 2 * i
		fmt.Fprintf(w, "\"node%d\":f%d -> \"node%d\";\n", node.id, fid, child.getNodeId()) // for ptr edges
		child.PrintDotGraph(w)
	}
}

func (node *InternalNode) getNodeId() int {
	return node.id
}

type BTree struct {
	degree int
	root   BTreeNode
}

type Iterator struct {
}

func NewBTree(degree int) *BTree {
	return &BTree{
		degree: degree,
		root:   NewLeafNode(degree),
	}

}

func (tree *BTree) FindRange(lowKey, highKey int) []int {
	return nil
}

func (tree *BTree) Find(key int) bool {
	return false
}

func (bt *BTree) Insert(key, val int) {
	promotedKey, rightSibling := bt.root.insertKV(key, val)
	if rightSibling == nil {
		return
	}
	newRoot := NewInternalNode(bt.degree)
	newRoot.keys[0] = promotedKey
	newRoot.nkeys = 1
	newRoot.ptrs[0] = bt.root
	newRoot.ptrs[1] = rightSibling
	bt.root = newRoot
}

func (bt *BTree) Delete(key int) {
	bt.root.delete(key)
	// 删除后，有可能root只剩下一个指针，没有key了，这时候需要去掉多余的root空节点
	if !bt.root.IsLeafNode() {
		root := bt.root.(*InternalNode)
		if root.nkeys == 0 {
			bt.root = root.ptrs[0]
		}
	}
}

func (bt *BTree) PrintDotGraph(w io.Writer) {
	fmt.Fprintln(w, "digraph g {")
	fmt.Fprintln(w, "node [shape = record,height=.1];")
	bt.root.PrintDotGraph(w)
	fmt.Fprintln(w, "}")
}
