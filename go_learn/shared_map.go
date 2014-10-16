package main
import "fmt"

type request struct {
    op int
    key string
    value string
    resChan chan string
}

func mapServ(m chan request){
    sharedMap := make(map[string] string)
    for {
        req := <-m
        switch req.op {
            case 1:{
                sharedMap[req.key] = req.value
                req.resChan <- "OK"
            }
            default:{
                req.resChan <- sharedMap[req.key]
            }
        }
    }
}

func set(m chan request, k, v string) string{
    resChan := make(chan string)
    m<-request{1, k, v, resChan}
    return <-resChan
}

func get(m chan request, k string) string{
    resChan := make(chan string)
    m<-request{2, k, "", resChan}
    return <-resChan
}

func main(){
    m := make(chan request)
    go mapServ(m)
    set(m, "k1", "v1")
    set(m, "k2", "v2")
    fmt.Println(get(m, "k1"))
    fmt.Println(get(m, "k2"))
}
