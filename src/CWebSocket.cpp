#ifndef DEBUG_LSC_WEBSOCKET
    #undef DEBUGINFOS
#endif

#include <Appl.h>
#include <WebSocket.h>
#include <Network.h>
#include <Security.h>
#include <FileSystem.h>

#if ARDUINOJSON_VERSION_MAJOR < 7
    #define DEFAULT_REQUEST_DOC_SIZE  2048
    #define DEFAULT_RESPONSE_DOC_SIZE 2048
    #define STATIC_RESPONSE_DOC_SIZE  1024
    #define JDOC_REQUEST(s) s = DynamicJsonDocument(DEFAULT_REQUEST_DOC_SIZE)

    #define JSONDOC_REQUEST(s)  DynamicJsonDocument s(DEFAULT_REQUEST_DOC_SIZE)
    #define JSONDOC_RESPONSE(s) DynamicJsonDocument s(DEFAULT_RESPONSE_DOC_SIZE)
#endif

#define JSONDOC_REQUEST(s)  JsonDocument s
#define JSONDOC_RESPONSE(s) JsonDocument s

// DynamicJsonDocument oXChangeDoc(DEFAULT_REQUEST_DOC_SIZE);


/// @brief Constructor - register the onEvent Handler...
///        Binds the onWebSocketEvent Handler to the basic WebSocket object.
/// @param strSocketName 
CWebSocket::CWebSocket(const char* strSocketName) : AsyncWebSocket(strSocketName){
	DEBUG_FUNC_START_PARMS("%s",NULL_POINTER_STRING(strSocketName));
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
}

/// @brief Catch the message from WebSocket and write the received data to the message queue
/// To process the queue, you have to dispatch the queue on a regular base (in loop)
/// otherwise, the queue will run out of memory...
/// @param pSocket	
/// @param pClient 
/// @param eType 
/// @param arg 
/// @param pData 
/// @param nLen 
void CWebSocket::onWebSocketEvent(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, AwsEventType eType, void *arg, uint8_t *pData, size_t nLen)
{
	DEBUG_FUNC_START_PARMS("..,..,%d,..,..", eType);

	if (eType == WS_EVT_ERROR)
	{
		DEBUG_INFOS(" - WS_EVT_ERROR : (%d)", *((uint16_t *)arg));
		ApplLogWarnWithParms("WebSocket[%s][%u] error(%u): %s\r\n", pSocket->url(), pClient->id(), *((uint16_t *)arg), (char *)pData);
	} 
	else if (eType == WS_EVT_DATA) {
		AwsFrameInfo *pFrameInfo = (AwsFrameInfo *)arg;
		// Notice until doc is found....
		DEBUG_INFOS("WS - FRAME(final = %d, index = %lld, len = %d (of %lld))",
					pFrameInfo->final,pFrameInfo->index,nLen, pFrameInfo->len);

		if (pFrameInfo->final && pFrameInfo->index == 0 && pFrameInfo->len == nLen)
		{
			// the whole message is in a single frame and we got all of it's data
			pClient->_tempObject = malloc(pFrameInfo->len);
			memcpy((uint8_t *)(pClient->_tempObject), pData, nLen);
			addMessageToQueue(pSocket, pClient, pFrameInfo->len);
		}
		else {
			DEBUG_INFO("WS - EVT_DATA : (Multiple Message Frames)");
			// message is comprised of multiple frames or the frame is split into multiple packets
			if (pFrameInfo->index == 0)
			{
				if (pFrameInfo->num == 0 && pClient->_tempObject == NULL)
				{
					pClient->_tempObject = malloc(pFrameInfo->len);
				}
			}
			if (pClient->_tempObject != NULL)
			{
				memcpy((uint8_t *)(pClient->_tempObject) + pFrameInfo->index, pData, nLen);
			}
			if ((pFrameInfo->index + nLen) == pFrameInfo->len)
			{
				if (pFrameInfo->final)
				{
					addMessageToQueue(pSocket, pClient, pFrameInfo->len );
				}
			}
		}
	}
}



/// @brief send a Json doc message to a client, or if not defined - to all
/// @param oDoc 	// Document to be sent
/// @param pSocket 	// Socket to handle the communication
/// @param pClient  // Specific client to talk to.
void ICACHE_FLASH_ATTR CWebSocket::sendJsonDocMessage(JsonDocument &oDoc, AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient) {
	DEBUG_FUNC_START();
	// If no socket is in place, use your own socket...
	if(!pSocket) pSocket = this;

    // Serialize the message to be sent...
	size_t nSize = measureJson(oDoc) * 2;
	DEBUG_INFOS("WS: - allocating buffer(%u bytes)",nSize);
	char *pszMessageBuffer = (char *) malloc(nSize);
	memset(pszMessageBuffer,'\0',nSize);

	int nFinalSize = serializeJson(oDoc,pszMessageBuffer,nSize);
	ApplLogVerboseWithParms("WS: - sending (%d) bytes of data: %s",nFinalSize, pszMessageBuffer);

	if(pClient) pClient->text(pszMessageBuffer);
	else pSocket->textAll(pszMessageBuffer);
	free(pszMessageBuffer);
	DEBUG_FUNC_END();
}

