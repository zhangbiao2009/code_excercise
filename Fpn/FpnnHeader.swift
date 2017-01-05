//
//  FPNHeader.swift
//  MeMe
//
//  Created by LuanMa on 16/9/6.
//  Copyright © 2016年 sip. All rights reserved.
//

import Foundation

let FpnnMagic = "FPNN"

// flag constants
let FpnnFlagMsgpack: UInt8 = 0x80
let FpnnFlagJson: UInt8 = 0x40
let FpnnFlagZip: UInt8 = 0x20
let FpnnFlagEncrypt: UInt8 = 0x10

// package types
let FpnnPackMsgpack: UInt8 = 0
let FpnnPackJson: UInt8 = 1

// message types
let FpnnMtOneway: UInt8 = 0
let FpnnMtTwoway: UInt8 = 1
let FpnnMtAnswer: UInt8 = 2

let FpnnVersion: UInt8 = 1

struct FpnnHeader {
	static let Length = UInt(12)

	let magic: String
	let version: UInt8
	let flag: UInt8
	let mType: UInt8
	let ss: UInt8
	let pSize: UInt32

	init(magic: String, version: UInt8, flag: UInt8, mType: UInt8, ss: UInt8, pSize: UInt32) {
		self.magic = magic
		self.version = version
		self.flag = flag
		self.mType = mType
		self.ss = ss
		self.pSize = pSize
	}

	init(data: NSData) {
		self.magic = String(data: data.subdataWithRange(NSRange(location: 0, length: 4)), encoding: NSUTF8StringEncoding)!

		let lengthUInt8 = sizeof(UInt8)
		var valueUInt8 = UInt8(0)
		data.subdataWithRange(NSRange(location: 4, length: lengthUInt8)).getBytes(&valueUInt8, length: lengthUInt8)
		version = valueUInt8

		data.subdataWithRange(NSRange(location: 5, length: lengthUInt8)).getBytes(&valueUInt8, length: lengthUInt8)
		flag = valueUInt8

		data.subdataWithRange(NSRange(location: 6, length: lengthUInt8)).getBytes(&valueUInt8, length: lengthUInt8)
		mType = valueUInt8

		data.subdataWithRange(NSRange(location: 7, length: lengthUInt8)).getBytes(&valueUInt8, length: lengthUInt8)
		ss = valueUInt8

		let lengthUInt32 = sizeof(UInt32)
		var valueUInt32 = UInt32(0)
		data.subdataWithRange(NSRange(location: 8, length: lengthUInt32)).getBytes(&valueUInt32, length: lengthUInt32)
		pSize = valueUInt32
	}

	var pack: NSData {
		let data = NSMutableData()
		data.appendData(magic.dataUsingEncoding(NSUTF8StringEncoding)!)

		let lengthUInt8 = sizeof(UInt8)
		var valueUInt8: UInt8 = version
		data.appendBytes(&valueUInt8, length: lengthUInt8)

		valueUInt8 = flag
		data.appendBytes(&valueUInt8, length: lengthUInt8)

		valueUInt8 = mType
		data.appendBytes(&valueUInt8, length: lengthUInt8)

		valueUInt8 = ss
		data.appendBytes(&valueUInt8, length: lengthUInt8)

		let lengthUInt32 = sizeof(UInt32)
		var valueUInt32: UInt32 = pSize
		data.appendBytes(&valueUInt32, length: lengthUInt32)

		return data
	}
	
	var bodySize: UInt {
		return UInt(Int(pSize) + sizeof(UInt32))
	}
}

extension FpnnHeader: CustomStringConvertible {
	var description: String {
		return "FpnnHeader[magic=\(magic), version=\(version), flag=\(flag), mType=\(mType), ss=\(ss), pSize=\(pSize)]"
	}
}
