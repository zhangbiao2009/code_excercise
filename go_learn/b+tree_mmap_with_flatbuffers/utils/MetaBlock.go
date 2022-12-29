// Code generated by the FlatBuffers compiler. DO NOT EDIT.

package utils

import (
	flatbuffers "github.com/google/flatbuffers/go"
)

type MetaBlock struct {
	_tab flatbuffers.Table
}

func GetRootAsMetaBlock(buf []byte, offset flatbuffers.UOffsetT) *MetaBlock {
	n := flatbuffers.GetUOffsetT(buf[offset:])
	x := &MetaBlock{}
	x.Init(buf, n+offset)
	return x
}

func GetSizePrefixedRootAsMetaBlock(buf []byte, offset flatbuffers.UOffsetT) *MetaBlock {
	n := flatbuffers.GetUOffsetT(buf[offset+flatbuffers.SizeUint32:])
	x := &MetaBlock{}
	x.Init(buf, n+offset+flatbuffers.SizeUint32)
	return x
}

func (rcv *MetaBlock) Init(buf []byte, i flatbuffers.UOffsetT) {
	rcv._tab.Bytes = buf
	rcv._tab.Pos = i
}

func (rcv *MetaBlock) Table() flatbuffers.Table {
	return rcv._tab
}

func (rcv *MetaBlock) Degree() *uint32 {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(4))
	if o != 0 {
		v := rcv._tab.GetUint32(o + rcv._tab.Pos)
		return &v
	}
	return nil
}

func (rcv *MetaBlock) MutateDegree(n uint32) bool {
	return rcv._tab.MutateUint32Slot(4, n)
}

func (rcv *MetaBlock) RootBlockId() *uint32 {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(6))
	if o != 0 {
		v := rcv._tab.GetUint32(o + rcv._tab.Pos)
		return &v
	}
	return nil
}

func (rcv *MetaBlock) MutateRootBlockId(n uint32) bool {
	return rcv._tab.MutateUint32Slot(6, n)
}

func (rcv *MetaBlock) Nkeys() *uint64 {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(8))
	if o != 0 {
		v := rcv._tab.GetUint64(o + rcv._tab.Pos)
		return &v
	}
	return nil
}

func (rcv *MetaBlock) MutateNkeys(n uint64) bool {
	return rcv._tab.MutateUint64Slot(8, n)
}

func (rcv *MetaBlock) NusedBlocks() *int32 {
	o := flatbuffers.UOffsetT(rcv._tab.Offset(10))
	if o != 0 {
		v := rcv._tab.GetInt32(o + rcv._tab.Pos)
		return &v
	}
	return nil
}

func (rcv *MetaBlock) MutateNusedBlocks(n int32) bool {
	return rcv._tab.MutateInt32Slot(10, n)
}

func MetaBlockStart(builder *flatbuffers.Builder) {
	builder.StartObject(4)
}
func MetaBlockAddDegree(builder *flatbuffers.Builder, degree uint32) {
	builder.PrependUint32(degree)
	builder.Slot(0)
}
func MetaBlockAddRootBlockId(builder *flatbuffers.Builder, rootBlockId uint32) {
	builder.PrependUint32(rootBlockId)
	builder.Slot(1)
}
func MetaBlockAddNkeys(builder *flatbuffers.Builder, nkeys uint64) {
	builder.PrependUint64(nkeys)
	builder.Slot(2)
}
func MetaBlockAddNusedBlocks(builder *flatbuffers.Builder, nusedBlocks int32) {
	builder.PrependInt32(nusedBlocks)
	builder.Slot(3)
}
func MetaBlockEnd(builder *flatbuffers.Builder) flatbuffers.UOffsetT {
	return builder.EndObject()
}