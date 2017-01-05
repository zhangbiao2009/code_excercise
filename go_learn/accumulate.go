package main

import(
	"fmt"
)

type Adder interface {
	add(Adder) Adder
}

func accumulate(s []Adder, init Adder) Adder{
	for _, v := range s {
		init = init.add(v)
	}
	return init
}



type Int int
func (t Int) add(u Adder) Adder {
	return t + u.(Int)
}


type String string
func(s String) add(u Adder) Adder {
	return s + u.(String)
}

func main() {

	/*
	s := make([]int, 4);
	for i:=0; i<len(s); i++{
		s[i] = 1;
	}

	adders := make([]Adder, len(s))
	for i, v := range s {
		adders[i] = Int(v);
	}
	*/
	adders := []Adder{Int(1), Int(2), Int(3)}

	//var init Int
	res := accumulate(adders, Int(0))
	fmt.Printf("%d\n", res);

	/*
	adders := []Adder{String("I "), String("say "), String("hello")}
	res := accumulate(adders, String(""))
	fmt.Printf("%s\n", res);
	*/
}
