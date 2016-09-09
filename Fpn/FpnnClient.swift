//
//  FPNClient.swift
//  MeMe
//
//  Created by LuanMa on 16/9/6.
//  Copyright © 2016年 sip. All rights reserved.
//

import CocoaAsyncSocket
import Result

typealias FpnnAnswerCallback = (Result<FpnnAnswer, CAPError>) -> Void

private enum Tag: Int {
	case Header = 1
	case Body
}

private class AnswerTask {
	var quest: FpnnQuest
	var callback: FpnnAnswerCallback
	var cancelTask: CancelableTimeoutBlock

	init(quest: FpnnQuest, callback: FpnnAnswerCallback, cancelTask: CancelableTimeoutBlock) {
		self.quest = quest
		self.callback = callback
		self.cancelTask = cancelTask
	}
}

class FpnnClient: NSObject {
	var host: String?
	var port: UInt16?
	let readTimeout: NSTimeInterval

	private var answerTaskCache = [UInt32: AnswerTask]()

	private var sequenceNumber: UInt32 = 0
	private let sequenceLock = NSLock()

	private var socket: GCDAsyncSocket!
	private var fpnnQueue: dispatch_queue_t
	private var lastHeader: FpnnHeader?

	private var startConnectTime = NSDate()

	var working = false
	var connected: Bool {
		return socket.isConnected
	}

	init(timeout: NSTimeInterval) {
		readTimeout = timeout
		fpnnQueue = dispatch_queue_create("meme.fpnn.queue", DISPATCH_QUEUE_SERIAL)

		super.init()

		let queue = fpnnQueue
		socket = GCDAsyncSocket(delegate: self, delegateQueue: queue)
	}

	func connect(host host: String, port: UInt16) -> Bool {
		do {
			self.host = host
			self.port = port
			startConnectTime = NSDate()
			try socket.connectToHost(host, onPort: port)
			working = true
			return true
		} catch {
			log.error("\(error)")
			RumTracker.TraceFPNNConnectStatus("failed", error: error as NSError, connectStart: startConnectTime)
			return false
		}
	}

	private func connect() -> Bool {
		if let host = host, port = port {
			return connect(host: host, port: port)
		} else {
			return false
		}
	}

	func disconnect() {
		working = false
		socket.disconnect()

		dispatch_async(fpnnQueue) {
			for answerTask in self.answerTaskCache.values {
				answerTask.cancelTask.cancel()
			}
		}

		answerTaskCache.removeAll()
	}

	func send(quest: FpnnQuest, callback: FpnnAnswerCallback?) {
		dispatch_async(fpnnQueue) { [weak self] in
			self?.doSend(quest, callback: callback)
		}
	}

	private func recievedAnswer(answer: FpnnAnswer) {
		let seqNum = answer.seqNum
		if let answerStore = answerTaskCache.removeValueForKey(seqNum) {
			answerStore.cancelTask.cancel()
			main_async() {
				answerStore.callback(.Success(answer))
			}
		} else {
			log.info("received noresponse answer: \(answer)")
		}
	}

	private func doSend(quest: FpnnQuest, callback: FpnnAnswerCallback?) {
		if FpnnMtTwoway == quest.header.mType {
			let seqNum = nextSequenceNumber()
			quest.seqNum = seqNum

			if let callback = callback {
				if let answerTask = answerTaskCache.removeValueForKey(seqNum) {
					answerTask.cancelTask.cancel()
				}

				let task = timeout(readTimeout) { [weak self] in
					if let answerStore = self?.answerTaskCache.removeValueForKey(seqNum) {
						let userInfo = [NSLocalizedFailureReasonErrorKey: "seqNum[\(seqNum)] request timed out"]
						let error = NSError(domain: CAPErrorDomain, code: 100, userInfo: userInfo)
						main_async() {
							answerStore.callback(.Failure(.Network(error)))
						}
					}
				}
				answerTaskCache[seqNum] = AnswerTask(quest: quest, callback: callback, cancelTask: task)
			}
		}

		socket.writeData(quest.pack, withTimeout: -1, tag: 0)
	}
}

extension FpnnClient {
	private func nextSequenceNumber() -> UInt32 {
		defer {
			sequenceLock.unlock()
		}

		sequenceLock.lock()
		sequenceNumber += 1
		if sequenceNumber >= UInt32.max {
			sequenceNumber = 0
		}
		return sequenceNumber
	}
}

extension FpnnClient: GCDAsyncSocketDelegate {
	func socket(sock: GCDAsyncSocket, didConnectToHost host: String, port: UInt16) {
		RumTracker.TraceFPNNConnectStatus("success", connectStart: startConnectTime)
		socket.readDataToLength(FpnnHeader.Length, withTimeout: -1, tag: Tag.Header.rawValue)
	}

	func socket(sock: GCDAsyncSocket, didReadData data: NSData, withTag tag: Int) {
		guard let tag = Tag(rawValue: tag) else {
			return
		}

		switch tag {
		case .Header:
			let header = FpnnHeader(data: data)
			lastHeader = header
			socket.readDataToLength(header.bodySize, withTimeout: -1, tag: Tag.Body.rawValue)
		case .Body:
			if let header = lastHeader, answer = FpnnAnswer(header: header, data: data) {
				recievedAnswer(answer)
			} else {
				log.error("lastHeader: \(lastHeader), or decode data pack failed?")
			}
			lastHeader = nil
			socket.readDataToLength(FpnnHeader.Length, withTimeout: -1, tag: Tag.Header.rawValue)
		}
	}

	func socketDidDisconnect(sock: GCDAsyncSocket, withError err: NSError?) {
		if let error = err where working && !answerTaskCache.isEmpty {
			log.error("\(error)")
			if connect() {
				for answerTask in self.answerTaskCache.values {
					self.doSend(answerTask.quest, callback: answerTask.callback)
				}
			} else {
				for answerStore in answerTaskCache.values {
					answerStore.cancelTask.cancel()

					let userInfo = [NSLocalizedFailureReasonErrorKey: "FPNN connection is breakon!"]
					let error = NSError(domain: CAPErrorDomain, code: 101, userInfo: userInfo)
					main_async() {
						answerStore.callback(.Failure(.System(error)))
					}
				}
				answerTaskCache.removeAll()
				working = false
			}
		} else {
			working = false
		}
	}
}
