//
//  FpnWorkMgr.swift
//  LiveStream
//
//  Created by 邢海华 on 16/6/12.
//  Copyright © 2016年 sip. All rights reserved.
//

import UIKit
import Result
import SwiftyJSON
import FunPlusRUM

class FpnWorkMgr {

	static let workQueue = NSMutableDictionary()

	let UUID: String

	var client: FPNNClient?

	init() {
		UUID = NSUUID().UUIDString
		FpnWorkMgr.workQueue.setValue(self, forKey: UUID)
	}

	private func connect(completion: (lsFPNNClient: FPNNClient, success: Bool) -> Void) -> Void {
		let connectStart = NSDate()
		client = FPNNClient(host: APIConfigService.FPNNServer, port: APIConfigService.FPNNPort, timeout: LSGlobals.FPNNTimeout) { lsFPNNClient, nssStreamEvent, nsError in
			RumTracker.TraceFPNNConnect(nssStreamEvent, error: nsError, connectStart: connectStart)
			completion(lsFPNNClient: lsFPNNClient, success: nssStreamEvent == NSStreamEvent.OpenCompleted)
		}
	}

	func sendRequest(method: String, params: [String: AnyObject], withEncrypt encrypted: Bool = true, completion: (Result<[String: AnyObject]?, CAPError>) -> Void) {
		let myParams: [String: AnyObject]
		if encrypted {
			do {
				myParams = try encryptRequest(params, withMethod: method)
			} catch {
				release()
				completion(.Failure(error as! CAPError))
				return
			}
		} else {
			myParams = params
			log.debug("[\(method)]params: \(myParams)")
		}

		let quest = FPNNQuest(method: method, params: myParams, isOneway: false)
		let connetionBlock = { [weak self](lsFPNNClient: FPNNClient, success: Bool) in
			guard let wself = self else {
				return
			}

			lsFPNNClient.FPNNEventCallback = nil
			guard success else {
				completion(.Failure(.Network(nil)))
				wself.release()
				return
			}

			let start = NSDate()
			log.debug("-- [\(method)] connectd: \(success) --")
			lsFPNNClient.sendQuest(quest) { [weak self] answer, nsError in
				guard let wwself = self else {
					return
				}
				let result: Result<[String: AnyObject]?, CAPError>
				if let error = nsError {
					log.error("\(error)")
					result = .Failure(.Network(error))
				} else if let answer = answer {
					if answer.isOk() {
						let dict: [String: AnyObject]?
						if encrypted {
							dict = wwself.decryptResponse(answer)
						} else {
							dict = answer.answerMap as? [String: AnyObject]
						}
						log.debug("\(method) response: \(dict)")
						result = .Success(dict)
					} else if let answerMap = answer.answerMap {
						// error message: {ex=Invalid Token., raiser=UserGate, code=100002}
						let json = JSON(answerMap)
						if let code = json["code"].int, raiser = json["raiser"].string {
							let error = NSError(domain: raiser, code: code, userInfo: json.dictionaryObject!)
							result = .Failure(.Network(error))
							if code == 100002 {
								NSNotificationCenter.defaultCenter().postNotificationName(Notification.Unauthorized, object: nil)
							}
						} else {
							log.error("\(answer.answerMap)")
							result = .Failure(.Network(nil))
						}
					} else {
						result = .Failure(.Network(nil))

					}
				} else {
					result = .Failure(.Network(nil))
				}

				RumTracker.TraceFPNNAction(method, fnppStatus: result, actionStart: start, responseSize: Int(answer.rawResponseLength))
				completion(result)
				wself.release()
			}
		}

		log.debug("-- [\(method)] connect --")
		connect(connetionBlock)
	}

	private func encryptRequest(toEncryptParams: [String: AnyObject], withMethod method: String) throws -> [String: AnyObject] {
		guard let account = LSUserService.currentAccount, key = account.key, iv = account.iv else {
			throw CAPError.Auth(NSError(domain: "livestream", code: CAPErrorCode.Auth.rawValue, userInfo: nil))
		}

		var params = toEncryptParams
		params["sessionToken"] = account.sessionToken

		var jsonString: String?
		do {
			let jsonData = try NSJSONSerialization.dataWithJSONObject(params, options: [])
			jsonString = String(data: jsonData, encoding: NSUTF8StringEncoding)
		} catch {
			throw CAPError.System(nil)
		}

		if let jsonString = jsonString {
			let data = AESUtil.encrypt(key, iv: iv, dataStr: jsonString)
			let len = 512
			if jsonString.characters.count >= len {
				log.debug("[\(method)]params: \(jsonString.substringToIndex(jsonString.startIndex.advancedBy(len - 1)))")
			} else {
				log.debug("[\(method)]params: \(jsonString)")
			}
			return ["sessionToken": account.sessionToken, "params": data]
		}

		return toEncryptParams
	}

	private func decryptResponse(fpnAnswer: FPNNAnswer) -> [String: AnyObject]? {
		guard let result = fpnAnswer.want("result"), response = result as? NSData else {
			log.error("ERROR: \(fpnAnswer)")
			return nil
		}

		guard let account = LSUserService.currentAccount, key = account.key, iv = account.iv else {
			log.error("No account?!")
			return nil
		}

		let responseStr: String = AESUtil.decrypt(key, iv: iv, data: response)
		// log.debug("responseStr:\(responseStr)")
		do {
			let jsonData = responseStr.dataUsingEncoding(NSUTF8StringEncoding, allowLossyConversion: false)
			let jsonDictionary = try NSJSONSerialization.JSONObjectWithData(jsonData!, options: NSJSONReadingOptions.MutableContainers) as? [String: AnyObject]
			// log.debug("\(jsonDictionary)")
			return jsonDictionary
		} catch {
			log.error("parse error")
		}

		return nil
	}

	private func release() {
		FpnWorkMgr.workQueue.removeObjectForKey(UUID)
//		if let client = client {
		// client.stopAndRelease()
		// client.destroyConn(defaultError())
//		}
	}

	deinit {
		if let client = client {
			log.info("Retain count \(CFGetRetainCount(client))")
		}
		client = nil
		log.debug("deinit")
	}
}
