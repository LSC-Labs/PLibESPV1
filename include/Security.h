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
