#include "BLEDevice.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "esp32-hal-cpu.h"
static boolean debug = false;
AsyncWebServer server(80);
int yourInputInt;
int blockInt;
int unblockInt;
const char* defaultrssi = "-69";
const char* defaultblock = "7";
const char* defaultunblock = "6";
const char* ssid = "feed";
const char* password = "feedfeed";
const char* PARAM_INT = "inputInt";
const char* PARAM2_INT = "blockInt";
const char* PARAM3_INT = "unblockInt";

int alr = 0;
int Contador = 0;
int q = 0;
int f = 0;
int motorpin1 = 26;                  //define digital output pin no.
int motorpin2 = 27;                  //define digital output pin no.
int motoren = 25;                  //define digital output pin no.
int relaypin = 32;
BLEClient* pClient;
static boolean Verbinde = false;   // Phasen
static boolean Verbunden = false;
static boolean Suche = true;
bool deviceNear = false;
bool Encendida = false;
bool BotonOff = false;
String knownAddresses[] = { "88:4a:ea:3b:65:6f"};
unsigned long entry;
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
  <form action="/get" target="hidden-form">
    Enter distance to block .<br>(-60 very closer -70 very far)<br> (current value %inputInt%): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Retry to block <br>(7 standart)<br> (current value %blockInt%): <input type="number " name="blockInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Retry to unblock <br>(6 standart)<br> (current value %unblockInt%): <input type="number " name="unblockInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path) {
  if (debug){Serial.printf("Reading file: %s\r\n", path);}
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    if (debug){Serial.println(F("- empty file or failed to open file"));}
    return String();
  }
  if (debug){Serial.println(F("- read from file:"));}
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  if (debug){Serial.println(fileContent);}

  file.close();
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  if (debug){Serial.printf("Writing file: %s\r\n", path);}
  delay(150);
  File file = fs.open(path, "w");
  if (!file) {
    if (debug){Serial.println(F("- failed to open file for writing"));}
    return;
  }
  if (file.print(message)) {
    if (debug){Serial.println(F("- file written"));}
  } else {
    if (debug){Serial.println(F("- write failed"));}
  }
}

String processor(const String& var) {
  if (var == "inputInt") {
    return readFile(SPIFFS, "/inputInt.txt");
  }
  else if (var == "blockInt") {
    return readFile(SPIFFS, "/block.txt");
  }
  else if (var == "unblockInt") {
    return readFile(SPIFFS, "/unblock.txt");
  }
  return String();
}



class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        Serial.println("Verbunden");
        Verbunden = true;
        Verbinde = false;
    }

    void onDisconnect(BLEClient* pclient) {
        Verbunden = false;
        Verbinde = false;
        Suche = true;
        Serial.println("Verbindung verloren");
    }
};


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device) override {
        BLEAddress pServerAddress = BLEAddress(device.getAddress());
        for (size_t i = 0; i < (sizeof(knownAddresses) / sizeof(knownAddresses[0])); i++) {
            bool deviceKnown = knownAddresses[i].equalsIgnoreCase(pServerAddress.toString().c_str());
            if (deviceKnown) {
                //deviceNear = device.getRSSI() > -76;
                deviceNear = device.getRSSI() > yourInputInt;
                if (deviceNear) {
                    Serial.printf("CAT IS FOUND (RSSI): %d\n", device.getRSSI());
                }
                if (!deviceNear) {
                    Serial.printf("CAT IS AWAY (RSSI): %d\n", device.getRSSI());
                }
                break;
            }
        }
    }
};


void Forw(int u) {
  digitalWrite(motoren, HIGH);
  digitalWrite(relaypin, HIGH);
  digitalWrite(motorpin1, HIGH);
  digitalWrite(motorpin2, LOW);
  delay(u) ;
  digitalWrite(motoren, LOW);
  digitalWrite(relaypin, LOW);
  digitalWrite(motorpin1, LOW);
  digitalWrite(motorpin2, LOW);

}
void Backw(int t) {
  digitalWrite(motoren, HIGH);
  digitalWrite(relaypin, HIGH);
  digitalWrite(motorpin1, LOW);
  digitalWrite(motorpin2, HIGH);
  delay(t) ;
  digitalWrite(motoren, LOW);
  digitalWrite(relaypin, LOW);
  digitalWrite(motorpin1, LOW);
  digitalWrite(motorpin2, LOW);
}
void inputx(){
  yourInputInt = readFile(SPIFFS, "/inputInt.txt").toInt();
  if ( yourInputInt == NULL) {
    yourInputInt = -69;
    writeFile(SPIFFS, "/inputInt.txt", defaultrssi);
  }
  if ( yourInputInt < -120 ) {
    yourInputInt = -69;
    writeFile(SPIFFS, "/inputInt.txt", defaultrssi);
  }
  if ( yourInputInt > -30 ) {
    yourInputInt = -69;
    writeFile(SPIFFS, "/inputInt.txt", defaultrssi);
  }
  
  if (debug) {
    Serial.printf("*** Your inputInt: %d\n", yourInputInt);
  };

  blockInt = readFile(SPIFFS, "/block.txt").toInt();
  if ( blockInt == NULL) {
    blockInt = 7;
    writeFile(SPIFFS, "/block.txt", defaultblock);
  }
  if ( blockInt < 1 ) {
    blockInt = 7;
    writeFile(SPIFFS, "/block.txt", defaultblock);
  }
  if ( blockInt > 20 ) {
    blockInt = 7;
    writeFile(SPIFFS, "/block.txt", defaultblock);
  }
  
  if (debug) {
    Serial.printf("*** Your block value: %d\n", blockInt);
  };
  
  unblockInt = readFile(SPIFFS, "/unblock.txt").toInt();
  if ( unblockInt == NULL) {
    unblockInt = 6;
    writeFile(SPIFFS, "/unblock.txt", defaultunblock);
  }
  if ( unblockInt < 1 ) {
    unblockInt = 6;
    writeFile(SPIFFS, "/unblock.txt", defaultunblock);
  }
  if ( unblockInt > 20 ) {
    unblockInt = 6;
    writeFile(SPIFFS, "/unblock.txt", defaultunblock);
  }
  
  if (debug) {
    Serial.printf("*** Your unblock value: %d\n", unblockInt);
  };


  
}

