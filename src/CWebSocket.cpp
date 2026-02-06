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
#include <FileSystem.h>
#include <JsonHelper.h>

#define DEFAULT_REQUEST_DOC_SIZE  2048
#define DEFAULT_RESPONSE_DOC_SIZE 2048

/**
 * @brief Constructor - register the onEvent Handler...
 *        Binds the onWebSocketEvent Handler to the basic WebSocket object.
 * @param strSocketName 
 * @param bRegisterOnMsgBus If true, the WebSocket will register itself on the application message bus.
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
	// if(bRegisterOnMsgBus) Appl.MsgBus.registerEventReceiver(this,__FUNCTION__);
	DEBUG_FUNC_END();
}

#pragma region Receiving Messages (Event / Socket)
/**
 * @brief Dispatch the message queue from the application object
 * To get this work, this object instance will register itself on the message bus of the application.
 * @see constructor CWebSocket
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

		case MSG_WEBSOCKET_SEND_JSON :
        case MSG_WIFI_SCAN_RESULT : if(pMessage != nullptr) {
										JsonDocument *pDoc = ( JsonDocument *) pMessage;
										sendJsonDocMessage(*pDoc);
									}
									break;
		default: break;
    }
    return(EVENT_MSG_RESULT_OK);
}

/** 
 * @brief Catch the message from WebSocket and write the received data to the message queue
 * To process the queue, you have to dispatch the queue on a regular base (in loop)
 * otherwise, the queue will run out of memory...
 * @param pSocket		// Socket (this)
 * @param pClient 		// Client, sending the message
 * @param eType			// Type of Message 
 * @param pArg 			// Error code or Frame Info - depends on eType 
 * @param pData 		// pointer to data received
 * @param nDataLen 		// Length of the current data (pData)
 * @see : https://github.com/ESP32Async/ESPAsyncWebServer/wiki#async-websocket-event
 * - Implementation of this side, seems to contain error in implmenting multi frames ? >> check and see below
 * */
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
		Serial.printf("============ New Socket Message =============\n");
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
			/* Debug version from the source page (diag the bug in index counting)
			// message is comprised of multiple frames or the frame is split into multiple packets
			DEBUG_INFO("WS: - EVT_DATA : (Multiple Message Frames)");
			if(pFrameInfo->index == 0){
        		if(pFrameInfo->num == 0) {
					
					Serial.printf("ws[%s][%u] %s-message start\n", 
									pSocket->url(), 
									pClient->id(), 
									(pFrameInfo->message_opcode == WS_TEXT)?"text":"binary");
				}
        		Serial.printf("ws[%s][%u] frame[%u] start(total len?)[%llu]\n", 
								pSocket->url(), 
								pClient->id(), 
								pFrameInfo->num, 
								pFrameInfo->len);
      		}

      		Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", 
							pSocket->url(), 
							pClient->id(), 
							pFrameInfo->num, 
							(pFrameInfo->message_opcode == WS_TEXT)?"text":"binary",
							 pFrameInfo->index, 
							 pFrameInfo->index + nDataLen);

      		if(pFrameInfo->message_opcode == WS_TEXT){
        		pData[nDataLen] = 0;
        		Serial.printf("%s\n", (char*)pData);
      		} else {
        		for(size_t i=0; i < nDataLen; i++){
          			Serial.printf("%02x ", pData[i]);
        		}
        	Serial.print("\n");
      		}

			if((pFrameInfo->index + nDataLen) == pFrameInfo->len) {
        		Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", pSocket->url(), pClient->id(), pFrameInfo->num, pFrameInfo->len);
        		if(pFrameInfo->final){
          			Serial.printf("ws[%s][%u] %s-message end\n", pSocket->url(), pClient->id(), (pFrameInfo->message_opcode == WS_TEXT)?"text":"binary");
					if(pFrameInfo->message_opcode == WS_TEXT)
						pClient->text("I got your text message");
					else
						pClient->binary("I got your binary message");
				}
      		}
			*/
			
			// First frame ? allocate the WebSocket message object in temp of client
			if(pFrameInfo->index == 0) {
				pClient->_tempObject = new CWebSocketMessage(pSocket,pClient,pFrameInfo->len,pFrameInfo->opcode);
			}
			// Move date into WebSocket message object...
			CWebSocketMessage * pMsgObj = (CWebSocketMessage *) pClient->_tempObject;
			if(pMsgObj) {
				pMsgObj->setMessageData(pData,pFrameInfo->index,nFrameDataLen);
			}

			// was it the final frame ? then store the data into the message queue...
			if(pFrameInfo->final && (pFrameInfo->index + nFrameDataLen) == pFrameInfo->len) {
				addMessageToQueue(pMsgObj);
			}
		}
	}
	DEBUG_FUNC_END();
}


