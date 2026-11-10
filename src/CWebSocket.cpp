#ifndef DEBUG_LSC_WEBSOCKET
    #undef DEBUGINFOS
#endif
/**
 * @file CWebSocket.cpp
 * @author Peter Liebl
 * @brief Implementation of the CWebSocket class
 * @version 1.3
 * @date 2023-11-26
 *
 * Threading model:
 *  - ESP32   : onWebSocketEvent() runs in the AsyncTCP task, dispatching runs in
 *              loop(). The message queue is protected by a mutex, messages only
 *              keep the client id / remote IP (the client may be deleted at any time).
 *  - ESP8266 : callbacks run cooperatively in the SYS context, no lock needed.
 * No exceptions are used (new(std::nothrow), malloc checks), so the code
 * behaves the same with or without -fexceptions.
 */

#include <WebSocket.h>
#include <Appl.h>
#include <Security.h>
#include <AccessToken.h>
#include <FileSystem.h>

#ifdef ESP32
	#define WS_QUEUE_LOCK() std::lock_guard<std::mutex> oQueueGuard(m_oQueueLock)
#else
	#define WS_QUEUE_LOCK()
#endif

/**
 * @brief Creates a WebSocket endpoint and binds its event callback.
 * @param strSocketName URL/path of the WebSocket endpoint.
 * @param bRegisterOnMsgBus Reserved flag for message bus registration.
 */
CWebSocket::CWebSocket(const char* strSocketName, bool bRegisterOnMsgBus) : AsyncWebSocket(strSocketName){
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(strSocketName),bRegisterOnMsgBus);
	(void) bRegisterOnMsgBus;
	// Register the own onWebSocketEvent handler on the socket
	onEvent([this](AsyncWebSocket *pServer, AsyncWebSocketClient *pClient, AwsEventType eType, void *pArg, uint8_t *pData, size_t nLen) {
		this->onWebSocketEvent(pServer, pClient, eType, pArg, pData, nLen);
	});
	// Now listen on the message bus for incomming commands....
	// if(bRegisterOnMsgBus) Appl.MsgBus.registerEventReceiver(this,"WebSocket");
	DEBUG_FUNC_END();
}

/**
 * @brief Deletes all messages that were not dispatched yet.
 */
CWebSocket::~CWebSocket() {
	CWebSocketMessage *pMsg;
	while((pMsg = popMessageFromQueue()) != nullptr) delete pMsg;
}

#pragma region Receiving Messages (Event / Socket)
/**
 * @brief Handles application events relevant to WebSocket processing.
 *
 * MSG_APPL_LOOP drains queued WebSocket messages and periodically cleans up
 * inactive clients. JSON send events are serialized to all clients.
 *
 * @return EVENT_MSG_RESULT_OK after processing.
 */
int CWebSocket::receiveEvent(const void * pSender, int nMsgId, const void * pMessage, int nType) {
	(void) pSender; (void) nType;
    switch(nMsgId) {
		case MSG_APPL_LOOP:
			// Dispatch the messages and cleanup inactive clients
			dispatchMessageQueue();
			if(m_oClientCleanupDelay.isDone()) {
				cleanupClients();
				m_oClientCleanupDelay.restart();
			}
			break;

		case MSG_WEBSOCKET_SEND_JSONNODE :
        case MSG_WIFI_SCAN_RESULT : if(pMessage != nullptr) {
										JsonNode *pDoc = ( JsonNode *) pMessage;
										sendJsonDocMessage(*pDoc);
									}
									break;
		default: break;
    }
    return(EVENT_MSG_RESULT_OK);
}

/**
 * @brief Captures WebSocket events and queues complete messages.
 *
 * Runs in the async TCP context (AsyncTCP task on ESP32) - keep it short and
 * never block. Single-frame messages are queued immediately; messages split
 * into several packets and/or several (fragmented) frames are assembled in
 * the client's _tempObject and queued after the final frame.
 *
 * @param pSocket Socket receiving the event.
 * @param pClient Client that sent the data.
 * @param eType Event type.
 * @param pArg Error code or AwsFrameInfo depending on eType.
 * @param pData Current frame payload.
 * @param nFrameDataLen Length of pData.
 * @see : https://github.com/ESP32Async/ESPAsyncWebServer/wiki#async-websocket-event
 */
