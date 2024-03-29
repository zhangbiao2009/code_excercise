package btree

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os"

	"golang.org/x/exp/constraints"
)

var id int

type BTreeNode[K Key, V any] interface { // 为统一处理leaf node和internal node建立的抽象
	isLeafNode() bool
	insertKV(key K, val V) (keyPromoted K, newRightSibling BTreeNode[K, V])
	getLeftMostKey() K
	delete(key K)
	find(key K) (theNode *LeafNode[K, V], index int)
	findMinLeaf() *LeafNode[K, V]
	degree() int
	getNodeId() int
	getNKeys() int
	needSplit() bool
	stealFromRight(rightp BTreeNode[K, V], parentKey K) (midKey K)
	stealFromLeft(leftp BTreeNode[K, V], parentKey K) (midKey K)
	mergeWithRight(rightp BTreeNode[K, V], parentKey K)
	PrintDotGraph(w io.Writer)
}

type Key interface {
	constraints.Integer | ~string
}

type LeafNode[K Key, V any] struct {
	id    int
	prev  *LeafNode[K, V]
	next  *LeafNode[K, V]
	nkeys int
	keys  []K
	vals  []V
}

func NewLeafNode[K Key, V any](degree int) *LeafNode[K, V] {
	id++
	return &LeafNode[K, V]{
		id:   id,
		keys: make([]K, degree), // 多分配一个，split的时候可以先插入再split，简化算法
		vals: make([]V, degree),
	}
}

func (node *LeafNode[K, V]) isLeafNode() bool {
	return true
}

func (node *LeafNode[K, V]) degree() int {
	return len(node.keys)
}

func (node *LeafNode[K, V]) needSplit() bool {
	return node.nkeys == len(node.keys)
}

func (node *LeafNode[K, V]) getLeftMostKey() K {
	return node.keys[0]
}

func (node *LeafNode[K, V]) getNodeId() int {
	return node.id
}

func (node *LeafNode[K, V]) getNKeys() int {
	return node.nkeys
}

func (node *LeafNode[K, V]) findMinLeaf() *LeafNode[K, V] {
	return node
}

func (node *LeafNode[K, V]) print() {
	fmt.Printf("node id: %d\n", node.id)
	fmt.Println("keys")
	for i := 0; i < node.nkeys; i++ {
		fmt.Printf("%v ", node.keys[i])
	}
	fmt.Println("values")
	for i := 0; i < node.nkeys; i++ {
		fmt.Printf("%v ", node.vals[i])
	}
	fmt.Println()
}

func (node *LeafNode[K, V]) find(key K) (*LeafNode[K, V], int) {
	i := 0
	for ; i < node.nkeys; i++ {
		if key <= node.keys[i] {
			break
		}
	}
	// assert: i == node.nkeys || key <= node.keys[i]
	if i == node.nkeys {
		return nil, -1
	}
	// key <= node.keys[i]
	return node, i
}

