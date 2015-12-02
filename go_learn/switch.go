package main

import "fmt"

func main() {
	var n int
	fmt.Scanf("%d", &n)
	switch {
	case n > 1:
		fmt.Println("n greater than 1");
	case n > 5:
		fmt.Println("n greater than 5");
	default:
		fmt.Println("default");
	}
}
