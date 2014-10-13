package main
import "fmt"

/*
func uniq(s []int) map[int]int{
    m := make(map[int]int)
    for _,v := range s{
        m[v] = v
    }
    return m
}

*/

func main(){
    /*
    s := []int{4,5,6,3,5,4}
    m := uniq(s)
    for _,v := range m {
        fmt.Println(v)
    }
    */
    m := make(map[interface{}]int)
    m["h"] = 1
    m[2] = 5
    for k,v := range m{
        fmt.Println(k,v)
    }

}