// if split return right sibling else return null
func (node *LeafNode[K, V]) insertKV(key K, val V) (K, BTreeNode[K, V]) {
	deg := node.degree()
	i := 0
	for ; i < node.nkeys; i++ {
		if node.keys[i] == key { // key already exists, update value only
			node.vals[i] = val
			return *new(K), nil
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
		rightSibling := NewLeafNode[K, V](deg)
		nLeft := deg / 2
		nRight := deg - nLeft
		l := node.nkeys - 1
		for r := nRight - 1; r >= 0; r-- {
			rightSibling.keys[r] = node.keys[l]
			rightSibling.vals[r] = node.vals[l]
			node.keys[l] = *new(K)
			node.vals[l] = *new(V)
			l--
		}
		node.nkeys = nLeft
		rightSibling.nkeys = nRight
		rightSibling.prev = node
		rightSibling.next = node.next
		if node.next != nil {
			node.next.prev = rightSibling
		}
		node.next = rightSibling
		return rightSibling.getLeftMostKey(), rightSibling
	}
	return *new(K), nil
}

func (node *LeafNode[K, V]) delete(key K) {
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

func (node *LeafNode[K, V]) delKeyValByIndex(i int) {
	for j := i + 1; j < node.nkeys; j++ {
		node.keys[j-1] = node.keys[j]
		node.vals[j-1] = node.vals[j]
	}
	node.keys[node.nkeys-1] = *new(K) // default key
	node.vals[node.nkeys-1] = *new(V)
	node.nkeys--
}

func (node *LeafNode[K, V]) removeMinKeyVal() (key K, val V) {
	key = node.keys[0]
	val = node.vals[0]
	node.delKeyValByIndex(0)
	return
}

func (node *LeafNode[K, V]) removeMaxKeyVal() (key K, val V) {
	key = node.keys[node.nkeys-1]
	val = node.vals[node.nkeys-1]
	node.keys[node.nkeys-1] = *new(K)
	node.vals[node.nkeys-1] = *new(V)
	node.nkeys--
	return
}

func (node *LeafNode[K, V]) appendMaxKeyVal(key K, val V) {
	node.keys[node.nkeys] = key
	node.vals[node.nkeys] = val
	node.nkeys++
}

func (node *LeafNode[K, V]) stealFromLeft(leftp BTreeNode[K, V], parentKey K) (midKey K) {
	left := leftp.(*LeafNode[K, V])
	key, val := left.removeMaxKeyVal()
	node.insertKV(key, val) // TODO: 是不是专门写个函数会好一点，不调用insertKv；
	return key              // note: key is exactly the min key in curr
}

func (node *LeafNode[K, V]) stealFromRight(rightp BTreeNode[K, V], parentKey K) (midKey K) {
	right := rightp.(*LeafNode[K, V])
	key, val := right.removeMinKeyVal()
	node.appendMaxKeyVal(key, val)
	return right.getLeftMostKey()
}

func (node *LeafNode[K, V]) mergeWithRight(rightp BTreeNode[K, V], parentKey K) {
	//merge的时候，把在右边的key和val都copy过来，
	left := node
	right := rightp.(*LeafNode[K, V])
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

func (node *LeafNode[K, V]) PrintDotGraph(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.id)
	fmt.Fprintf(w, "<f0> ") // for prev ptr
	for i := 0; i < node.nkeys; i++ {
		fmt.Fprintf(w, "|<f%d> %v", 2*i+1, node.keys[i]) // for the keys[i]
		//fmt.Fprintf(w, "|<f%d> val:%d", 2*i+1, node.vals[i])	// for the vals[i]
		fmt.Fprintf(w, "|<f%d> v", 2*i+2) // for the vals[i]
	}
	nextPtrFid := 2*node.nkeys + 1
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

type InternalNode[K Key, V any] struct {
	id    int
	nkeys int
	keys  []K
	ptrs  []BTreeNode[K, V]
}

func NewInternalNode[K Key, V any](degree int) *InternalNode[K, V] {
	id++
	return &InternalNode[K, V]{
		id:   id,
		keys: make([]K, degree),
		ptrs: make([]BTreeNode[K, V], degree+1),
	}
}

func (node *InternalNode[K, V]) degree() int {
	return len(node.keys)
}
func (node *InternalNode[K, V]) getNKeys() int {
	return node.nkeys
}

func (node *InternalNode[K, V]) isLeafNode() bool {
	return false
}

func (node *InternalNode[K, V]) needSplit() bool {
	return node.nkeys == len(node.keys)
}

func (node *InternalNode[K, V]) getLeftMostKey() K {
	return node.keys[0]
}

func (node *InternalNode[K, V]) find(key K) (*LeafNode[K, V], int) {
	i := 0
	for ; i < node.nkeys; i++ {
		if key < node.keys[i] {
			break
		}
	}
	return node.ptrs[i].find(key)
}

func (node *InternalNode[K, V]) findMinLeaf() *LeafNode[K, V] {
	return node.ptrs[0].findMinLeaf()
}

func (node *InternalNode[K, V]) delete(key K) {
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
		if i+1 <= node.nkeys && node.ptrs[i+1].getNKeys() > minKeys {
			midKey := node.ptrs[i].stealFromRight(node.ptrs[i+1], node.keys[i])
			node.keys[i] = midKey
			return
		}
		// left sibling exists and can steal
		if i-1 >= 0 && node.ptrs[i-1].getNKeys() > minKeys {
			midKey := node.ptrs[i].stealFromLeft(node.ptrs[i-1], node.keys[i-1])
			node.keys[i-1] = midKey
			return
		}

		// if steal not possible, try to merge
		if i+1 <= node.nkeys { // right sibling exists
			node.ptrs[i].mergeWithRight(node.ptrs[i+1], node.keys[i])
			//因为right sibling不应该存在了，所以parent对应的key和ptr也删除；
			node.delKeyandPtrByIndex(i, i+1)
			return
		}

		if i-1 >= 0 { // left sibling exists
			// 合并到左边的sibling去
			node.ptrs[i-1].mergeWithRight(node.ptrs[i], node.keys[i-1])
			node.delKeyandPtrByIndex(i-1, i)
			return
		}
	}
}

func (node *InternalNode[K, V]) stealFromLeft(leftp BTreeNode[K, V], parentKey K) (midKey K) {
	/* 要拿到key来自于父节点，还要从left sibling那最大的一个指针过来，作为这边最小的指针，
	然后left sibling的最大的key变为父节点的key；
	*/
	left := leftp.(*InternalNode[K, V])
	key, ptr := left.removeMaxKeyAndPtr()
	node.appendMinKeyAndPtr(parentKey, ptr)
	return key
}

func (node *InternalNode[K, V]) stealFromRight(rightp BTreeNode[K, V], parentKey K) (midKey K) {
	/* 要拿到key来自于父节点，还要从right sibling那最小的一个指针过来，作为这边最大的指针，
	然后right sibling最小的key变为父节点的key；
	*/
	right := rightp.(*InternalNode[K, V])
	key, ptr := right.removeMinKeyAndPtr()
	node.appendMaxKeyAndPtr(parentKey, ptr)
	return key
}

func (node *InternalNode[K, V]) mergeWithRight(rightp BTreeNode[K, V], parentKey K) {
	/* 和叶子节点的merge不同，需要先把parent的key copy过来，
	然后copy right sibling的key和ptr；然后删除parent的key和它相邻的右边的指针
	*/
	left := node
	right := rightp.(*InternalNode[K, V])
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

func (node *InternalNode[K, V]) removeMinKeyAndPtr() (key K, ptr BTreeNode[K, V]) {
	key = node.keys[0]
	ptr = node.ptrs[0]
	node.delKeyandPtrByIndex(0, 0)
	return
}

func (node *InternalNode[K, V]) appendMaxKeyAndPtr(key K, ptr BTreeNode[K, V]) {
	node.keys[node.nkeys] = key
	node.ptrs[node.nkeys+1] = ptr
	node.nkeys++
}

func (node *InternalNode[K, V]) delKeyandPtrByIndex(keyStart, ptrStart int) {
	for j := keyStart + 1; j < node.nkeys; j++ {
		node.keys[j-1] = node.keys[j]
	}
	node.keys[node.nkeys-1] = *new(K)
	for j := ptrStart + 1; j <= node.nkeys; j++ {
		node.ptrs[j-1] = node.ptrs[j]
	}
	node.ptrs[node.nkeys] = nil
	node.nkeys--
}

func (node *InternalNode[K, V]) removeMaxKeyAndPtr() (key K, ptr BTreeNode[K, V]) {
	key = node.keys[node.nkeys-1]
	node.keys[node.nkeys-1] = *new(K)
	ptr = node.ptrs[node.nkeys]
	node.ptrs[node.nkeys] = nil
	node.nkeys--
	return
}

func (node *InternalNode[K, V]) appendMinKeyAndPtr(key K, ptr BTreeNode[K, V]) {
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

func (node *InternalNode[K, V]) insertKV(key K, val V) (K, BTreeNode[K, V]) {
	i := 0
	for ; i < node.nkeys; i++ {
		if key < node.keys[i] {
			break
		}
	}
	// assert: ptrs[i] is the place to go, note when i == node.nkeys, ptrs[i] is still valid
	promotedKey, newChild := node.ptrs[i].insertKV(key, val)
	if newChild == nil {
		return *new(K), nil
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
		rightSibling := NewInternalNode[K, V](deg)
		nLeft := deg / 2
		nRight := deg - nLeft - 1
		l := node.nkeys - 1

		for r := nRight - 1; r >= 0; r-- {
			rightSibling.keys[r] = node.keys[l]
			rightSibling.ptrs[r+1] = node.ptrs[l+1]
			node.keys[l] = *new(K)
			node.ptrs[l+1] = nil
			l--
		}
		rightSibling.ptrs[0] = node.ptrs[l+1] // 最左边的ptr
		node.ptrs[l+1] = nil

		pKey := node.keys[l]
		node.keys[l] = *new(K)
		node.nkeys = nLeft
		rightSibling.nkeys = nRight
		return pKey, rightSibling
	}

	return *new(K), nil
}

func (node *InternalNode[K, V]) PrintDotGraph(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.id)
	fmt.Fprintf(w, "<f0> ") // for the ptr[0]
	for i := 0; i < node.nkeys; i++ {
		fmt.Fprintf(w, "|<f%d> %v", 2*i+1, node.keys[i]) // for key i
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

func (node *InternalNode[K, V]) getNodeId() int {
	return node.id
}

type BTree[K Key, V any] struct {
	degree int
	root   BTreeNode[K, V]
}

func NewBTree[K Key, V any](degree int) *BTree[K, V] {
	return &BTree[K, V]{
		degree: degree,
		root:   NewLeafNode[K, V](degree),
	}
}

// return keys betwwen [*lowKey, *highKey)，如果为nil表示没有界限
func (tree *BTree[K, V]) FindRange(lowKey, highKey *K) *Iterator[K, V] {
	// 找到第一个包含 >= lowKey的leaf node，然后由Iterator提供顺序遍历功能
	var leafNode *LeafNode[K, V]
	var idx int
	if lowKey == nil {
		leafNode = tree.root.findMinLeaf()
		idx = 0
	} else {
		leafNode, idx = tree.root.find(*lowKey)
	}
	return &Iterator[K, V]{
		curr:    leafNode,
		currIdx: idx,
		lowKey:  lowKey,
		highKey: highKey,
	}
}

func (tree *BTree[K, V]) Find(key K) (V, bool) {
	leafNode, idx := tree.root.find(key)
	if leafNode != nil && leafNode.keys[idx] == key {
		return leafNode.vals[idx], true
	}
	return *new(V), false
}

func (bt *BTree[K, V]) Insert(key K, val V) {
	promotedKey, rightSibling := bt.root.insertKV(key, val)
	if rightSibling == nil {
		return
	}
	newRoot := NewInternalNode[K, V](bt.degree)
	newRoot.keys[0] = promotedKey
	newRoot.nkeys = 1
	newRoot.ptrs[0] = bt.root
	newRoot.ptrs[1] = rightSibling
	bt.root = newRoot
}

func (bt *BTree[K, V]) Delete(key K) {
	bt.root.delete(key)
	// 删除后，有可能root只剩下一个指针，没有key了，这时候需要去掉多余的root空节点
	if !bt.root.isLeafNode() {
		root := bt.root.(*InternalNode[K, V])
		if root.nkeys == 0 {
			bt.root = root.ptrs[0]
		}
	}
}

func (bt *BTree[K, V]) PrintDotGraph(w io.Writer) {
	fmt.Fprintln(w, "digraph g {")
	fmt.Fprintln(w, "node [shape = record,height=.1];")
	bt.root.PrintDotGraph(w)
	fmt.Fprintln(w, "}")
}

func (bt *BTree[K, V]) PrintDotGraph2(fileName string) {
	f, err := os.Create(fileName)
	if err != nil {
		log.Fatal(err)
	}
	bw := bufio.NewWriter(f)
	bt.PrintDotGraph(bw)
	bw.Flush()
	f.Close()
}

type Iterator[K Key, V any] struct {
	curr            *LeafNode[K, V]
	currIdx         int // index in LeafNode
	lowKey, highKey *K
}

func (it *Iterator[K, V]) hasNext() bool {
	if it.curr == nil {
		return false
	}
	if it.currIdx == it.curr.nkeys {
		it.curr = it.curr.next
		it.currIdx = 0
	}
	if it.curr == nil {
		return false
	}
	if it.highKey == nil {
		return true
	}
	return it.curr.keys[it.currIdx] < *it.highKey
}

func (it *Iterator[K, V]) next() (key K, val V) {
	key = it.curr.keys[it.currIdx]
	val = it.curr.vals[it.currIdx]
	it.currIdx++
	return
}
