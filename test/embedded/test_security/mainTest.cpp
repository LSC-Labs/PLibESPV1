
#include <gtest/gtest.h>
// uncomment line below if you plan to use GMock
// #include <gmock/gmock.h>

// Find more details to google test:
// https://google.github.io/googletest/primer.html

// The library sources (src/) are built with the tests (platformio.ini: test_build_src = yes)


#if defined(ARDUINO)
    #include <Arduino.h>

    void setup(){
        Serial.begin(115200);
        Serial.println("Running tests....");
        delay(2000);
        testing::InitGoogleTest();

        // if you plan to use GMock, replace the line above with
        // ::testing::InitGoogleMock();

        if(!RUN_ALL_TESTS()){
            Serial.println("Test failures Ignored:PASS...");
        } else {
            Serial.println("Test failures Ignored:FAIL...");

        } 
    }

    void loop() {

    }

#else
    int main(int argc, char **argv)
    {
        ::testing::InitGoogleTest(&argc, argv);
        // if you plan to use GMock, replace the line above with
        // ::testing::InitGoogleMock(&argc, argv);

        if (RUN_ALL_TESTS())
        ;

        // Always return zero-code and allow PlatformIO to parse results
        return 0;
    }
#endif