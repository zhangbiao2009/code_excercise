package main
import "fmt"

func main() {
    str := "Étoilé"
    for i := 0 ; i<len(str) ; i++ {
        fmt.Printf("%c", str[i])
    }

    fmt.Printf("\n")
    // Do this instead
    for _, c := range str {
        fmt.Printf("%c", c)
    }
    fmt.Printf("\n")
}