void CWebSocket::onWebSocketEvent(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, AwsEventType eType, void *pArg, uint8_t *pData, size_t nFrameDataLen)
{
	DEBUG_FUNC_START_PARMS("eType=%d,nLen=%u", eType,(unsigned) nFrameDataLen);
	if(!pClient) { DEBUG_FUNC_END(); return; }

	if (eType == WS_EVT_ERROR)
	{
		uint16_t nCode = pArg ? *((uint16_t *)pArg) : 0;
		// pData is not guaranteed to be zero terminated - print with length
		ApplLogWarnWithParms("WebSocket[%s][%u] error(%u): %.*s",
					pSocket ? pSocket->url() : "", (unsigned) pClient->id(), (unsigned) nCode,
					pData ? (int) nFrameDataLen : 0, pData ? (const char *) pData : "");
	}
	else if (eType == WS_EVT_CONNECT) {
		DEBUG_INFOS("WS: - WS_EVT_CONNECT : (Client ID: %u)", (unsigned) pClient->id());
	}
	else if (eType == WS_EVT_DISCONNECT) {
		// Called from the client destructor - free an incomplete message, the library will not do it
		DEBUG_INFOS("WS: - WS_EVT_DISCONNECT : (Client ID: %u)", (unsigned) pClient->id());
		CWebSocketMessage *pPending = (CWebSocketMessage *) pClient->_tempObject;
		pClient->_tempObject = nullptr;
		delete pPending;
	}
	else if (eType == WS_EVT_DATA && pArg) {
		AwsFrameInfo *pFrameInfo = (AwsFrameInfo *)pArg;
		DEBUG_INFOS("WS: - MESSAGE(final = %d, num = %u, index = %u, frame len = %u (data len %u))",
					pFrameInfo->final, (unsigned) pFrameInfo->num,
					(unsigned) pFrameInfo->index, (unsigned) pFrameInfo->len, (unsigned) nFrameDataLen);

		bool bFrameStart   = pFrameInfo->index == 0;
		bool bFrameEnd     = (pFrameInfo->index + nFrameDataLen) == pFrameInfo->len;
		bool bMessageStart = bFrameStart && pFrameInfo->num == 0;
		bool bMessageEnd   = bFrameEnd && pFrameInfo->final;

		if (bMessageStart && bMessageEnd)
		{
			// Complete message in one frame and one packet
			CWebSocketMessage * pMsgObj = new (std::nothrow) CWebSocketMessage(pSocket,pClient,pFrameInfo->len,pFrameInfo->message_opcode);
			if(pMsgObj && pMsgObj->isValid()) {
				pMsgObj->setMessageData(pData,0,nFrameDataLen);
				addMessageToQueue(pMsgObj);
			} else {
				ApplLogWarnWithParms("WS: message dropped (%u bytes, out of memory or too large)",(unsigned) pFrameInfo->len);
				delete pMsgObj;
			}
		}
		else {
			CWebSocketMessage * pMsgObj = (CWebSocketMessage *) pClient->_tempObject;
			if(bMessageStart) {
				// New message - drop an incomplete previous one
				delete pMsgObj;
				pMsgObj = new (std::nothrow) CWebSocketMessage(pSocket,pClient,pFrameInfo->len,pFrameInfo->message_opcode);
				if(pMsgObj && !pMsgObj->isValid()) { delete pMsgObj; pMsgObj = nullptr; }
				pClient->_tempObject = pMsgObj;
			} else if(bFrameStart && pMsgObj) {
				// Next frame of a fragmented message - extend the buffer
				if(!pMsgObj->addFrame(pFrameInfo->len)) {
					delete pMsgObj;
					pMsgObj = nullptr;
					pClient->_tempObject = nullptr;
				}
			}

			if(pMsgObj) {
				pMsgObj->setMessageData(pData,pFrameInfo->index,nFrameDataLen);
				if(bMessageEnd) {
					// Ownership moves to the queue - the client must not reference it any longer
					pClient->_tempObject = nullptr;
					addMessageToQueue(pMsgObj);
				}
			} else if(bMessageStart || bFrameStart) {
				ApplLogWarnWithParms("WS: fragmented message dropped (client %u, out of memory or too large)",(unsigned) pClient->id());
			}
		}
	}
	DEBUG_FUNC_END();
}


