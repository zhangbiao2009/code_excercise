package main

import (
    "fmt"
)

func main(){
    /*
    arr1 := [...]int{1,2,3,4,5,6,7,8}
    arr2 := arr1[:5]
    arr2[0] = 3
    fmt.Printf("%d %d\n", arr1[0], arr2[0])
    */
    s0 := make([]int, 2, 10)
    s1 := append(s0, 2)
    s2 := append(s0, 3)
    fmt.Printf("%d %d\n", s1[2], s2[2])
    s0 = []int{0,1}
    s1 = append(s0, 2)
    s2 = append(s1, 3)
    fmt.Printf("%d %d\n", s1[2], s2[2])
}
