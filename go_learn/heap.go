package main
import (
	"fmt"
)

type Heap struct {
	elems []int
}

func NewHeap(s []int) *Heap {
	h := &Heap{s}
	for idx := len(h.elems)/2-1; idx >= 0; idx-- {
		h.down(idx)
	}
	return h
}

func (h *Heap) Push(e int) {
	h.elems = append(h.elems, e)
	h.up()
}

func (h *Heap) Pop() int {
	res := h.elems[0]
	last := len(h.elems) - 1
	h.elems[0], h.elems[last] = h.elems[last], h.elems[0]
	h.elems = h.elems[:len(h.elems)-1]
	h.down(0)
	return res
}

func (h *Heap) down(idx int) {
	for {
		left := 2*(idx+1) - 1
		right := 2 * (idx + 1)
		minIdx := idx
		if left < len(h.elems) && h.elems[minIdx] > h.elems[left] {
			minIdx = left
		}
		if right < len(h.elems) && h.elems[minIdx] > h.elems[right] {
			minIdx = right
		}
		if minIdx != idx {
			h.elems[minIdx], h.elems[idx] = h.elems[idx], h.elems[minIdx]
			idx = minIdx
		} else {
			return
		}
	}
}

func (h *Heap) up() {
	idx := len(h.elems) - 1
	for par := (idx - 1) / 2; par >= 0 && h.elems[idx] < h.elems[par]; idx = par {
		h.elems[idx], h.elems[par] = h.elems[par], h.elems[idx]
	}
}

func (h *Heap) Len() int {
	return len(h.elems)
}

func main() {
	h := NewHeap([]int{10, 20, 34, 7, 5, 4, 9})
	//h := NewHeap([]int{10, 20, 34, 7, 5, 4, 9, 19, 19, 14, 29, 16})
	h.Push(19)
	h.Push(19)
	h.Push(14)
	h.Push(29)
	h.Push(16)
	for h.Len() > 0 {
		fmt.Println(h.Pop())
	}
}
