#include <gtest/gtest.h>
#include <string.h>
#include <Security.cpp>


// Test Base 64 encoding
TEST(Security,testBase64EncodeSimpleString) {
    String input = "Hello";
    String shouldBe = "SGVsbG8=";
    String result = getBase64EncodedString(input);
    EXPECT_TRUE(result.length() > 0);
    EXPECT_NE(result.c_str(),nullptr);
}

// Test Base64 decoding
TEST(Security,testBase64DecodeSimpleString) {
    String input = "Hello";
    String encoded = getBase64EncodedString(input).c_str();
    String decoded = getBase64DecodedString(encoded).c_str();
    EXPECT_STREQ(input.c_str(),decoded.c_str());
}


// Test Base64 with char array
TEST(Security,testBase64EncodeCharArray) {
    const char * data = "TestData";
    String result = getBase64EncodedString(data, strlen(data));
    EXPECT_TRUE(result.length() > 0);
}

// Test PKCS7 padding insert
TEST(Security,testPkcsPpaddingInsert) {
    byte data[32] = {0};
    insertPkcs7Padding(data, 10, 16);
    EXPECT_EQ(6, data[10]);
    EXPECT_EQ(6, data[15]);
}

// Test PKCS7 padding remove
TEST(Security,testPkcs7PaddingRemove) {
    byte data[32] = {0};
    size_t length = 16;
    for(int i = 10; i < 16; i++) data[i] = 6;
    removePkcs7Padding(data, length, 16);
    EXPECT_EQ((size_t) 10, length);
}

// Test random numbers initialization
TEST(Security,testRandomNumbersInitialized) {
    initializeRandomNumbers(true);
    EXPECT_TRUE(_bRandomNumbersInitialized);
}

// Test random numbers generation
TEST(Security,testGetRandomNumbers) {
    int nBuffLen = 16;
    byte buffer[nBuffLen];
    memset(buffer,'\0',sizeof(buffer));
    String result = getRandomNumbers(buffer, nBuffLen);
    EXPECT_TRUE(result.length() > 0);
    int nNotNulls = 0;
    for(int nIdx = 0; nIdx < nBuffLen; nIdx++) {
        if(buffer[nIdx] != 0) nNotNulls++;
    }
    EXPECT_GT(nNotNulls,10);
}

// Test encryption and decryption roundtrip
TEST(Security,testEncryptDecryptRoundtrip) {
    String originalData = "SecretMessage";
    uint8_t iv[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

    String encrypted = encryptDataToBase64(originalData.c_str(), iv);
    EXPECT_TRUE(encrypted.length() > 0);
    
    String decrypted = decryptDataFromBase64(encrypted.c_str(), iv);
    EXPECT_STREQ(originalData.c_str(), decrypted.c_str());
}

// Test convert string to hex
TEST(Security,testConvertStringToArray) {
    String input = "ABC";
    uint8_t hexArray[5] = {0};
    convertStringToArray(input, hexArray, 5);
    EXPECT_EQ('A', hexArray[0]);
    EXPECT_EQ('B', hexArray[1]);
    EXPECT_EQ('C', hexArray[2]);
}



// Test token key retrieval
TEST(Security,testGetTokenKey) {
    String key = getTokenKey();
    EXPECT_EQ((unsigned int) 16, key.length());
}

// Test new auth token generation
TEST(Security,testGetNewAuthToken) {
    String clientIP = "192.168.1.100";
    String token = getNewAuthToken(clientIP);
    EXPECT_GT(token.length(),(unsigned int) 0);
}

// Test decrypted token to json doc
TEST(Security,testWriteDecryptedTokenToJsonDoc) {
    String clientIP = "192.168.1.100";
    String token = getNewAuthToken(clientIP);
    JSON_DOC_STATIC(oDoc, JSON_DOC_TOKEN_SIZE);
    
    bool result = writeDecryptedTokenToJsonDoc(token, oDoc);
    EXPECT_TRUE(result);
    EXPECT_STREQ(clientIP.c_str(), oDoc["IP"].as<String>().c_str());
}

// Test auth token validation
TEST(Security,testIsAuthTokenValid) {
    String clientIP = "192.168.1.100";
    String token = getNewAuthToken(clientIP);
    
    bool isValid = isAuthTokenValid(token, clientIP);
    EXPECT_TRUE(isValid);
}

// Test auth token invalid with wrong IP
TEST(Security,testIsAuthTokenInvalidWrongIp) {
    String clientIP = "192.168.1.100";
    String wrongIP = "192.168.1.101";
    String token = getNewAuthToken(clientIP);
    
    bool isValid = isAuthTokenValid(token, wrongIP);
    EXPECT_FALSE(isValid);
}

// Test auth token invalid with empty token
TEST(Security,testIsAuthTokenInvalidEmptyToken) {
    String clientIP = "192.168.1.100";
    String emptyToken = "";
    
    bool isValid = isAuthTokenValid(emptyToken, clientIP);
    EXPECT_FALSE(isValid);
}
/*
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_base64_encode_simple_string);
    RUN_TEST(test_base64_decode_simple_string);
    RUN_TEST(test_base64_encode_char_array);
    RUN_TEST(test_pkcs7_padding_insert);
    RUN_TEST(test_pkcs7_padding_remove);
    RUN_TEST(test_random_numbers_initialized);
    RUN_TEST(test_get_random_numbers);
    RUN_TEST(test_convert_string_to_hex);
    RUN_TEST(test_encrypt_decrypt_roundtrip);
    RUN_TEST(test_get_token_key);
    RUN_TEST(test_get_new_auth_token);
    RUN_TEST(test_write_decrypted_token_to_json_doc);
    RUN_TEST(test_is_auth_token_valid);
    RUN_TEST(test_is_auth_token_invalid_wrong_ip);
    RUN_TEST(test_is_auth_token_invalid_empty_token);
    UNITY_END();
}

void loop() {}

*/