void setup() {
   setCpuFrequencyMhz(80); //Set CPU clock to 80MHz fo example
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    if (debug){Serial.println(F("An Error has occurred while mounting SPIFFS"));}
    return;
  }
  inputx();
  WiFi.softAP(ssid, password);
  if (debug){Serial.println(F("IP Address: "));}
  if (debug){ Serial.println(WiFi.localIP() ) ;}
  pinMode(motorpin1, OUTPUT);
  pinMode(motorpin2, OUTPUT);
  pinMode(motoren, OUTPUT);
  pinMode(relaypin, OUTPUT);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;

    if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      //long ch = inputMessage.toInt;
      
      int ch = inputMessage.toInt();
      if (ch  < -120) {
        ch = -69;
      }
      if (ch > -30) {
        ch = -69;
      }
      String wr = String(ch, DEC);
      writeFile(SPIFFS, "/inputInt.txt", wr.c_str());
    }

    else if (request->hasParam(PARAM2_INT)) {
      inputMessage = request->getParam(PARAM2_INT)->value();
      //long ch = inputMessage.toInt;
      
      int ch = inputMessage.toInt();
      if (ch  < 1) {
        ch = 4;
      }
      if (ch > 20) {
        ch = 4;
      }
      String wr = String(ch, DEC);
      writeFile(SPIFFS, "/block.txt", wr.c_str());
    }
    else if (request->hasParam(PARAM3_INT)) {
      inputMessage = request->getParam(PARAM3_INT)->value();
      //long ch = inputMessage.toInt;
      
      int ch = inputMessage.toInt();
      if (ch  < 1) {
        ch = 4;
      }
      if (ch > 20) {
        ch = 4;
      }
      String wr = String(ch, DEC);
      writeFile(SPIFFS, "/unblock.txt", wr.c_str());
    }

    else {
      inputMessage = "No message sent";
    }
    if (debug){Serial.println(inputMessage);}
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
  delay(2000);
  Serial.println(F("We sleep 85 sec, wait for feeder init"));
  delay(85000);
  //WiFi.mode(WIFI_OFF);


  for (int i = 0; i <= 3; i++) {
    digitalWrite(relaypin, HIGH);
    delay (200);
    digitalWrite(relaypin, LOW);
    delay (200);
  }

    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    auto scan = BLEDevice::getScan(); 
    scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    scan->setActiveScan(true);
    Serial.println("Done");
}
void CheckResults() {
    Serial.println();
    Serial.println("BLE Scan restarted.....");
    delay(500);
   
    if (deviceNear) {
      if (blockInt<=1){blockInt=2};
        if (f < blockInt) {
            f = f + 1;
            Serial.print("retry false positive cat exist:");
            Serial.println(f);
        }
        if (blockInt<=1){blockInt=2};
        if (f >= (blockInt - 1)) {
            if (alr == 1) {
                Serial.println("do nothing");
                delay(1000);
            }
            if (alr == 0) {
                q = 0;
                Serial.println("ON");
                Encendida = true;
                Backw(25000);
                alr = 1;

                Contador = 0;
                delay(1000);
            }
        }
    } else {
        if (alr == 1) {
            q = q + 1;
            Serial.print("retry false negative - cat is gone - n:");
            Serial.println(q);
            if (q == unblockInt) {
                Serial.println("OFF");
                Forw(25000);
                delay(1000);
                q = 0;
                f = 0;
                alr = 0;
            }
        }
    }
}


void loop() {
  delay(100);
  Serial.printf("Free heap: %d\n", esp_get_free_heap_size());
  inputx();
  BLEDevice::getScan()->start(5);
  CheckResults();

}