/// @brief send an "ERROR: Access Denied (401)" Message to a specific client...
/// @param oDoc Container will be filled with the message
/// @param pClient Client to send to...
/// @return 
void ICACHE_FLASH_ATTR CWebSocket::sendAccessDeniedMessage(JsonDocument &oRequestDoc,AsyncWebSocketClient *pClient) {
	if(pClient) {
		// Remember the requested command / type so it can be inserted in the message again.
		String strCommand = oRequestDoc["command"];
        String strType    = oRequestDoc["type"];

		oRequestDoc.clear();
		JsonObject oPayload = createPayloadStructure(F("error"),F("401"),oRequestDoc);
		oRequestDoc["AccessToken"] 	= F("notValid");		// destroy an existing access token
		oPayload["msg"] 			= F("access denied");	// set message for user...
		oPayload["command"] 		= strCommand;			// command that is not allowed
		oPayload["type"]    		= strType;				// type that is not allowed 
		sendJsonDocMessage(oRequestDoc,nullptr,pClient);
	}
}

void ICACHE_FLASH_ATTR CWebSocket::sendStatus(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient)
{
    JSONDOC_RESPONSE(oStatusDoc);
	JsonObject oStatusObj = createPayloadStructure(F("update"),F("status"),oStatusDoc);
	Appl.writeStatusTo(oStatusObj);
	sendJsonDocMessage(oStatusDoc,pSocket,pClient);	
}

JsonObject CWebSocket::createPayloadStructure(const char* pszCommand, const char *pszDataType, JsonDocument &oPayloadDoc, const char *pszData) {
	DEBUG_FUNC_START_PARMS("%s,%s,...",NULL_POINTER_STRING(pszCommand),NULL_POINTER_STRING(pszDataType));

	char pszBuffer[256];
	sprintf(pszBuffer,"{\"command\":\"%s\",\"data\":\"%s\"",pszCommand,pszDataType);
	String strData = pszBuffer;
	if(pszData) {
		strData += ",\"payload\":";
		strData += pszData;
	}
	strData += "}";
	DEBUG_INFO(" --> creating payload:");
	DEBUG_INFOS("%s",strData.c_str());
	DeserializationError error = deserializeJson(oPayloadDoc,strData);
	if(error) {
		ApplLogErrorWithParms("WS: Error creating payload structure : %s",error.c_str());
	} 
	DEBUG_FUNC_END();
	return(pszData == nullptr ? CreateJsonObject(oPayloadDoc,"payload") : oPayloadDoc["payload"]);
}

inline JsonObject CWebSocket::createPayloadStructure(const __FlashStringHelper* pszCommand, const __FlashStringHelper* pszDataType, JsonDocument &oPayloadDoc, const char *pszPayload) {
	return(createPayloadStructure((const char*) pszCommand,(const char*) pszDataType,oPayloadDoc,pszPayload));
}


bool inline CWebSocket::needsAuth(String &strCommand) {
	return(strNeedsAuth.indexOf(strCommand) > -1);
}

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


// messageSize needs to be one char bigger than the string to contain the string terminator
void CWebSocket::addMessageToQueue(AsyncWebSocket *pSocket, AsyncWebSocketClient *pClient, int nMessageSize)
{
	DEBUG_FUNC_START_PARMS("..,..,%d",nMessageSize);
	ApplLogTrace("WS: Adding message to queue....");
	WebSocketMessage *pNewMessage = new WebSocketMessage;
	// To be sure, to get always a zero terminated string, allocate + 1.
	pNewMessage->pSerializedMessage = (char *) malloc(nMessageSize + 1);
	memset(pNewMessage->pSerializedMessage,'\0',nMessageSize + 1);
	memcpy(pNewMessage->pSerializedMessage, (const char *)pClient->_tempObject, nMessageSize);
	// strlcpy(pNewMessage->serializedMessage, (const char *)pClient->_tempObject, nMessageSize +1);
	free(pClient->_tempObject);
	pClient->_tempObject = NULL;

	pNewMessage->pClient = pClient;
	pNewMessage->pSocket = pSocket;

	WebSocketMessage *lastMessage = pMsgQueue;
	// process only one message at the time
	if (lastMessage == NULL)
	{
		pMsgQueue = pNewMessage;
	}
	else
	{
		while (lastMessage->pNextMessage != NULL)
		{
			lastMessage = lastMessage->pNextMessage;
		}
		lastMessage->pNextMessage = pNewMessage;
	}
}

