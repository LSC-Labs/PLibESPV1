#include <Arduino.h>
#include <Vars.h>
#include <LittleFS.h>
#include <Logging.h>

// put function declarations here:
int myFunction(int, int);
CVarTable oVarTable;

void setup() {
  Serial.begin(115200);
  Serial.print("Var - Tests with JSON Version: ");
  Serial.println(ARDUINOJSON_VERSION );
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  Serial.println(" - setting vars...");
  oVarTable.set("Version","1.0.0");
  oVarTable.set("Auth",F("P. Liebl"));
  oVarTable.set("Password","geheim")->setCriticalVar(true);
  oVarTable.set("Millis",millis());


  Serial.println(" - writing to json obj");

  #if ARDUINOJSON_VERSION_MAJOR < 7
    StaticJsonDocument<500> oDoc;
  #else
    JsonDocument oDoc;
  #endif
  JsonObject oObj = oDoc.to<JsonObject>();
  oVarTable.writeConfigTo(oObj,true);
  serializeJsonPretty(oObj, Serial);

  CSerialLogWriter oLogWriter;
  oLogWriter.setLogLevel(9);
  oLogWriter.writeLogEntry("I", "Test Log Entry 1");
  oLogWriter.writeLogEntry("V",&oDoc);

  Serial.println(" --- done ---");
}

void loop() {

  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}