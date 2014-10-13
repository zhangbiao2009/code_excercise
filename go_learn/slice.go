package main

import (
    "fmt"
)

func printSlice(s []int){
    for i,v := range s{
        fmt.Printf("%d %d\n", i, v)
    }
    fmt.Println();
}
func main(){
    /*
    s0 := make([]int, 2, 10)
    s1 := append(s0, 2)
    s2 := append(s1, 3)

    for i:=0; i<len(s2); i++{
        fmt.Printf("%d\n", s2[i])
    }
    fmt.Printf("%d %d\n", s1[2], s2[2])
    s0 := []int{0,1}
    s1 := append(s0, 2)
    s2 := append(s0, 3)
    s3 := append(s1, s2...)
    printSlice(s3)
    */
    n:=0
    s := make([][]int, 3)
    for i := range s{
        s[i] = make([]int, 4)
        for j:= range s[i]{
            n++
            s[i][j] = n
        }
    }
    for i:=range s{
        fmt.Println(s[i])
    }
}
