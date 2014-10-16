package main
import "fmt"

const (
    Get=iota
    Set
    BeginTransaction
    EndTransaction
)

type Request struct {
    requestType int
    key int
    value string
    ret chan string
    transaction chan Request
}

func mapServ(m chan Request){
    sharedMap := make(map[int] string)
    HandleRequests(sharedMap, m)
}

func HandleRequests(m map[int]string, c chan Request) {
    for {
        req := <-c
        switch req.requestType {
        case Get:
            req.ret <- m[req.key]
        case Set:
            m[req.key] = req.value
        case BeginTransaction:
            HandleRequests(m, req.transaction)
        case EndTransaction:
            return
        }
    }
}

func get(m chan Request, key int) string {
    result := make(chan string)
    m <- Request{Get, key, "", result, nil }
    return <- result
}

func set (m chan Request, key int, value string) {
    m <- Request{Set, key, value, nil, nil}
}

func beginTransaction( m chan Request) chan Request {
    t := make (chan Request)
    m <- Request{BeginTransaction, 0, "", nil, t}
    return t
}

func endTransaction( m chan Request) {
    m <- Request{EndTransaction, 0, "", nil, nil}
}

func main(){
    m := make(chan Request)
    go mapServ(m)
    set(m, 1, "v1")
    t := beginTransaction(m)
    v1 := get(t, 1)
    set(t, 2, v1+"v2")
    endTransaction(t)
    fmt.Println(get(m, 1))
    fmt.Println(get(m, 2))
}
