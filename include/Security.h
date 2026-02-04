#pragma once

#include <Arduino.h>
#include <Network.h>
 
String getBase64DecodedString(String &strBase64String);
String getBase64EncodedString(String &strString);
String getBase64EncodedString(const char *tArray, int nSizeOfArray);
void setNewAuthHeader(AsyncWebServerRequest *pRequest, AsyncWebServerResponse *pResponse);
String getNewAuthToken(String &strIPAddress);
bool isAuthTokenValid(String &strEncBase64Token, String &strRemoteIPAddress);