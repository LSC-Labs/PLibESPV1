#ifndef DEBUG_LSC_WEBSOCKET
    #undef DEBUGINFOS
#endif
/**
 * @file CWebSocket.cpp
 * @author Peter Liebl
 * @brief Implementation of the CWebSocket class
 * @version 1.2	
 * @date 2023-11-26
 *       
 * 
 */

#include <WebSocket.h>
#include <Appl.h>
#include <Security.h>
#include <AccessToken.h>
#include <FileSystem.h>
// #include <JsonHelper.h>

#define DEFAULT_REQUEST_DOC_SIZE  2048
#define DEFAULT_RESPONSE_DOC_SIZE 2048

/**
 * @brief Creates a WebSocket endpoint and binds its event callback.
 * @param strSocketName URL/path of the WebSocket endpoint.
 * @param bRegisterOnMsgBus Reserved flag for message bus registration.
 */	
CWebSocket::CWebSocket(const char* strSocketName, bool bRegisterOnMsgBus) : AsyncWebSocket(strSocketName){
	DEBUG_FUNC_START_PARMS("%s,%d",NULL_POINTER_STRING(strSocketName),bRegisterOnMsgBus);
	std::function<void(	AsyncWebSocket *, 
						AsyncWebSocketClient *, 
						AwsEventType, 
						void *, 
						uint8_t *, 
						size_t)> funcOnEvent;

	// Prepare the bind of the own onWebSocketEvent Handler function
	funcOnEvent = std::bind(&CWebSocket::onWebSocketEvent,this,
							std::placeholders::_1, 
							std::placeholders::_2, 
							std::placeholders::_3, 
							std::placeholders::_4, 
							std::placeholders::_5, 
							std::placeholders::_6);

	// Register the onEventCallback to the socket
	onEvent(funcOnEvent);

	// Now listen on the message bus for incomming commands....
	// if(bRegisterOnMsgBus) Appl.MsgBus.registerEventReceiver(this,"WebSocket");
	DEBUG_FUNC_END();
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
 * Incoming data is copied into CWebSocketMessage objects. Single-frame messages
 * are queued immediately; multi-frame messages are assembled in the client's
 * temporary object and queued only after the final frame.
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
	DEBUG_FUNC_START_PARMS("eType=%d,nLen=%d", eType,nFrameDataLen);

	if (eType == WS_EVT_ERROR)
	{
		DEBUG_INFOS(" - WS_EVT_ERROR : (%d)", *((uint16_t *)pArg));
		ApplLogWarnWithParms("WebSocket[%s][%u] error(%u): %s\r\n", pSocket->url(), pClient->id(), *((uint16_t *)pArg), (char *)pData);
	} 
	else if (eType == WS_EVT_CONNECT) {
		// Client connected
		DEBUG_INFOS("WS: - WS_EVT_CONNECT : (Client ID: %u IP: %s)", pClient->id(), pClient->remoteIP().toString().c_str()	);
	}
	else if (eType == WS_EVT_DISCONNECT) {
		// Client connected
		DEBUG_INFOS("WS: - WS_EVT_DISCONNECT : (Client ID: %u IP: %s)", pClient->id(), pClient->remoteIP().toString().c_str()	);
	}
	else if (eType == WS_EVT_DATA) {
	
		AwsFrameInfo *pFrameInfo = (AwsFrameInfo *)pArg;
		// Notice until doc is validated (len in multi frames)....	
		DEBUG_INFOS("WS: - MESSAGE(final = %d, index = %lld, num = %d, total len = %lld (data len %d))",
					pFrameInfo->final,
					pFrameInfo->index,
					pFrameInfo->num,
					pFrameInfo->len,
					nFrameDataLen
					);
		
		// Is it a single frame message ?
		if (pFrameInfo->final && pFrameInfo->index == 0 && pFrameInfo->len == nFrameDataLen)
		{
			DEBUG_INFO("============ New Single Socket Message received =============");
			if(pFrameInfo->opcode == WS_TEXT) {
				DEBUG_INFO("WS: - EVT_DATA : (Single Message Frame - TEXT)");
			} else if(pFrameInfo->opcode == WS_BINARY) {
				DEBUG_INFO("WS: - EVT_DATA : (Single Message Frame - BINARY)");
			} else {
				DEBUG_INFO("WS: - EVT_DATA : (Single Message Frame - OTHER)");
			}
			CWebSocketMessage * pMsgObj = new CWebSocketMessage(pSocket,pClient,nFrameDataLen,pFrameInfo->opcode);
			pMsgObj->setMessageData(pData,0,nFrameDataLen);
			addMessageToQueue(pMsgObj);
		}
		// Handle segmented messages
		else {
			DEBUG_INFO("============ New Multi Socket Message received =============");
			// First frame ? allocate the WebSocket message object in temp of client
			if(pFrameInfo->index == 0) {
				DEBUG_INFO("WS:   - allocating new message object..");
				pClient->_tempObject = new CWebSocketMessage(pSocket,pClient,pFrameInfo->len,pFrameInfo->opcode);
			}
			// Move date into WebSocket message object...
			CWebSocketMessage * pMsgObj = (CWebSocketMessage *) pClient->_tempObject;
			if(pMsgObj) {
				DEBUG_INFOS("WS:   - storing data at %lld len = %d",pFrameInfo->index,nFrameDataLen);
				pMsgObj->setMessageData(pData,pFrameInfo->index,nFrameDataLen);
				DEBUG_INFOS("%s",pData);
			}

			// was it the final frame ? then store the data into the message queue...
			if(pFrameInfo->final && (pFrameInfo->index + nFrameDataLen) == pFrameInfo->len) {
				DEBUG_INFO("WS:   - pushing message to queue...");
				addMessageToQueue(pMsgObj);
			}
		}
	}
	DEBUG_FUNC_END();
}


