package main
import "fmt"

func loop(c,d chan int){
    for {
        i := <-c
        fmt.Println(i)
        i++
        c <- i      //如果不是有一个元素的buffer，此处会阻塞
    }
    d<-1
}
func main(){
    c:= make(chan int, 1)
    d:= make(chan int)
    go loop(c, d)
    c<-1
    <-d     // 使主线程等待
}
