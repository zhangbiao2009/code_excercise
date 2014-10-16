package main
import "fmt"
import "container/list"

func main(){
    l := list.New()
    l.PushBack("hi")
    l.PushBack(main)
    l.PushBack(2)
    for e:= l.Back(); e!=nil; e=e.Prev(){
        fmt.Printf("%v\n", e.Value)
    }
}
