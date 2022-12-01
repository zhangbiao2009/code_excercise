package btree

import (
	"fmt"
	"io"
)

var id int
type BTreeNode interface{
	IsLeafNode() bool
	insertKV(key, val int) (keyPromoted int, newRightSibling BTreeNode)
	GetLeftMostKey() int
	getNodeId() int 
	needSplit() bool
	PrintDotGraph(w io.Writer) 
}

type LeafNode struct {
	id int
	prev *LeafNode
	next *LeafNode
	nkeys int
	keys []int
	vals []int
}

func NewLeafNode(degree int) *LeafNode {
	id++
	return &LeafNode{
		id : id,
		keys: make([]int, degree),		// 多分配一个，split的时候可以先插入再split，简化算法
		vals: make([]int, degree),
	}
}

func (node *LeafNode) IsLeafNode() bool {
	return true
}

func (node *LeafNode) degree() int{
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

func (node *LeafNode) print()  {
	fmt.Printf("node id: %d\n", node.id)
	fmt.Println("keys")
	for i:=0; i<node.nkeys; i++ {
		fmt.Printf("%d ", node.keys[i])
	}
	fmt.Println("values")
	for i:=0; i<node.nkeys; i++ {
		fmt.Printf("%d ", node.vals[i])
	}
	fmt.Println()
}

// if split return right sibling else return null
func (node *LeafNode) insertKV(key, val int) (int, BTreeNode){
	deg := node.degree()
	i:=0
	for ; i<node.nkeys; i++{
		if node.keys[i] == key {		// key already exists, update value only
			node.vals[i] = val
			return 0, nil
		}
		if key < node.keys[i] {
			break
		}
	}

	// i is the plact to insert
	
	for l := node.nkeys-1; l >= i; l-- {
		node.keys[l+1] = node.keys[l]
		node.vals[l+1] = node.vals[l]
	}
	node.keys[i] = key
	node.vals[i] = val 
	node.nkeys++
	if node.needSplit() {
		rightSibling := NewLeafNode(deg)
		nLeft := deg/2
		nRight := deg - nLeft
		l := node.nkeys-1
		for r:=nRight-1; r >= 0; r-- {
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

func (node *LeafNode) PrintDotGraph(w io.Writer) {
	fmt.Fprintf(w, "node%d [label = \"", node.id)
	fmt.Fprintf(w, "<f0> ")	// for prev ptr
	for i:=0; i<node.nkeys; i++ {
		fmt.Fprintf(w, "|<f%d> %d", 2*i+1, node.keys[i])	// for the keys[i]
		//fmt.Fprintf(w, "|<f%d> val:%d", 2*i+1, node.vals[i])	// for the vals[i]
		fmt.Fprintf(w, "|<f%d> v", 2*i+2)	// for the vals[i]
	}
	nextPtrFid := 2*node.nkeys+1
	fmt.Fprintf(w, "|<f%d> ", nextPtrFid)	// for next ptr
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
	id int
	nkeys int
	keys []int
	ptrs []BTreeNode
}

func NewInternalNode(degree int) *InternalNode {
	id++
	return &InternalNode {
		id:id,
		keys: make([]int, degree),
		ptrs: make([]BTreeNode, degree+1),
	}
}

func (node *InternalNode) degree() int{
	return len(node.keys)
}

func (node *InternalNode) IsLeafNode() bool {
	return true
}

func (node *InternalNode) needSplit() bool {
	return node.nkeys == len(node.keys)
}

func (node *InternalNode) GetLeftMostKey() int {
	return node.keys[0]
}


func (node *InternalNode) insertKV(key, val int) (int, BTreeNode) {
	i:=0
	for ; i<node.nkeys; i++{
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
	for l := node.nkeys-1; l >= i; l-- {
		node.keys[l+1] = node.keys[l]
		node.ptrs[l+2] = node.ptrs[l+1]
	}
	node.keys[i] = promotedKey
	node.ptrs[i+1] = newChild
	node.nkeys++

	if node.needSplit() {
		deg := node.degree()
		rightSibling := NewInternalNode(deg)
		nLeft := deg/2
		nRight := deg - nLeft - 1
		l := node.nkeys-1	

		for r:=nRight-1; r >= 0; r-- {
			rightSibling.keys[r] = node.keys[l]
			rightSibling.ptrs[r+1] = node.ptrs[l+1]
			node.keys[l] = 0
			node.ptrs[l+1] = nil
			l--
		}
		rightSibling.ptrs[0] = node.ptrs[l+1]	// 最左边的ptr
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
	fmt.Fprintf(w, "<f0> ")	// for the ptr[0]
	for i:=0; i<node.nkeys; i++ {
		fmt.Fprintf(w,	"|<f%d> %d", 2*i+1, node.keys[i])	// for key i
		fmt.Fprintf(w,	"|<f%d> ", 2*(i+1))	// for ptr i+1
	}
	fmt.Fprintln(w, "\"];")
	
	for i:=0; i<=node.nkeys; i++ {
		child := node.ptrs[i]
		fid := 2*i
		fmt.Fprintf(w, "\"node%d\":f%d -> \"node%d\";\n", node.id, fid, child.getNodeId()) // for ptr edges
		child.PrintDotGraph(w)
	}
}

func (node *InternalNode) getNodeId() int {
	return node.id
}

type BTree struct {
	degree int
	root BTreeNode
}

type Iterator struct {

}

func NewBTree(degree int) *BTree {
	return &BTree{
		degree: degree,
		root: NewLeafNode(degree),
	}

}

func (tree *BTree) FindRange(lowKey, highKey int) []int{
	return nil
}

func (tree *BTree) Find(key int) bool{
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

func (bt *BTree) PrintDotGraph(w io.Writer) {
	fmt.Fprintln(w, "digraph g {")
	fmt.Fprintln(w, "node [shape = record,height=.1];")
	bt.root.PrintDotGraph(w)
	fmt.Fprintln(w, "}")
}


func splitLeafNode() {

}

/*
func find(t BTreeNode, key int) *LeafNode {
	if(t.IsLeafNode())
		return t;

}
*/

func (tree *BTree) Delete(key int) {

}