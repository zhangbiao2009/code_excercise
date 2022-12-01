package main

import (
	"bufio"
	"math/rand"
	"os"

	"excercise.com/btree/btree"
)

func main() {
	bt := btree.NewBTree(5)
	for i:=0; i<30; i++ {
		num := rand.Intn(1000)
		bt.Insert(num, num)
	}
	/*
	bt.Insert(6, 6)
	bt.Insert(3, 3)
	bt.Insert(19, 19)
	bt.Insert(7, 7)
	bt.Insert(10, 10)
	*/
	bw := bufio.NewWriter(os.Stdout)
	bt.PrintDotGraph(bw)
	bw.Flush()
}
