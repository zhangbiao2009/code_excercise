package btree

import (
	"bufio"
	"fmt"
	"os"
	"testing"
)
func TestInsertLeafNodeNoSplit(t *testing.T) {
	curr := NewLeafNode(5)
	curr.insertKV(6, 6)
	curr.insertKV(3, 3)
	curr.insertKV(9, 9)
	curr.print()
}
func TestInsertLeafNodeSplit(t *testing.T) {
	curr := NewLeafNode(5)
	pkey, right := curr.insertKV(6, 6)
	pkey, right = curr.insertKV(3, 3)
	pkey, right = curr.insertKV(9, 9)
	pkey, right = curr.insertKV(8, 8)
	pkey, right = curr.insertKV(7, 7)
	_ = pkey
	curr.print()
	fmt.Println(right)

}

func TestInsertLeafNodeSplit2(t *testing.T) {
	curr := NewLeafNode(5)
	pkey, right := curr.insertKV(6, 6)
	pkey, right = curr.insertKV(3, 3)
	pkey, right = curr.insertKV(9, 9)
	pkey, right = curr.insertKV(8, 8)
	pkey, right = curr.insertKV(10, 10)
	_ = pkey
	curr.print()
	fmt.Println(right)
}

func TestInsertLeafNodeSplit3(t *testing.T) {
	curr := NewLeafNode(5)
	pkey, right := curr.insertKV(6, 6)
	pkey, right = curr.insertKV(3, 3)
	pkey, right = curr.insertKV(9, 9)
	pkey, right = curr.insertKV(8, 8)
	pkey, right = curr.insertKV(5, 5)
	_ = pkey
	curr.print()
	fmt.Println(right)
}

func TestBTreeInsert(t *testing.T) {
	bt := NewBTree(3)
	bt.Insert(6, 6)
	bt.Insert(3, 3)
	bt.Insert(9, 9)
	writer := bufio.NewWriter(os.Stdout)
	bt.PrintDotGraph(writer)
}