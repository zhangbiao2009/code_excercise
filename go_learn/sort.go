package main
import "fmt"

func sort(s []int){
    for k:=0; k<len(s); k++{
        for i:=1; i<len(s)-k; i++ {
            if s[i] < s[i-1]{
                s[i-1],s[i] = s[i],s[i-1]
            }
        }
    }
}

func main(){
    arr := [...]int{5, 3, 7, 2}
    sort(arr[:])
    for i:=0; i<len(arr); i++ {
        fmt.Printf("%d\n", arr[i])
    }
}