/**
 * @brief Adds a completed message to the internal processing queue (takes ownership).
 *
 * Messages are processed outside the socket callback to keep the callback short.
 * If too many messages are pending, the new message is dropped.
 */
void CWebSocket::addMessageToQueue(CWebSocketMessage *pMsgObj)
{
	DEBUG_FUNC_START();
	if(pMsgObj) {
		bool bAdded = false;
		{
			WS_QUEUE_LOCK();
			if(m_tMsgQueue.size() < WS_MAX_QUEUED_MESSAGES) {
				m_tMsgQueue.push(pMsgObj);
				bAdded = true;
			}
		}
		if(!bAdded) {
			ApplLogWarn(F("WS: message queue full - message dropped"));
			delete pMsgObj;
		}
	}
	DEBUG_FUNC_END();
}

/**
 * @brief Removes the oldest message from the queue.
 * @return Message (caller owns it) or nullptr if the queue is empty.
 */
CWebSocketMessage * CWebSocket::popMessageFromQueue() {
	WS_QUEUE_LOCK();
	if(m_tMsgQueue.empty()) return(nullptr);
	CWebSocketMessage *pMsg = m_tMsgQueue.front();
	m_tMsgQueue.pop();
	return(pMsg);
}

#pragma endregion

#pragma region Message Sending

/**
 * @brief Sends a JsonNode as a WebSocket text message.
 * @param oDoc Document to serialize.
 * @param pSocket Socket to use. nullptr means this socket.
 * @param pClient Specific client. nullptr broadcasts to all clients.
 *                The pointer must be valid - from loop() context prefer the
 *                overload with CWebSocketMessage, which sends by client id.
 */
void ICACHE_FLASH_ATTR CWebSocket::sendJsonDocMessage(JsonNode &oDoc, AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient) {
	DEBUG_FUNC_START();
	if(!pSocket) pSocket = this;
	String strData = oDoc.getAsJsonText();
	DEBUG_INFOS("WS: - sending message (%u bytes) : %s",(unsigned) strData.length(),strData.c_str());
	// The String overloads copy the data into a shared buffer (no raw buffer handling needed).
	// If the client queue is full, the library discards the message instead of failing.
	if(pClient) {
		if(pClient->status() == WS_CONNECTED) pClient->text(strData);
	}
	else pSocket->textAll(strData);
	DEBUG_FUNC_END();
}

/**
 * @brief Sends a JsonNode as reply to a queued message.
 *
 * The client is looked up by id inside the library, so a disconnected
 * client is simply skipped.
 */
void ICACHE_FLASH_ATTR CWebSocket::sendJsonDocMessage(JsonNode &oDoc, CWebSocketMessage *pMessage) {
	DEBUG_FUNC_START();
	if(pMessage) {
		AsyncWebSocket *pSocket = pMessage->pSocket ? pMessage->pSocket : this;
		String strData = oDoc.getAsJsonText();
		DEBUG_INFOS("WS: - reply to client %u (%u bytes)",(unsigned) pMessage->ClientId,(unsigned) strData.length());
		// Looks up the client by id; a disconnected client is skipped silently.
		pSocket->text(pMessage->ClientId, strData);
	}
	DEBUG_FUNC_END();
}

/**
 * @brief Prepares the access-denied payload in the request document.
 *
 * The original request document is reused as response container and the token is
 * explicitly invalidated in the response.
 */
static void prepareAccessDeniedDoc(JsonNode &oRequestDoc) {
	// Remember the requested command / type so it can be inserted in the message again.
	String strCommand = oRequestDoc.getValue("command");
	String strType    = oRequestDoc.getValue("type");

	oRequestDoc.clear();

	JsonNode * pPayload = oRequestDoc.createPayloadStructure("error","401");
	oRequestDoc["AccessToken"] 	= "notValid";		// destroy an existing access token
	if(pPayload) {
		(* pPayload)["msg"] 		= "access denied";	// set message for user...
		(* pPayload)["command"] 	= strCommand;		// command that is not allowed
		(* pPayload)["type"]    	= strType;			// type that is not allowed
	}
}

/**
 * @brief Sends an access-denied payload to a specific client.
 */
