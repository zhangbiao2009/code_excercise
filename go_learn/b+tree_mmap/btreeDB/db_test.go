package btreedb

import (
	"bytes"
	"fmt"
	"log"
	"math/rand"
	"sort"
	"testing"
)

var dbFilePath string = "test.db"
var db *DB

func createDB(degree int) {
	maxKeySize := 128
	var err error
	db, err = CreateDB(dbFilePath, maxKeySize, degree)
	if err != nil {
		log.Fatal(err)
	}
}

func TestDBInsert(t *testing.T) {
	createDB(5)
	key := []byte("key1")
	val := []byte("val1")
	if err := db.Insert(key, val); err != nil {
		t.Error(err)
	}
	valRead, err := db.Find(key)
	if err != nil {
		t.Error(err)
	}
	if bytes.Compare(val, valRead) != 0 {
		t.Error("val not equal")
	}
}

func TestDBInsert2(t *testing.T) {
	db, err := OpenDB(dbFilePath)
	if err != nil {
		t.Fatal(err)
	}
	key := []byte("ssskey")
	val := []byte("sssval")
	if err := db.Insert(key, val); err != nil {
		t.Error(err)
	}
}

func TestDBInsertSplit(t *testing.T) {
	createDB(5)
	m := map[string]string{
		"key1": "val1",
		"key2": "val2",
		"key3": "val3",
		"key4": "val4",
		"key5": "val5",
	}
	for k, v := range m {
		key := []byte(k)
		val := []byte(v)
		if err := db.Insert(key, val); err != nil {
			t.Error(err)
		}
	}
	for k, v := range m {
		key := []byte(k)
		val := []byte(v)
		valRead, err := db.Find(key)
		if err != nil {
			t.Fatal(err)
		}

		if bytes.Compare(val, valRead) != 0 {
			t.Fatal("val not equal")
		}
	}
	db.PrintDotGraph2("mydb.dot")
	/*
	   valRead, err := db.Find(key)

	   	if err != nil {
	   		t.Error(err)
	   	}

	   	if bytes.Compare(val, valRead) != 0 {
	   		t.Error("val not equal")
	   	}
	*/
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
	createDB(5)
	intSlice := getUniqueInts(17)
	for _, num := range intSlice {
		key := fmt.Sprintf("k%d", num)
		val := fmt.Sprintf("v%d", num)
		db.Insert([]byte(key), []byte(val))
	}
	db.PrintDotGraph2("mydb.dot")
	for i := 0; i < 5; i++ {
		ri := rand.Intn(len(intSlice))
		randKey := fmt.Sprintf("k%d", intSlice[ri])
		expectVal := fmt.Sprintf("v%d", intSlice[ri]) 
		val, err := db.Find([]byte(randKey))
		if err != nil || bytes.Compare(val, []byte(expectVal)) != 0{
			t.Error("failed")
		}
	}
}

func TestDBFind(t *testing.T) {
	db, err := OpenDB(dbFilePath)
	if err != nil {
		t.Fatal(err)
	}

	key := []byte("ssskey")
	valRead, err := db.Find(key)
	if err != nil {
		t.Error(err)
	}
	t.Logf("%v", valRead)
}
