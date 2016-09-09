//
//  FPNQuest.swift
//  MeMe
//
//  Created by LuanMa on 16/9/6.
//  Copyright © 2016年 sip. All rights reserved.
//

import MPMessagePack

class FpnnQuest {
	let header: FpnnHeader
	var cTime: NSTimeInterval
	var seqNum: UInt32
	var method: String
	var payload: NSData

	init(method: String, params: [String: AnyObject]?, isOneway: Bool) {
		self.method = method
		cTime = NSDate.timeIntervalSinceReferenceDate()
		self.seqNum = 0
		if let params = params {
			let dict = params as NSDictionary
			payload = dict.mp_messagePack()
		} else {
			payload = NSDictionary().mp_messagePack()
		}

		let type = isOneway ? FpnnMtOneway : FpnnMtTwoway
		header = FpnnHeader(magic: FpnnMagic, version: FpnnVersion, flag: FpnnFlagMsgpack, mType: type, ss: UInt8(method.characters.count), pSize: UInt32(payload.length))
	}

	var pack: NSData {
		let data = NSMutableData()
		data.appendData(header.pack)
		if header.mType == FpnnMtTwoway {
			var value = seqNum
			data.appendBytes(&value, length: sizeof(UInt32))
		}
		data.appendData(method.dataUsingEncoding(NSUTF8StringEncoding)!)
		data.appendData(payload)
		return data
	}
}

extension FpnnQuest: CustomStringConvertible {
	var description: String {
		return "FpnnQuest[\(header), cTime=\(cTime), seqNum=\(seqNum), method=\(method)]"
	}
}