void ICACHE_FLASH_ATTR CWebSocket::sendAccessDeniedMessage(JsonNode &oRequestDoc,AsyncWebSocketClient *pClient) {
	if(pClient) {
		prepareAccessDeniedDoc(oRequestDoc);
		sendJsonDocMessage(oRequestDoc,nullptr,pClient);
	}
}

/**
 * @brief Sends an access-denied payload as reply to a queued message.
 */
void ICACHE_FLASH_ATTR CWebSocket::sendAccessDeniedMessage(JsonNode &oRequestDoc,CWebSocketMessage *pMessage) {
	if(pMessage) {
		prepareAccessDeniedDoc(oRequestDoc);
		sendJsonDocMessage(oRequestDoc,pMessage);
	}
}

#pragma endregion

#pragma region Authentication

/**
 * @brief Replaces the comma-delimited command list that requires auth.
 * @return Stored command list.
 */
String CWebSocket::setNeedsAuth(const String &strCommands) {
	m_strNeedsAuth = strCommands;
	return m_strNeedsAuth;
};

/**
 * @brief Gets the comma-delimited command list that requires auth.
 */
String CWebSocket::getNeedsAuth() {
	return m_strNeedsAuth;
};


/**
 * @brief Checks whether a command is listed as requiring authentication.
 *
 * Compares complete list entries (case insensitive), so "config" does not
 * match "saveconfig". An empty command never needs auth.
 * @param strCommand Command name.
 * @return true when the command appears in m_strNeedsAuth.
 */
bool CWebSocket::needsAuth(String &strCommand) {
	if(strCommand.length() == 0) return(false);
	String strList = "," + m_strNeedsAuth + ",";
	String strKey  = "," + strCommand + ",";
	strList.replace(" ","");
	strList.toLowerCase();
	strKey.toLowerCase();
	return(strList.indexOf(strKey) > -1);
}

/**
 * @brief Validates the token in a request against the client IP address.
 * @param oJsonRequest Request document containing a "token" value.
 * @param oRemoteIP IP address of the requesting client.
 * @return true when token structure, IP and application key are valid.
 */
bool CWebSocket::checkAuth(JsonNode &oJsonRequest, const IPAddress &oRemoteIP) {
	DEBUG_FUNC_START();
	bool isAuthenticated = false;
	String strAuthToken = oJsonRequest.getValue("token");
	if(strAuthToken.length() > 10) {
		String strClientRemoteIP = oRemoteIP.toString();
		CAccessToken oToken(strAuthToken.c_str());
		isAuthenticated = oToken.isAuthValid(strClientRemoteIP.c_str(),APPL_SECURITY_TOKEN_KEY);
	}
	DEBUG_FUNC_END_PARMS("%d",isAuthenticated);
	return(isAuthenticated);
}

/**
 * @brief Validates the token in a request against the client IP address.
 * @param pClient Requesting client (must be valid).
 */
bool CWebSocket::checkAuth(JsonNode &oJsonRequest, AsyncWebSocketClient *pClient) {
	return(pClient ? checkAuth(oJsonRequest, pClient->remoteIP()) : false);
}

#pragma endregion

#pragma region Message Dispatching


/**
 * @brief Processes and deletes all queued WebSocket messages.
 *
 * Call this from the main loop or via MSG_APPL_LOOP to keep socket callbacks
 * lightweight. Each message is removed from the queue before it is processed,
 * so new messages can be queued concurrently (ESP32) or re-entrantly (ESP8266 yield).
 */
void CWebSocket::dispatchMessageQueue()
{
	// Process at most the messages present now - avoids starving loop() under load
	for(int nCount = 0; nCount < WS_MAX_QUEUED_MESSAGES; nCount++) {
		CWebSocketMessage * pMessageToProcess = popMessageFromQueue();
		if(!pMessageToProcess) break;
		dispatchMessage(pMessageToProcess);
		delete pMessageToProcess;
	}
}

/**
 * @brief Dispatches a parsed JSON request.
 *
 * Built-in commands cover status/config retrieval, authenticated config changes,
 * restart, scans, backup/restore and factory reset. Unknown commands return
 * false so application code can handle them through MSG_WEBSOCKET_DATA_RECEIVED.
 *
 * @param oJsonRequest Parsed request document.
 * @param pMessage Original WebSocket message metadata.
 * @return true when the command was handled or access-denied was sent.
 */
