#pragma once
#include <Arduino.h>
#include <Network.h>

// ****************************************************
// The Application security token password
// ... only the first 16 bytes are used (!)
// ****************************************************
#ifndef APPL_SECURITY_TOKEN_PASS
    #define APPL_SECURITY_TOKEN_PASS "Kis8%$vvQ@ä+qw12"
#endif

// ****************************************************
// Unique Key - embeded into the security token
// ****************************************************
#ifndef APPL_SECURITY_TOKEN_KEY
    #define APPL_SECURITY_TOKEN_KEY "ds$woEir=ncn<jek"
#endif

/*
String getBase64DecodedString(String &strBase64String);
String getBase64EncodedString(String &strString);
String getBase64EncodedString(const char *tArray, int nSizeOfArray);
void setNewAuthHeader(AsyncWebServerRequest *pRequest, AsyncWebServerResponse *pResponse);
String getNewAuthToken(String &strIPAddress);
bool isAuthTokenValid(String &strEncBase64Token, String &strRemoteIPAddress);
*/