package main
import "fmt"
func main() {
    jobs := make(chan int)
    done := make(chan bool)
    go func() {
        for {
            j, more := <-jobs
            if more {
                fmt.Println("received job", j)
            } else {
                fmt.Println("received all jobs")
                done <- true
                return
            }
        }
    }()
    for j := 1; j <= 3; j++ {
        fmt.Println("send job", j)
        jobs <- j
    }
    close(jobs)
    fmt.Println("sent all jobs")
    <-done
}