/**
 * @brief Adds a completed message to the internal processing queue.
 *
 * Messages are processed outside the socket callback to keep the callback short.
 */
void CWebSocket::addMessageToQueue(CWebSocketMessage *pMsgObj)
{
	DEBUG_FUNC_START();
	if(pMsgObj) {
		m_tMsgQueue.push(pMsgObj);
		#ifdef DEBUGINFOS
			if(pMsgObj->MessageType == WS_TEXT) {
				DEBUG_INFO("WS: message pushed to queue :");
				DEBUG_INFOS("%s",pMsgObj->pSerializedMessage);
			}
		#endif
	}

	DEBUG_FUNC_END();
}

#pragma endregion

#pragma region Message Sending

/** 
 * @brief Sends a JsonNode as a WebSocket text message.
 * @param oDoc Document to serialize.
 * @param pSocket Socket to use. nullptr means this socket.
 * @param pClient Specific client. nullptr broadcasts to all clients.
 * @see  https://github.com/ESP32Async/ESPAsyncWebServer/wiki#asyncwebsocketmessagebuffer-and-makebuffer
 */
 void ICACHE_FLASH_ATTR CWebSocket::sendJsonDocMessage(JsonNode &oDoc, AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient) {
	DEBUG_FUNC_START();
	// If no socket is in place, use your own socket...
	if(!pSocket) pSocket = this;

    // Serialize the message to be sent...
	String strData = oDoc.getAsJsonText();
	size_t nSize = strData.length();
	DEBUG_INFOS("WS: - allocating buffer(%u bytes)",nSize);
	AsyncWebSocketMessageBuffer* pBuffer = pSocket->makeBuffer(nSize);
	#ifdef DEBUG_LSC_WEBSOCKET
		assert(pBuffer);
	#endif
	strncpy((char *) pBuffer->get(),strData.c_str(),nSize);
	// serializeJson(oDoc,pBuffer->get(),nSize);
	#ifdef DEBUGINFOS
		DEBUG_INFOS("WS: - sending message (%u bytes)\n",nSize);
		for(size_t nIdx = 0; nIdx < nSize; nIdx++) Serial.printf("%c",pBuffer->get()[nIdx]);
		Serial.print("\n");
	#endif
	if(pClient) pClient->text(pBuffer);
	else pSocket->textAll(pBuffer);
	DEBUG_FUNC_END();
}

