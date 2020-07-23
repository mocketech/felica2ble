#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>

static const char* LOG_TAG = "BLEDevice";
uint32_t passkey;

// Nordic UART UUID
// #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// generate with https://www.uuidgenerator.net/
#define SERVICE_UUID           "e2f199c6-4d64-4aaf-9c6e-f7d9fcf0c365"
#define CHARACTERISTIC_UUID_RX "e0271d45-3d81-4565-a132-c13867adf98c"
#define CHARACTERISTIC_UUID_TX "89ce49f1-a641-442f-9d86-d4199feda02c"

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

boolean bleDataIsReceived;
std::string storedValue;
portMUX_TYPE storeDataMux = portMUX_INITIALIZER_UNLOCKED;

class MySecurity : public BLESecurityCallbacks {  // BLE security with pass key
  uint32_t onPassKeyRequest(){
    Serial.println( "onPassKeyRequest()");
    ESP_LOGI(LOG_TAG, "PassKeyRequest");
    return passkey;
  }

  void onPassKeyNotify(uint32_t pass_key){
    Serial.println( "onPassKeyNotify()");
    ESP_LOGI(LOG_TAG, "The passkey Notify number:%d", pass_key);
  }

  bool onConfirmPIN(uint32_t pass_key){
    Serial.println( "onConfirmPIN()");
    ESP_LOGI(LOG_TAG, "The passkey YES/NO number:%d", pass_key);
    vTaskDelay(5000);
    return true;
  }

  bool onSecurityRequest(){
    Serial.println( "onSecurityRequest()");
    ESP_LOGI(LOG_TAG, "SecurityRequest");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    Serial.println( "onAuthenticationComplete()");
    if(cmpl.success){
      // ペアリング完了
      Serial.println("auth success");
      ESP_LOGI(LOG_TAG, "Starting BLE work!");
    }else{
      // ペアリング失敗
      deviceConnected = false;
      Serial.println("auth failed");
      ESP_LOGI(LOG_TAG, "Starting BLE failed!");
    }
  }
};

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      portENTER_CRITICAL_ISR(&storeDataMux);
      storedValue = rxValue;
      bleDataIsReceived = true;
      portEXIT_CRITICAL_ISR(&storeDataMux);
    }
  }
};

void setup_ble( int pk)
{
  passkey = pk;
  bleDataIsReceived = false;
  
  // Create a secure BLE Device
  BLEDevice::init("IDm Reader");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());
  
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

 // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    // BLECharacteristic::PROPERTY_NOTIFY
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  // BLE security settings
  BLESecurity *pSecurity = new BLESecurity();  // pin code
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  uint32_t tmp_passkey = passkey;
  
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &tmp_passkey, sizeof(uint32_t));
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  pSecurity->setCapability(ESP_IO_CAP_OUT);
  pSecurity->setKeySize(16);
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

void deinit_ble() {
  BLEDevice::deinit( true);
}

boolean ble_device_is_connected() {
  return deviceConnected;
}

boolean ble_data_is_received() {
  return bleDataIsReceived;
}

void notify_data( String str)
{
  if (deviceConnected) {
    portENTER_CRITICAL_ISR(&storeDataMux);
    pTxCharacteristic->setValue( str.c_str());
    pTxCharacteristic->notify();
    portEXIT_CRITICAL_ISR(&storeDataMux);                
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }  
}

String receive_data()
{
  String ret = "";
  
  if (deviceConnected) {
    portENTER_CRITICAL_ISR(&storeDataMux);
    if (bleDataIsReceived) {
      bleDataIsReceived = false;
      ret = storedValue.c_str();
      Serial.println("received string: ");
      Serial.println(storedValue.c_str());
    }
    portEXIT_CRITICAL_ISR(&storeDataMux);
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }  

  return ret;
}
