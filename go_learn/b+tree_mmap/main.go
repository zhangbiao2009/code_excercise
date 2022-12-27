package main

import (
	btreedb "btree/btreeDB"
	"bytes"
	"log"
)



func main() {
	/*
	maxKeySize := 128
	_, err := btreedb.CreateDB("test.db", maxKeySize)
	if err != nil {
		log.Fatal(err)
	}
	*/

	db, err := btreedb.OpenDB("test.db")
	if err != nil {
		log.Fatal(err)
	}
	_ = db
}