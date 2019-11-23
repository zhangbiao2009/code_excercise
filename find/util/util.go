package util

import (
	"io/ioutil"
	"os"
	"strings"
)

type Option interface {
	IsSatisfied(fileInfo os.FileInfo) bool
}

type NameOption struct {
	Pattern string
}

func (no *NameOption) IsSatisfied(fileInfo os.FileInfo) bool {
	return strings.Contains(fileInfo.Name(), no.Pattern)
}

type SizeType string

const (
	SizeOpEQ SizeType = "=="
	SizeOpNE SizeType = "!="
	SizeOpLT SizeType = "<"
	SizeOpLE SizeType = "<="
	SizeOpGT SizeType = ">"
	SizeOpGE SizeType = ">="
)

type SizeOption struct {
	Size int64
	Op   SizeType
}

func (so *SizeOption) IsSatisfied(fileInfo os.FileInfo) bool {
	fsz := fileInfo.Size()
	switch so.Op {
	case SizeOpEQ:
		return fsz == so.Size
	case SizeOpNE:
		return fsz != so.Size
	case SizeOpLE:
		return fsz <= so.Size
	case SizeOpLT:
		return fsz < so.Size
	case SizeOpGE:
		return fsz >= so.Size
	case SizeOpGT:
		return fsz > so.Size
	}

	return false
}

func find(dir string, results *[]string, options ...Option) {
	fileInfos, err := ioutil.ReadDir(dir)
	if err != nil {
		return
	}

	for _, fi := range fileInfos {
		if fi.IsDir() {
			find(dir+"/"+fi.Name(), results, options...)
		} else {
			satisfied := true
			for _, opt := range options {
				if !opt.IsSatisfied(fi) {
					satisfied = false
					break
				}
			}
			if satisfied {
				*results = append(*results, fi.Name())
			}
		}
	}
}

func Find(dir string, options ...Option) []string {
	var results []string
	find(dir, &results, options...)
	return results
}