/** 
 * @brief Sends an access-denied payload to a specific client.
 *
 * The original request document is reused as response container and the token is
 * explicitly invalidated in the response.
 */
void ICACHE_FLASH_ATTR CWebSocket::sendAccessDeniedMessage(JsonNode &oRequestDoc,AsyncWebSocketClient *pClient) {
	if(pClient) {
		// Remember the requested command / type so it can be inserted in the message again.
		String strCommand = oRequestDoc.getValue("command");
        String strType    = oRequestDoc.getValue("type");

		oRequestDoc.clear();
		
		JsonNode * pPayload = oRequestDoc.createPayloadStructure("error","401");
		oRequestDoc["AccessToken"] 	= "notValid";		// destroy an existing access token
		(* pPayload)["msg"] 			= "access denied";	// set message for user...
		(* pPayload)["command"] 		= strCommand;			// command that is not allowed
		(* pPayload)["type"]    		= strType;				// type that is not allowed 
		sendJsonDocMessage(oRequestDoc,nullptr,pClient);
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
 * @param strCommand Lowercase command name.
 * @return true when the command appears in m_strNeedsAuth.
 */
bool inline CWebSocket::needsAuth(String &strCommand) {
	return(m_strNeedsAuth.indexOf(strCommand) > -1);
}

/**
 * @brief Validates the token in a request against the client IP address.
 * @param oJsonRequest Request document containing a "token" value.
 * @param pClient Requesting client.
 * @return true when token structure, IP and application key are valid.
 */
bool CWebSocket::checkAuth(JsonNode &oJsonRequest, AsyncWebSocketClient *pClient) {
	DEBUG_FUNC_START();
	DEBUG_JSON_OBJ(oJsonRequest);
	bool isAuthenticated = false;
	String strAuthToken = oJsonRequest.getValue("token");
	DEBUG_INFOS("Auth Token : %s",strAuthToken.c_str());
	DEBUG_INFOS(" - Length check %d && Client Check (%d)",strAuthToken.length() > 10, pClient ? 1 : 0);
	if(strAuthToken.length() > 10 && pClient) {
		String strClientRemoteIP = pClient->remoteIP().toString();
		CAccessToken oToken(strAuthToken.c_str());
		
		isAuthenticated = oToken.isAuthValid(strClientRemoteIP.c_str(),APPL_SECURITY_TOKEN_KEY);
	}
	DEBUG_FUNC_END_PARMS("%d",isAuthenticated);
	return(isAuthenticated);
}

#pragma endregion

#pragma region Message Dispatching

     
/**
 * @brief Processes and deletes all queued WebSocket messages.
 *
 * Call this from the main loop or via MSG_APPL_LOOP to keep socket callbacks
 * lightweight and avoid unbounded queue growth.
 */
void CWebSocket::dispatchMessageQueue()
{
	while(!m_tMsgQueue.empty()) {
		CWebSocketMessage * pMessageToProcess = m_tMsgQueue.front();
		dispatchMessage(pMessageToProcess);
		delete(pMessageToProcess);
		m_tMsgQueue.pop();
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
	bool bResult = true;
	CFS oFS;
	// Json Document is ready...
	// Web Browser sends some commands, check which command is given
	String strCommand = oJsonRequest.getValue("command");
	strCommand.toLowerCase();
	DEBUG_INFOS("WS: dispatching command \"%s\"",strCommand.c_str());
	bool isAuthNeeded = needsAuth(strCommand);
	bool isAuthenticated = isAuthNeeded ? checkAuth(oJsonRequest,pMessage->pClient) : false;
	DEBUG_INFOS("WS: Auth needed : %d, Authenticated : %d",isAuthNeeded,isAuthenticated);
	if(isAuthNeeded && !isAuthenticated) {
		ApplLogErrorWithParms("WS: Access denied - %s",strCommand.c_str());
		sendAccessDeniedMessage(oJsonRequest,pMessage->pClient);
	} else {
		// bool isAuthenticated = bNeedsAuth ? isAuthTokenValid(strAuthToken, strClientRemoteIP) : true;
		if (strCommand.equalsIgnoreCase(F("getsysstatus")))
		{
			JsonNode * pPayloadNode = oJsonRequest.createPayloadStructure("update","sysstatus");
			Appl.writeSystemStatusTo(*pPayloadNode);
			sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
		}
		if (strCommand.equalsIgnoreCase(F("getstatus")))
		{
			const char * pszStatus = Appl.getStatusAsText();
			oJsonRequest.createPayloadStructure("update","status",pszStatus);
			// JsonObject oStatusNode = LSC::createPayloadStructure(F("update"),F("status"),oJsonRequest);
			// Appl.writeStatusTo(oStatusNode);
			sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
		}
		else if (strCommand.equalsIgnoreCase(F("getconfig")))
		{
			// NO authentication needed, cause critical informations are hidde (!)
			JsonNode * pCfgNode = oJsonRequest.createPayloadStructure("update","config");
			Appl.writeConfigTo(*pCfgNode,true);
			sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
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
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,nullptr,0);
			}
		} 
		else if (strCommand.equalsIgnoreCase(F("restart")))
		{
			if(isAuthenticated) {
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,nullptr,0);
			}
		}
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
				// DynamicJsonDocument oResponseDoc(DEFAULT_RESPONSE_DOC_SIZE);
				JsonNode * pPayload = oJsonRequest.createPayloadStructure("backup","config");
				if(oFS.fileExists(JSON_APPL_CONFIG_FILE)) {
					// loadJsonContentFromFile(JSON_CONFIG_DEFAULT_NAME,oXChangeDoc);
					String strData;
					oFS.loadFileToString(JSON_APPL_CONFIG_FILE,strData);
					pPayload->parse(strData.c_str());
				} else {
					ApplLogWarnWithParms(F("WS: Config file %s not found, using current config"),JSON_APPL_CONFIG_FILE);
				} 
				Appl.writeConfigTo(*pPayload,false);
				sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
			} 
		}
		else if (strCommand.equalsIgnoreCase(F("restorebackup"))) 
		{
			if(isAuthenticated) { // To ensure - only if authenticated...
				JsonNode * pCfgData = oJsonRequest.getObject("payload");
				ApplLogInfo("WS: Restoring config from backup...");
				DEBUG_JSON_OBJ((*pCfgData));
				oFS.saveJsonContentToFile(JSON_APPL_CONFIG_FILE,*pCfgData);
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,nullptr,0);
			}
		}
		else if (strCommand.equalsIgnoreCase(F("factoryreset")))
		{
			if(isAuthenticated) { 
				ApplLogInfo("WS: Restoring factory settings...");
				oFS.deleteFile("/config.json");
				Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,nullptr,0);
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
	bool bResult = true;
    
    // We should always get a JSON object (stringfied) from browser, so parse it
	// JSON_DOC(oXChangeDoc,DEFAULT_REQUEST_DOC_SIZE);

	JsonNode oXChangeDoc;
	// DynamicJsonDocument oXChangeDoc(DEFAULT_REQUEST_DOC_SIZE);
	// AsyncWebSocket       *pSocket = pMessage->pSocket;
	// AsyncWebSocketClient *pClient = pMessage->pClient;
	// cast to const char * to avoid in-place editing of serializedMessage
	// auto error = deserializeJson(oXChangeDoc, (const char *)pMessage->pSerializedMessage);
	const char *psz = oXChangeDoc.parse((const char *)pMessage->pSerializedMessage);
    if(*psz != '\0') {
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