bool CWebSocket::dispatchJsonMessage(JsonNode & oJsonRequest, CWebSocketMessage *pMessage) {
	DEBUG_FUNC_START();
	if(!pMessage) return(false);
	bool bResult = true;
	CFS oFS;
	// Web Browser sends some commands, check which command is given
	String strCommand = oJsonRequest.getValue("command");
	strCommand.toLowerCase();
	DEBUG_INFOS("WS: dispatching command \"%s\"",strCommand.c_str());
	bool isAuthNeeded = needsAuth(strCommand);
	bool isAuthenticated = isAuthNeeded ? checkAuth(oJsonRequest,pMessage->RemoteIP) : false;
	DEBUG_INFOS("WS: Auth needed : %d, Authenticated : %d",isAuthNeeded,isAuthenticated);
	if(isAuthNeeded && !isAuthenticated) {
		ApplLogErrorWithParms("WS: Access denied - %s",strCommand.c_str());
		sendAccessDeniedMessage(oJsonRequest,pMessage);
	} else {
		if (strCommand.equalsIgnoreCase(F("getsysstatus")))
		{
			JsonNode * pPayloadNode = oJsonRequest.createPayloadStructure("update","sysstatus");
			if(pPayloadNode) Appl.writeSystemStatusTo(*pPayloadNode);
			sendJsonDocMessage(oJsonRequest,pMessage);
		}
		else if (strCommand.equalsIgnoreCase(F("getstatus")))
		{
			// The status text is JSON - inserted as object (the frontend ignores a string payload)
			const char * pszStatus = Appl.getStatusAsText();
			oJsonRequest.createPayloadStructure("update","status",pszStatus ? pszStatus : "{}");
			sendJsonDocMessage(oJsonRequest,pMessage);
		}
		else if (strCommand.equalsIgnoreCase(F("getconfig")))
		{
			// NO authentication needed, cause critical informations are hidden (!)
			JsonNode * pCfgNode = oJsonRequest.createPayloadStructure("update","config");
			if(pCfgNode) Appl.writeConfigTo(*pCfgNode,true);
			sendJsonDocMessage(oJsonRequest,pMessage);
		}
		else if (strCommand.equalsIgnoreCase(F("saveconfig")))
		{
			JsonNode * pPayload = oJsonRequest.getObject("payload");
			if(isAuthenticated && pPayload) {
				// First load the config - to enable validation of settings (!)
				// then write the new config file to the file system
				// ... and ask for a reboot !
				DEBUG_INFO(" - updating current config");
				Appl.readConfigFrom(*pPayload);
				DEBUG_INFO(" - saving config...");
				Appl.saveConfig();
				DEBUG_INFO(" - requesting reboot...");
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,"WebSocket: save config",0);
			}
		}
		else if (strCommand.equalsIgnoreCase(F("restart")))
		{
			if(isAuthenticated) {
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,"WebSocket: restart",0);
			}
		}
		// Note for the scan receivers: pMessage->pClient is only valid during this call
		// (the client may disconnect) - keep pMessage->ClientId for a later answer.
		else if (strCommand.equalsIgnoreCase(F("scanwifi")))
		{
			DEBUG_INFO("WS: sending scan wifi request...");
			Appl.MsgBus.sendEvent(this,MSG_WIFI_SCAN,pMessage->pClient,0);
		}
		else if (strCommand.equalsIgnoreCase(F("scanrf433")))
		{
			Appl.MsgBus.sendEvent(this,MSG_RF433_SCAN,pMessage->pClient,0);
		}
		else if (strCommand.equalsIgnoreCase(F("getbackup")))
		{
			if(isAuthenticated) { // To ensure - only if authenticated...
				JsonNode * pPayload = oJsonRequest.createPayloadStructure("backup","config");
				if(pPayload) {
					// The backup is the config file as stored in the file system.
					// Only if the file is missing or invalid, the current (in memory) config is used.
					// Note: writeConfigTo() must not be called on a loaded file - it would overwrite
					// all file values with the in memory values (i.e. defaults, if the load failed at boot).
					bool bFileLoaded = false;
					if(oFS.fileExists(JSON_APPL_CONFIG_FILE)) {
						String strData;
						if(oFS.loadFileToString(JSON_APPL_CONFIG_FILE,strData)) {
							// strict first, then legacy mode (files written by older versions)
							bFileLoaded = pPayload->parse(strData.c_str()) != nullptr || pPayload->parse(strData.c_str(),true) != nullptr;
						}
						if(!bFileLoaded) {
							ApplLogWarnWithParms(F("WS: Config file %s is not valid JSON, using current config"),JSON_APPL_CONFIG_FILE);
						}
					} else {
						ApplLogWarnWithParms(F("WS: Config file %s not found, using current config"),JSON_APPL_CONFIG_FILE);
					}
					if(!bFileLoaded) Appl.writeConfigTo(*pPayload,false);
				}
				sendJsonDocMessage(oJsonRequest,pMessage);
			}
		}
		else if (strCommand.equalsIgnoreCase(F("restorebackup")))
		{
			JsonNode * pCfgData = oJsonRequest.getObject("payload");
			if(isAuthenticated && pCfgData) { // To ensure - only if authenticated...
				// Only a non empty JSON object is a configuration. Writing truncates the
				// current file - a failed write would leave the device without config,
				// so reboot only on success and report errors to the client.
				bool bValid = pCfgData->isJsonObject() && pCfgData->Elements.size() > 0;
				bool bSaved = false;
				if(bValid) {
					ApplLogInfo("WS: Restoring config from backup...");
					DEBUG_JSON_OBJ((*pCfgData));
					bSaved = oFS.saveJsonContentToFile(JSON_APPL_CONFIG_FILE,*pCfgData);
				}
				if(bSaved) {
					Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,"WebSocket: restore backup",0);
				} else {
					ApplLogErrorWithParms("WS: restore backup failed (%s)", bValid ? "write error" : "invalid data");
					JsonNode oReply;
					JsonNode *pReply = oReply.createPayloadStructure("error","restorebackup");
					if(pReply) (*pReply)["msg"] = bValid ? "write error" : "invalid backup data";
					sendJsonDocMessage(oReply,pMessage);
				}
			}
		}
		else if (strCommand.equalsIgnoreCase(F("factoryreset")))
		{
			if(isAuthenticated) {
				ApplLogInfo("WS: Restoring factory settings...");
				oFS.deleteFile(JSON_APPL_CONFIG_FILE);
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,"WebSocket: factory reset",0);
			}
		}
		else bResult = false;
	}
	DEBUG_FUNC_END_PARMS("%d",bResult);
	return bResult;
}

