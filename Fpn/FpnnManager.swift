//
//  FpnnManager.swift
//  MeMe
//
//  Created by LuanMa on 16/9/7.
//  Copyright © 2016年 sip. All rights reserved.
//

import Result
import SwiftyJSON

class FpnnManager {
	static let instance = FpnnManager()
	private let client = FpnnClient(timeout: LSGlobals.FpnnTimeout)

	private init() {
		NSNotificationCenter.defaultCenter().addObserver(self, selector: #selector(FpnnManager.handleNotification(_:)), name: Notification.ApiNetworkConfigChanged, object: nil)
	}

	deinit {
		release()
		NSNotificationCenter.defaultCenter().removeObserver(self)
	}

	static func sendMethod(method: String, params: [String: AnyObject], withEncrypt encrypted: Bool = true, completion: (Result<[String: AnyObject]?, CAPError>) -> Void) {
		FpnnManager.instance.sendRequest(method, params: params, withEncrypt: encrypted, completion: completion)
	}

	private func sendRequest(method: String, params: [String: AnyObject], withEncrypt encrypted: Bool = true, completion: (Result<[String: AnyObject]?, CAPError>) -> Void) {
		let myParams: [String: AnyObject]
		if encrypted {
			do {
				myParams = try encryptRequest(params, withMethod: method)
			} catch {
				completion(.Failure(error as! CAPError))
				return
			}
		} else {
			myParams = params
			log.debug("[\(method)]params: \(myParams)")
		}

		let start = NSDate()
		let quest = FpnnQuest(method: method, params: myParams, isOneway: false)
		let callback: FpnnAnswerCallback = { [weak self] result in
			guard let wself = self else {
				return
			}

			var resultSize = 0
			let responseResult: Result<[String: AnyObject]?, CAPError>
			switch result {
			case .Success(let answer):
				resultSize = answer.packSize
				if answer.isOk {
					let dict: [String: AnyObject]?
					if encrypted {
						dict = wself.decryptResponse(answer)
					} else {
						dict = answer.answerMap
					}
					log.debug("\(method) response: \(dict)")
					responseResult = .Success(dict)
				} else {
					let json = JSON(answer.answerMap)
					if let code = json["code"].int, raiser = json["raiser"].string {
						let error = NSError(domain: raiser, code: code, userInfo: json.dictionaryObject!)
						responseResult = .Failure(.Network(error))
						if code == 100002 {
							NSNotificationCenter.defaultCenter().postNotificationName(Notification.Unauthorized, object: nil)
						}
					} else {
						log.error("\(answer.answerMap)")
						responseResult = .Failure(.Network(nil))
					}
				}
			case .Failure(let error):
				log.error("\(error)")
				responseResult = .Failure(error)
			}

			RumTracker.TraceFPNNAction(method, fnppStatus: responseResult, actionStart: start, responseSize: resultSize)
			completion(responseResult)
		}

		connect()
		client.send(quest, callback: callback)
	}

	func release() {
		if client.working {
			client.disconnect()
		}
	}
}

extension FpnnManager {
	private func connect() {
		guard !client.working else {
			return
		}

		client.connect(host: APIConfigService.FPNNServer, port: UInt16(APIConfigService.FPNNPort))
	}

	@objc func handleNotification(notification: NSNotification) {
		client.disconnect()
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

	private func decryptResponse(fpnAnswer: FpnnAnswer) -> [String: AnyObject]? {
		guard let result = fpnAnswer.wantForKey("result"), response = result as? NSData else {
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
}
