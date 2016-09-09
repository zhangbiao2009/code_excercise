//
//  FPNAnswer.swift
//  MeMe
//
//  Created by LuanMa on 16/9/6.
//  Copyright © 2016年 sip. All rights reserved.
//

import MPMessagePack

class FpnnAnswer {
	let header: FpnnHeader
	var cTime: NSTimeInterval
	var seqNum: UInt32
	var answerMap: [String: AnyObject]

	init(header: FpnnHeader, seqNum: UInt32, answerMap: [String: AnyObject]) {
		self.header = header
		self.seqNum = seqNum
		self.answerMap = answerMap
		cTime = NSDate.timeIntervalSinceReferenceDate()
	}

	init?(header: FpnnHeader, data: NSData) {
		do {
			let length = sizeof(UInt32)
			var seq = UInt32(0)
			data.getBytes(&seq, length: length)
			let subData = data.subdataWithRange(NSMakeRange(length, data.length - length))

			guard let map = try MPMessagePackReader.readData(subData) as? [String: AnyObject] else {
				return nil
			}
			self.header = header
			self.seqNum = seq
			self.answerMap = map
			cTime = NSDate.timeIntervalSinceReferenceDate()
		} catch {
			log.error("\(error)")
			return nil
		}
	}

	var isOk: Bool {
		return header.ss == 0
	}
	
	var packSize: Int {
		return Int(FpnnHeader.Length) + Int(header.bodySize)
	}

	func wantForKey(key: String) -> AnyObject? {
		return answerMap[key]
	}
}

extension FpnnAnswer: CustomStringConvertible {
	var description: String {
		return "FpnnAnswer[\(header), cTime=\(cTime), seqNum=\(seqNum), answerMap=\(answerMap)]"
	}
}