/**
 * @brief Parses and dispatches one queued WebSocket message.
 *
 * Non-JSON messages and unknown commands are forwarded to the application
 * message bus for custom handling.
 *
 * @param pMessage Queued message to process.
 * @return true when the message was parsed and handled by built-in dispatch.
 */
bool CWebSocket::dispatchMessage( CWebSocketMessage *pMessage) {
	DEBUG_FUNC_START();
	if(!pMessage || !pMessage->isValid()) return(false);
	bool bResult = true;

	// The client may have disconnected since the message was queued - re-resolve it.
	// nullptr means "gone"; receivers must check pClient before using it.
	AsyncWebSocket *pSocket = pMessage->pSocket ? pMessage->pSocket : this;
	pMessage->pClient = pSocket->client(pMessage->ClientId);

	// We should always get a JSON object (stringified) from browser, so parse it
	JsonNode oXChangeDoc;
	const char *psz = oXChangeDoc.parse((const char *)pMessage->pSerializedMessage);
    // parse() returns nullptr on any error (including trailing garbage)
    if(!psz) {
        ApplLogError(F("WS: Parse message error"));
		Appl.MsgBus.sendEvent(this,MSG_WEBSOCKET_DATA_RECEIVED,pMessage,0);
		bResult = false;
    } else {
		bResult = dispatchJsonMessage(oXChangeDoc,pMessage);
		if(!bResult) {
			Appl.MsgBus.sendEvent(this,MSG_WEBSOCKET_DATA_RECEIVED,pMessage,1);
		}
	}
	DEBUG_FUNC_END_PARMS("%d",bResult);
	return(bResult);
}

#pragma endregion
