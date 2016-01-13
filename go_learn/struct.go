package main

import "fmt"

type Resource interface {
	Consume()
}

type Gold struct {
	name string
}

func (this *Gold) Consume() {
	fmt.Printf("gold is consumed by %s\n", this.name);
}

type A struct {
	Resource
}

func main(){
	a := A{&Gold{"John"}}
	a.Consume()
}
