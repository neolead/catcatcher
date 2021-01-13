#include "BLEDevice.h"
int Lampara = 33;
int Contador = 0;
int q = 0;
int f = 0;
static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;
bool deviceFound = false;
bool Encendida = false;
bool BotonOff = false;
String knownAddresses[] = { "ff:ff:30:02:09:6e"};
unsigned long entry;
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
}
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice Device){
      //Serial.print("BLE Advertised Device found: "); //
      //Serial.println(Device.toString().c_str());//
      pServerAddress = new BLEAddress(Device.getAddress()); 
      bool known = false;
      bool Master = false;
      for (int i = 0; i < (sizeof(knownAddresses) / sizeof(knownAddresses[0])); i++) {
        if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0) 
          known = true;
      }
      if (known) {
        Serial.print("CAT IS FOUND: ");
        Serial.println(Device.getRSSI());
        if (Device.getRSSI() > -87) {
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
void setup() {
  Serial.begin(115200);
  pinMode(Lampara,OUTPUT);
  digitalWrite(Lampara,LOW);
  BLEDevice::init("");
  pClient  = BLEDevice::createClient();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Done");
}
void Bluetooth() {
  Serial.println();
  Serial.println("BLE Scan restarted.....");
  deviceFound = false;
  BLEScanResults scanResults = pBLEScan->start(5);
  if (deviceFound) {
    if (f<10){    
    f=f+1;
    Serial.print("retry false positive cat exist:");
    Serial.println(f);
    }
    
    if (f>=10){   
      q=0;
      Serial.println("ON");
      Encendida = true;
      digitalWrite(Lampara,HIGH);
      Contador = 0;
      delay(1000);
    }
  }
  else{
    q=q+1;
    Serial.print("retry false negative - cat is gone - n:");
    Serial.println(q);
    if (q==6){
    Serial.println("OFF");
    digitalWrite(Lampara,LOW);
    delay(1000);
    q=0;
    f=0;
    }
    
  }
}
void loop() { 
  Bluetooth();
}
