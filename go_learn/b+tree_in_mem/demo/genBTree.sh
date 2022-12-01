#! /bin/sh
go run main.go > tmp.dot 
dot -Tpng -o btree.png tmp.dot
open btree.png