/// @brief Dispatch the message queue...
///  	   This function should be called in the main loop to process the message queue
///        Processes all messages in the queue and cleans up the memory
void CWebSocket::dispatchMessageQueue(funcDispatchMessage pFuncDispatchMessage)
{
	while (pMsgQueue != NULL)
	{
		DEBUG_INFOS("WS: dispatchMessageQueue()");
		WebSocketMessage *pMessageToProcess = CWebSocket::pMsgQueue;
		CWebSocket::pMsgQueue = pMessageToProcess->pNextMessage;
		// Process the message.
		// If not in default processing, use the application specific, if defined.
		if(!dispatchMessage(pMessageToProcess) && pFuncDispatchMessage != nullptr) {
			pFuncDispatchMessage(pMessageToProcess);
		};
		/// Clean up...
		free(pMessageToProcess->pSerializedMessage);
		free(pMessageToProcess);
	}
}

/**
 * Dispatch the WebSocket Message
 * Expecting always a JSON Object from client !
 */
bool CWebSocket::dispatchMessage( WebSocketMessage *pMessage) {
	DEBUG_FUNC_START();
	DEBUG_INFOS("WS: dispatching message : %s",pMessage->serializedMessage);
	bool bResult = true;
    CFS oFS;
    // We should always get a JSON object (stringfied) from browser, so parse it

    JSONDOC_REQUEST(oXChangeDoc);
	// DynamicJsonDocument oXChangeDoc(DEFAULT_REQUEST_DOC_SIZE);
	// AsyncWebSocket       *pSocket = pMessage->pSocket;
	// AsyncWebSocketClient *pClient = pMessage->pClient;
	// cast to const char * to avoid in-place editing of serializedMessage
	auto error = deserializeJson(oXChangeDoc, (const char *)pMessage->pSerializedMessage);
    if(error) {
        ApplLogErrorWithParms(F("WS: Parse WebSocket message to Json : %s"),error.c_str());
    } else {
        // Json Document is ready...
        // Web Browser sends some commands, check which command is given
	    String strCommand = oXChangeDoc["command"];
		strCommand.toLowerCase();
		ApplLogTraceWithParms(F("WS: dispatching command \"%s\""),strCommand.c_str());
		bool isAuthNeeded = needsAuth(strCommand);
		bool isAuthenticated = isAuthNeeded ? checkAuth(oXChangeDoc,pMessage->pClient) : false;
		DEBUG_INFOS("WS: Auth needed : %d, Authenticated : %d",isAuthNeeded,isAuthenticated);
		if(isAuthNeeded && !isAuthenticated) {
			ApplLogErrorWithParms("WS: Access denied - %s",strCommand.c_str());
			sendAccessDeniedMessage(oXChangeDoc,pMessage->pClient);
		} else {
			// bool isAuthenticated = bNeedsAuth ? isAuthTokenValid(strAuthToken, strClientRemoteIP) : true;
			if (strCommand.equalsIgnoreCase(F("getstatus")))
			{
				JsonObject oStatusNode = createPayloadStructure(F("update"),F("status"),oXChangeDoc);
				Appl.writeStatusTo(oStatusNode);
				sendJsonDocMessage(oXChangeDoc,pMessage->pSocket,pMessage->pClient);
			}
			else if (strCommand.equalsIgnoreCase(F("getconfig")))
			{
				if(isAuthenticated) { // To ensure - only if authenticated...
					JsonObject oCfgNode = createPayloadStructure(F("update"),F("config"),oXChangeDoc);
					Appl.writeConfigTo(oCfgNode,true);
					sendJsonDocMessage(oXChangeDoc,pMessage->pSocket,pMessage->pClient);
				}
			}
			else if (strCommand.equalsIgnoreCase(F("saveconfig")))
			{
				if(isAuthenticated) {
					JsonObject oPayload = oXChangeDoc["payload"];
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
                Appl.MsgBus.sendEvent(this,MSG_WIFI_SCAN,pMessage->pClient,0);
			}
			else if (strCommand.equalsIgnoreCase(F("scanrf433")))
			{
				Appl.MsgBus.sendEvent(this,MSG_SCAN_RF433,pMessage->pClient,0);
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
						createPayloadStructure(F("backup"),F("config"),oXChangeDoc,strData.c_str());
					} else {
						ApplLogWarnWithParms(F("WS: Config file %s not found, using current config"),JSON_CONFIG_DEFAULT_NAME);
						JsonObject oCfgNode = createPayloadStructure(F("backup"),F("config"),oXChangeDoc);
						Appl.writeConfigTo(oCfgNode,false);
					} 
					sendJsonDocMessage(oXChangeDoc,pMessage->pSocket,pMessage->pClient);
				} 
			}
			else if (strCommand.equalsIgnoreCase(F("restorebackup"))) 
			{
				if(isAuthenticated) { // To ensure - only if authenticated...
					JsonObject oCfgData = oXChangeDoc["payload"];
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
					oFS.deleteAllFilesOnPath("/");
					Appl.MsgBus.sendEvent(this,MSG_REBOOT_REQUEST,nullptr,0);
				}
			}
			else bResult = false;
		}
    }
	DEBUG_FUNC_END_PARMS("%d",bResult);
	return(bResult);
}


