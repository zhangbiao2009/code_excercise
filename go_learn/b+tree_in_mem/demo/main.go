package main

import (
	"bufio"
	"math/rand"
	"os"

	"excercise.com/btree/btree"
)

func main() {
	bt := btree.NewBTree[int](5)
	fileNumber := 0
	m := make(map[int]struct{})
	for i := 0; i < 17; i++ {
		num := rand.Intn(1000)
		m[num] = struct{}{}
		bt.Insert(num, num)
		fileNumber++
		//name := fmt.Sprintf("btree%d.dot", fileNumber)
	}
	for num := range m {
		bt.Delete(num)
	}
	/*
		bt.Insert(6, 6)
		bt.Insert(3, 3)
		bt.Insert(19, 19)
		bt.Insert(7, 7)
		bt.Insert(10, 10)
		bt.Insert(5, 5)
		bt.Delete(5)
		bt.Delete(3)
	*/
	bw := bufio.NewWriter(os.Stdout)
	bt.PrintDotGraph(bw)
	bw.Flush()
}
