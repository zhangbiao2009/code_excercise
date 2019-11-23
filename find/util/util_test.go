package util

import (
	"testing"
)

func TestFind(t *testing.T) {
	option := &NameOption{Pattern: "go"}
	files := Find("..", option)
	for _, f := range files {
		t.Log(f)
	}
}

func TestFind2(t *testing.T) {
	option := &SizeOption{
		Size: 100,
		Op:   ">",
	}
	option2 := &SizeOption{
		Size: 500,
		Op:   "<",
	}
	option3 := &NameOption{Pattern: "go"}

	files := Find("..", option, option2, option3)
	for _, f := range files {
		t.Log(f)
	}
}
