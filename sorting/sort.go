package main

import (
	"fmt"
	"sort"
)

func main() {
	vec := []int{3, 5, 2, 1}
	slice := sort.IntSlice(vec)
	sort.Sort(slice[0:])
	for _, val := range slice {
		fmt.Println(val)
	}
}
