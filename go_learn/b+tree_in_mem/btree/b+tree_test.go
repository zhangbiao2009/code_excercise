package btree

import (
	"bufio"
	"fmt"
	"math/rand"
	"os"
	"sort"
	"testing"
)

func TestInsertLeafNodeNoSplit(t *testing.T) {
	curr := NewLeafNode[int](5)
	curr.insertKV(6, 6)
	curr.insertKV(3, 3)
	curr.insertKV(9, 9)
	curr.print()
}
func TestInsertLeafNodeSplit(t *testing.T) {
	curr := NewLeafNode[int](5)
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
	curr := NewLeafNode[int](5)
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
	curr := NewLeafNode[int](5)
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
	bt := NewBTree[int](3)
	bt.Insert(6, 6)
	bt.Insert(3, 3)
	bt.Insert(9, 9)
	writer := bufio.NewWriter(os.Stdout)
	bt.PrintDotGraph(writer)
}

func getUniqueInts(n int) []int {
	m := make(map[int]struct{})
	for {
		num := rand.Intn(100 * n)
		m[num] = struct{}{}
		if len(m) >= n {
			break
		}
	}
	res := make([]int, len(m))
	cnt := 0
	for num := range m {
		res[cnt] = num
		cnt++
	}
	sort.Ints(res)
	rand.Shuffle(len(res), func(i, j int) { res[i], res[j] = res[j], res[i] })
	return res
}

func TestBTreeFind(t *testing.T) {
	intSlice := getUniqueInts(17)
	bt := NewBTree[int](5)
	for _, num := range intSlice {
		bt.Insert(num, num*2)
	}
	for i := 0; i < 5; i++ {
		ri := rand.Intn(len(intSlice))
		randKey := intSlice[ri]
		val, ok := bt.Find(randKey)
		if !ok || val != randKey*2 {
			t.Error("failed")
		}
	}
}

func TestBTreeFindRange(t *testing.T) {
	intSlice := getUniqueInts(37)
	bt := NewBTree[int](5)
	for _, num := range intSlice {
		bt.Insert(num, num*2)
	}
	sort.Ints(intSlice)
	for i := 0; i < 5; i++ {
		ri := rand.Intn(len(intSlice))
		ri2 := rand.Intn(len(intSlice))
		if ri == ri2 {
			continue
		}
		if ri > ri2 {
			ri, ri2 = ri2, ri
		}
		lowKey := intSlice[ri]
		highKey := intSlice[ri2]

		it := bt.FindRange(&lowKey, &highKey)
		for it.hasNext() {
			key, val := it.next()
			if key != intSlice[ri] || val != 2*key {
				t.Error("failed")
				break
			}
			ri++
		}
		if ri != ri2 {
			t.Error("failed")
		}
	}
}

func TestBTreeFindRange2(t *testing.T) {
	intSlice := getUniqueInts(39)
	bt := NewBTree[int](5)
	for _, num := range intSlice {
		bt.Insert(num, num*2)
	}
	sort.Ints(intSlice)
	for i := 0; i < 5; i++ {
		ri := rand.Intn(len(intSlice))
		lowKey := intSlice[ri]
		nKeys := len(intSlice) - ri

		cnt := 0
		var laskKey *int = nil
		it := bt.FindRange(&lowKey, nil)
		for it.hasNext() {
			key, val := it.next()
			if key != intSlice[ri] || val != 2*key {
				t.Error("failed")
				break
			}
			if laskKey == nil {
				laskKey = &key
			} else {
				if key < *laskKey {
					t.Error("failed")
					break
				}
			}
			ri++
			cnt++
		}
		if cnt != nKeys {
			t.Error("failed")
		}
	}
}

func TestBTreeFindRange3(t *testing.T) {
	intSlice := getUniqueInts(39)
	bt := NewBTree[int](5)
	for _, num := range intSlice {
		bt.Insert(num, num*2)
	}
	sort.Ints(intSlice)
	for i := 0; i < 5; i++ {
		ri := rand.Intn(len(intSlice))
		highKey := intSlice[ri]
		nKeys := ri

		cnt := 0
		var laskKey *int = nil
		it := bt.FindRange(nil, &highKey)
		for it.hasNext() {
			key, val := it.next()
			if key != intSlice[cnt] || val != 2*key {
				t.Error("failed")
				break
			}
			if laskKey == nil {
				laskKey = &key
			} else {
				if key < *laskKey {
					t.Error("failed")
					break
				}
			}
			cnt++
		}
		if cnt != nKeys {
			t.Error("failed")
		}
	}
}

func TestBTreeFindRange4(t *testing.T) {
	intSlice := getUniqueInts(39)
	bt := NewBTree[int](5)
	for _, num := range intSlice {
		bt.Insert(num, num*2)
	}
	sort.Ints(intSlice)
	nKeys := len(intSlice)
	cnt := 0
	var laskKey *int = nil
	it := bt.FindRange(nil, nil)
	for it.hasNext() {
		key, val := it.next()
		if key != intSlice[cnt] || val != 2*key {
			t.Error("failed")
			break
		}
		if laskKey == nil {
			laskKey = &key
		} else {
			if key < *laskKey {
				t.Error("failed")
				break
			}
		}
		cnt++
	}
	if cnt != nKeys {
		t.Error("failed")
	}
}