/**
 * @brief Add a message to the internal message queue.
 * It should be processed outside the WebSocket event handler in main loop
 */
void CWebSocket::addMessageToQueue(CWebSocketMessage *pMsgObj)
{
	DEBUG_FUNC_START();
	if(pMsgObj) {
		m_tMsgQueue.push(pMsgObj);
		#ifdef DEBUGINFOS
			if(pMsgObj->MessageType == WS_TEXT) {
				DEBUG_INFO("WS: message received:");
				DEBUG_INFOS("%s",pMsgObj->pSerializedMessage);
			}
		#endif
	}

	DEBUG_FUNC_END();
}

#pragma endregion

#pragma region Message Sending

/** 
 * @brief send a Json doc message to a client, or if not defined - to all
 * @param oDoc 	Document to be sent
 * @param pSocket Socket to handle the communication - if not specified, use own socket.
 * @param pClient Specific client to talk to. (if set, the message will be sent only to this client)
 * @see  https://github.com/ESP32Async/ESPAsyncWebServer/wiki#asyncwebsocketmessagebuffer-and-makebuffer
 */
 void ICACHE_FLASH_ATTR CWebSocket::sendJsonDocMessage(JsonDocument &oDoc, AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient) {
	DEBUG_FUNC_START();
	// If no socket is in place, use your own socket...
	if(!pSocket) pSocket = this;

    // Serialize the message to be sent...
	size_t nSize = measureJson(oDoc);
	DEBUG_INFOS("WS: - allocating buffer(%u bytes)",nSize);
	AsyncWebSocketMessageBuffer* pBuffer = pSocket->makeBuffer(nSize);
	#ifdef DEBUG_LSC_WEBSOCKET
		assert(pBuffer);
	#endif
	serializeJson(oDoc,pBuffer->get(),nSize);
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
 * @brief send an "ERROR: Access Denied (401)" Message to a specific client...
 * @param oDoc Container will be filled with the message
 * @param pClient Client to send to...
 * */
void ICACHE_FLASH_ATTR CWebSocket::sendAccessDeniedMessage(JsonDocument &oRequestDoc,AsyncWebSocketClient *pClient) {
	if(pClient) {
		// Remember the requested command / type so it can be inserted in the message again.
		String strCommand = oRequestDoc["command"];
        String strType    = oRequestDoc["type"];

		oRequestDoc.clear();
		JsonObject oPayload = LSC::createPayloadStructure(F("error"),F("401"),oRequestDoc);
		oRequestDoc["AccessToken"] 	= F("notValid");		// destroy an existing access token
		oPayload["msg"] 			= F("access denied");	// set message for user...
		oPayload["command"] 		= strCommand;			// command that is not allowed
		oPayload["type"]    		= strType;				// type that is not allowed 
		sendJsonDocMessage(oRequestDoc,nullptr,pClient);
	}
}

#pragma endregion

#pragma region Authentication

/**
 * @brief Set the Needs Auth string
 * Be aware that this will overwrite any existing command list.
 * @param strCommands Comma delimited string containing the commands needing authentication.
 * @return The set string
 */
String CWebSocket::setNeedsAuth(const String &strCommands) {
	m_strNeedsAuth = strCommands;
	return m_strNeedsAuth;
};

/**
 * @brief Get the Needs Auth string
 * @return String containing the commands needing authentication, delimited by commas
 */
String CWebSocket::getNeedsAuth() {
	return m_strNeedsAuth;
};


/**
 * @brief Check if a command needs authentication
 * @param strCommand The command to be checked	
 * @return true if the command needs authentication, false otherwise
 */
bool inline CWebSocket::needsAuth(String &strCommand) {
	return(m_strNeedsAuth.indexOf(strCommand) > -1);
}

/**
 * @brief Check the authentication token in the Json Document is a valid token and matches the client IP
 * @param oJsonRequest The Json Document containing the request and the token
 * @param pClient The WebSocket Client requesting the command (needed to get the remote IP)
 * @return true if the token is valid, false otherwise
 */
bool CWebSocket::checkAuth(JsonDocument &oJsonRequest, AsyncWebSocketClient *pClient) {
	DEBUG_FUNC_START();
	bool isAuthenticated = false;
	String strAuthToken = oJsonRequest["token"];
	if(strAuthToken.length() > 10 && pClient) {
		String strClientRemoteIP = pClient->remoteIP().toString();
		isAuthenticated = isAuthTokenValid(strAuthToken, strClientRemoteIP);
	}
	DEBUG_FUNC_END_PARMS("%d",isAuthenticated);
	return(isAuthenticated);
}

#pragma endregion

#pragma region Message Dispatching

     
/**
 * @brief Dispatch the current message queue with the well known commands...
 * This function should be called in the main loop to process the message queue.
 * Calls dispatchMessage with the act message and cleans up the memory after processing.
 */
void CWebSocket::dispatchMessageQueue()
{
	while(!m_tMsgQueue.empty()) {
		CWebSocketMessage * pMessageToProcess = m_tMsgQueue.front();
		dispatchMessage(pMessageToProcess);
		free(pMessageToProcess->pSerializedMessage);
		free(pMessageToProcess);
		m_tMsgQueue.pop();
	}
}

/**
 * @brief Dispatch a JSON WebSocket Message
 * Override this function to implement your own message handling.
 * Expecting always a JSON Object from client !
 * @param oJsonRequest the JSON request document
 * @param pMessage the message to be processed
 * @return true if message command was recognized processed, false otherwise
 */
bool CWebSocket::dispatchJsonMessage(JsonDocument &oJsonRequest, CWebSocketMessage *pMessage) {
	DEBUG_FUNC_START();
	bool bResult = true;
	CFS oFS;
	// Json Document is ready...
	// Web Browser sends some commands, check which command is given
	String strCommand = oJsonRequest["command"];
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
		if (strCommand.equalsIgnoreCase(F("getstatus")))
		{
			JsonObject oStatusNode = LSC::createPayloadStructure(F("update"),F("status"),oJsonRequest);
			Appl.writeStatusTo(oStatusNode);
			sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
		}
		else if (strCommand.equalsIgnoreCase(F("getconfig")))
		{
			// NO authentication needed, cause critical informations are hidde (!)
			JsonObject oCfgNode = LSC::createPayloadStructure(F("update"),F("config"),oJsonRequest);
			Appl.writeConfigTo(oCfgNode,true);
			sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
		}
		else if (strCommand.equalsIgnoreCase(F("saveconfig")))
		{
			if(isAuthenticated) {
				JsonObject oPayload = oJsonRequest["payload"];
				// First load the config - to enable validation of settings (!)
				// then write the new config file to the file system
				// ... and ask for a reboot !
				Appl.readConfigFrom(oPayload);
				Appl.saveConfig();
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
				if(oFS.fileExists(JSON_CONFIG_DEFAULT_NAME)) {
					// loadJsonContentFromFile(JSON_CONFIG_DEFAULT_NAME,oXChangeDoc);
					String strData;
					oFS.loadFileToString(JSON_CONFIG_DEFAULT_NAME,strData);
					// deserializeJson(oXChangeDoc,strData);
					LSC::createPayloadStructure(F("backup"),F("config"),oJsonRequest,strData.c_str());
					// Now enrich with the current configuration...
					JsonObject oPayload = GetJsonObject(oJsonRequest,"payload");
					Appl.writeConfigTo(oPayload,false);
				} else {
					ApplLogWarnWithParms(F("WS: Config file %s not found, using current config"),JSON_CONFIG_DEFAULT_NAME);
					JsonObject oCfgNode = LSC::createPayloadStructure(F("backup"),F("config"),oJsonRequest);
					Appl.writeConfigTo(oCfgNode,false);
				} 
				sendJsonDocMessage(oJsonRequest,pMessage->pSocket,pMessage->pClient);
			} 
		}
		else if (strCommand.equalsIgnoreCase(F("restorebackup"))) 
		{
			if(isAuthenticated) { // To ensure - only if authenticated...
				JsonObject oCfgData = oJsonRequest["payload"];
				ApplLogInfo("WS: Restoring config from backup...");
				DEBUG_JSON_OBJ(oCfgData);
				oFS.saveJsonContentToFile(JSON_CONFIG_DEFAULT_NAME,oCfgData);
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
 * @brief Dispatch a single WebSocket Message
 * Override this function to implement your own message handling.
 * Expecting always a JSON Object from client !
 * @param pMessage the message to be processed
 * @return true if message command was recognized processed, false otherwise
 */
bool CWebSocket::dispatchMessage( CWebSocketMessage *pMessage) {
	DEBUG_FUNC_START();
	bool bResult = true;
    
    // We should always get a JSON object (stringfied) from browser, so parse it
	JSON_DOC(oXChangeDoc,DEFAULT_REQUEST_DOC_SIZE);
	// DynamicJsonDocument oXChangeDoc(DEFAULT_REQUEST_DOC_SIZE);
	// AsyncWebSocket       *pSocket = pMessage->pSocket;
	// AsyncWebSocketClient *pClient = pMessage->pClient;
	// cast to const char * to avoid in-place editing of serializedMessage
	auto error = deserializeJson(oXChangeDoc, (const char *)pMessage->pSerializedMessage);
    if(error) {
        ApplLogErrorWithParms(F("WS: Parse message : %s"),error.c_str());
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
