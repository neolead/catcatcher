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
const char* defaultrssi = "-67";
const char* ssid = "feed";
const char* password = "feedfeed";
const char* PARAM_INT = "inputInt";
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
    Enter distance to block .<br>(-57 very closer -65 very far)<br> (current value %inputInt%): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path) {
  if (debug == true){Serial.printf("Reading file: %s\r\n", path);}
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    if (debug == true){Serial.println(F("- empty file or failed to open file"));}
    return String();
  }
  if (debug == true){Serial.println(F("- read from file:"));}
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  if (debug == true){Serial.println(fileContent);}
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  if (debug == true){Serial.printf("Writing file: %s\r\n", path);}
  delay(150);
  File file = fs.open(path, "w");
  if (!file) {
    if (debug == true){Serial.println(F("- failed to open file for writing"));}
    return;
  }
  if (file.print(message)) {
    if (debug == true){Serial.println(F("- file written"));}
  } else {
    if (debug == true){Serial.println(F("- write failed"));}
  }
}

String processor(const String& var) {
  if (var == "inputInt") {
    return readFile(SPIFFS, "/inputInt.txt");
  }
  return String();
}


int Lampara = 33;
int alr = 0;
int Contador = 0;
int q = 0;
int f = 0;
int motorpin1 = 26;                  //define digital output pin no.
int motorpin2 = 27;                  //define digital output pin no.
int motoren = 25;                  //define digital output pin no.
int relaypin = 32;
static BLEAddress *pServerAddress;
static boolean Verbinde = false;   // Phasen
static boolean Verbunden = false;
static boolean Suche = true;
BLEScan* pBLEScan;
BLEClient*  pClient;
bool deviceFound = false;
bool Encendida = false;
bool BotonOff = false;
String knownAddresses[] = { "88:4a:ea:3b:65:6f"};
unsigned long entry;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  if (debug == true){Serial.print("Notify callback for characteristic ");}
  if (debug == true){Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());}
  if (debug == true){Serial.print(" of data length ");}
  if (debug == true){Serial.println(F(length));}
}
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      if (debug == true){Serial.println(F("Verbunden"));}
      Verbunden = true;
      Verbinde = false;
    }

    void onDisconnect(BLEClient* pclient) {
      Verbunden = false;
      Verbinde = false;
      Suche = true;
      if (debug == true){Serial.println(F("Verbindung verloren"));}
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice Device) {
      pServerAddress = new BLEAddress(Device.getAddress());
      bool known = false;
      bool Master = false;
      for (int i = 0; i < (sizeof(knownAddresses) / sizeof(knownAddresses[0])); i++) {
        if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0)
          known = true;
      }
      if (known) {
        if (debug == true){Serial.print(F("CAT IS FOUND: "));}
        if (debug == true){Serial.println(F(Device.getRSSI()));}
        if (Device.getRSSI() > yourInputInt) {
          deviceFound = true;
        }
        else {
          deviceFound = false;
        }
        Device.getScan()->stop();
        delay(100);
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

void setup() {
  setCpuFrequencyMhz(80); //Set CPU clock to 80MHz fo example
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    if (debug == true){Serial.println(F("An Error has occurred while mounting SPIFFS"));}
    return;
  }
  WiFi.softAP(ssid, password);
  if (debug == true){Serial.println(F("IP Address: "));}
  if (debug == true){ Serial.println(WiFi.localIP() ) ;}
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
        ch = -67;
      }
      if (ch > -30) {
        ch = -67;
      }
      
      String wr = String(ch, DEC);
      //writeFile(SPIFFS, "/inputInt.txt", inputMessage.c_str());
      writeFile(SPIFFS, "/inputInt.txt", wr.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    if (debug == true){Serial.println(inputMessage);}
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
  Serial.println(F("we sleep 85"));
  delay(85000);
  WiFi.mode(WIFI_OFF);


  for (int i = 0; i <= 3; i++) {
    digitalWrite(relaypin, HIGH);
    delay (200);
    digitalWrite(relaypin, LOW);
    delay (200);
  }

  BLEDevice::init("");
  pClient  = BLEDevice::createClient();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  if (debug == true){Serial.println(F("Done"));}

}

void Bluetooth() {
  Serial.println(F("BLE Scan restarted....."));
  deviceFound = false;
  delay (500);
  BLEScanResults scanResults = pBLEScan->start(5);
  delay (200);
  if (deviceFound) {
    if (f < 7) {
      f = f + 1;
      if (debug == true){
        Serial.print(F("retry false positive cat exist:"));
        Serial.print(f);
        }
    }

    if (f >= 6) {

      if (alr == 1) {
        if (debug == true){Serial.println(F("do nothing"));}
        delay(1000);

      }
      if (alr == 0) {
        q = 0;
        if (debug == true){Serial.println(F("ON"));}
        Encendida = true;
        Backw(25000);
        alr = 1;

        Contador = 0;
        delay(1000);
      }
    }
  }
  else {
    if (alr == 1) {
      q = q + 1;
      if (debug == true){Serial.println(F("retry false negative - cat is gone - n:"));}
      if (debug == true){Serial.println(F(q));}
      if (q == 6) {
        if (debug == true){Serial.println(F("OFF"));}
        //    digitalWrite(Lampara,LOW);
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
  yourInputInt = readFile(SPIFFS, "/inputInt.txt").toInt();
  if ( yourInputInt == NULL) {
    yourInputInt = -67;
    writeFile(SPIFFS, "/inputInt.txt", defaultrssi);
  }
  if ( yourInputInt < -120 ) {
    yourInputInt = -67;
    writeFile(SPIFFS, "/inputInt.txt", defaultrssi);
  }
  if ( yourInputInt > -30 ) {
    yourInputInt = -67;
    writeFile(SPIFFS, "/inputInt.txt", defaultrssi);
  }
  delay(100);
  if (debug == true){Serial.print(F("*** Your inputInt: "));}
  if (debug == true){Serial.println(F(yourInputInt));}
  Bluetooth();

}
