#include <SPI.h>
#include <SD.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

const int SD_CS = 10;
File fileHandle;
bool receivingFile = false;

// BLE globals
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// UUIDs
#define SERVICE_UUID        "19B10000-E8F2-537E-4F6C-D104768A1214"
#define CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"

// Maximum MTU negotiation (set elsewhere if you like)
static const uint16_t REQUESTED_MTU = 247;

// BLE server callbacks
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer)   { deviceConnected = true;  Serial.println("BLE: Client connected"); }
  void onDisconnect(BLEServer* pServer){ deviceConnected = false; Serial.println("BLE: Client disconnected"); }
};

// BLE characteristic write callback
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *chr) {
    std::string data = chr->getValue();
    if (data.empty()) return;
    String s((char*)data.data(), data.size());

    // === File transfer start marker [FILE_START:fname.ext] ===
    if (s.startsWith("[FILE_START:") && s.endsWith("]")) {
      int c = s.indexOf(':');
      int b = s.lastIndexOf(']');
      String fname = s.substring(c + 1, b);
      String path = "/" + fname;
      if (fileHandle) fileHandle.close();
      fileHandle = SD.open(path, FILE_WRITE);
      if (!fileHandle) {
        Serial.print("SD: Failed to open "); Serial.println(path);
      } else {
        receivingFile = true;
        Serial.print("SD: Begin file → "); Serial.println(path);
      }
    }
    // === File transfer end marker ===
    else if (s == "[FILE_END]") {
      if (receivingFile) {
        fileHandle.close();
        receivingFile = false;
        Serial.println("SD: File transfer complete");
      }
    }
    // === Directory listing request ===
    else if (s == "[LIST]") {
      Serial.println("SD: Listing directory on request");
      // Inform start
      pCharacteristic->setValue("[DIR_START]");
      pCharacteristic->notify();
      delay(50);

      File root = SD.open("/");
      while (File entry = root.openNextFile()) {
        String line = String(entry.name());
        if (!entry.isDirectory()) {
          line += "\t" + String(entry.size());
        } else {
          line += "/";
        }
        pCharacteristic->setValue(line.c_str());
        pCharacteristic->notify();
        delay(50);
        entry.close();
      }
      root.close();

      // Inform end
      pCharacteristic->setValue("[DIR_END]");
      pCharacteristic->notify();
      delay(50);
    }
    // === Regular file chunk or text ===
    else {
      if (receivingFile && fileHandle) {
        fileHandle.write((const uint8_t*)data.data(), data.size());
      } else {
        Serial.print("Received: ");
        Serial.println(s);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  // SD card init
  Serial.println("Initializing SD card…");
  SPI.begin();
  if (!SD.begin(SD_CS)) {
    Serial.println("X SD init failed! X");
    while (true) delay(100);
  }
  Serial.println("!!! SD initialized. !!!");

  // List files on startup
  File root = SD.open("/");
  Serial.println("Files on SD at boot:");
  while (File f = root.openNextFile()) {
    Serial.print("  ");
    Serial.print(f.name());
    Serial.print(f.isDirectory() ? "/" : "");
    Serial.print("  \t");
    Serial.println(f.size());
    f.close();
  }
  root.close();

  // BLE init
  BLEDevice::init("NanoESP32-BLE");
  BLEDevice::setMTU(REQUESTED_MTU);

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ    |
    BLECharacteristic::PROPERTY_WRITE_NR |  // best throughput
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Init");
  pService->start();

  BLEAdvertising *pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->start();

  Serial.println("BLE Advertising started");
}

void loop() {
  // no periodic activity
}
