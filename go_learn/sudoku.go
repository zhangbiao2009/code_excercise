package main

import "fmt"

var matrix [][]int
var nRes int

func init() {
	matrix = make([][]int, 9)
	for i := 0; i < 9; i++ {
		matrix[i] = make([]int, 9)
	}
}

func input() {
	//fmt.Println("please input:")
	for i := 0; i < 9; i++ {
		for j := 0; j < 9; j++ {
			fmt.Scanf("%d", &matrix[i][j])
		}
	}

}
func getNexElem(i, j int) (ni, nj int) {
	if j < 8 {
		return i, j + 1
	}
	return i + 1, 0
}
func printResult() {
	fmt.Println("result: ")
	for i := 0; i < 9; i++ {
		for j := 0; j < 9; j++ {
			fmt.Printf("%d ", matrix[i][j])
		}
		fmt.Println()
	}
	fmt.Println()
}

func isValid(i, j int) bool {
	// check row
	for k := 0; k < 9; k++ {
		if k != j {
			if matrix[i][j] == matrix[i][k] {
				return false
			}
		}
	}
	// check col
	for k := 0; k < 9; k++ {
		if k != i {
			if matrix[i][j] == matrix[k][j] {
				return false
			}
		}
	}

	si := i - i%3
	sj := j - j%3
	// check grid
	for i2 := si; i2 < si+3; i2++ {
		for j2 := sj; j2 < sj+3; j2++ {
			if i2 != i || j2 != j {
				if matrix[i][j] == matrix[i2][j2] {
					return false
				}
			}
		}
	}

	return true
}

func backtracking(i, j int) {
	if i == 9 {
		printResult()
		nRes++
		return
	}
	ni, nj := getNexElem(i, j)
	if matrix[i][j] != 0 {
		backtracking(ni, nj)
		return
	}
	for k := 1; k <= 9; k++ {
		matrix[i][j] = k
		if isValid(i, j) {
			backtracking(ni, nj)
		}
		matrix[i][j] = 0
	}
}

func main() {
	input()
	//printResult()
	backtracking(0, 0)
	//fmt.Println("number of result: ", nRes)
}
