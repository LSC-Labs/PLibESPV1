#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <AESCryptor.h>


TEST(CAESToken,testCreateRandom) {
    CAESToken token;
    EXPECT_NE(token.Data[0],'\0');
}

TEST(CAESToken,testGetAsString) {
    CAESToken token;
    const char* result = token.getAsString();
    EXPECT_NE(result,nullptr);
    EXPECT_NE('\0',*result);
}

TEST(CAESToken,testGetAsBase64) {
    CAESToken token;
    const char* result = token.getAsBase64();
    EXPECT_NE(result,nullptr);
    EXPECT_NE('\0',*result);
}

TEST(CAESToken,testLoadFromString) {
    CAESToken token;
    token.loadFromString("TestPassphrase123");
    // As block size is 16, the string is truncated...
    EXPECT_STREQ("TestPassphrase12", token.getAsString());
}

TEST(CAESToken,testLoadFromBase64) {
    CAESToken token;
    token.loadFromString("1234567890123456");
    const char* base64 = token.getAsBase64();
    
    CAESToken token2;
    token2.loadFromBase64(base64);
    EXPECT_STREQ(token.getAsString(),token2.getAsString());
}

class CAESCryptorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize before each test
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

TEST_F(CAESCryptorTest,testEncryptDecrypt) {
    CAESCryptor aes;
    const char* passphrase = "TestPassphrase16";
    char plaintext[] = "Hello World!!!!!\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    char ciphertext[64] = {0};
    
    memcpy(ciphertext, plaintext, strlen(plaintext));
    
    size_t encLen = aes.encrypt(ciphertext, strlen(plaintext), passphrase);
    EXPECT_GT(encLen,(size_t)0);
    
    size_t decLen = aes.decrypt(ciphertext, encLen, passphrase);
    EXPECT_GT(decLen,(size_t)0);
    EXPECT_STREQ(plaintext, ciphertext);
}

TEST_F(CAESCryptorTest,testInsertPkcs7Padding) {
    CAESCryptor aes;
    byte data[32] = {0};
    memcpy(data, "Hello", 5);
    
    size_t result = aes.insertPkcs7Padding(data, 5);
    EXPECT_EQ((size_t) 16, result);
    EXPECT_EQ(11, data[5]);
}

TEST_F(CAESCryptorTest,testRemovePkcs7Padding) {
    CAESCryptor aes;
    byte data[32];
    memcpy(data, "Hello", 5);
    
    size_t nResult = aes.insertPkcs7Padding(data, 5);
    EXPECT_EQ((size_t) 16, nResult);
    EXPECT_EQ(11, data[5]);
    
    aes.removePkcs7Padding(data, nResult);
    EXPECT_EQ((size_t) 5, nResult);
    EXPECT_EQ(0, data[5]);
}

/*
void (*testFunctions[])(void) = {
    test_CAESToken_createRandom,
    test_CAESToken_getAsString,
    test_CAESToken_getAsBase64,
    test_CAESToken_loadFromString,
    test_CAESToken_loadFromBase64,
    test_CAESCryptor_encrypt_decrypt,
    test_insertPkcs7Padding,
    test_removePkcs7Padding
};

int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    for (size_t i = 0; i < sizeof(testFunctions) / sizeof(testFunctions[0]); i++) {
        RUN_TEST(testFunctions[i]);
    }
    
    return UNITY_END();
}

*/