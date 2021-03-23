#include <esp_now.h>
#include <WiFi.h>
#include <M5Atom.h>

#define CHANNEL 1

uint8_t masterAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t response = 0;
uint8_t masterRegisted = 0;
esp_now_peer_info_t masterPeerInfo;

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  String Prefix = "Slave:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "123456789";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

void setup() {
  M5.begin(true, false, true);
  Serial.begin(115200);
  Serial.println("ESPNow/Basic/Slave Example");
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP_STA);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  masterAddress[0] = mac_addr[0];
  masterAddress[1] = mac_addr[1];
  masterAddress[2] = mac_addr[2];
  masterAddress[3] = mac_addr[3];
  masterAddress[4] = mac_addr[4];
  masterAddress[5] = mac_addr[5];
  
  response = *data;

//  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
//           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);
//  Serial.println("");
  if (masterRegisted == 0) {
    masterRegisted = registerMaster();
  }

  M5.dis.drawpix(0, 0xf00000);
  
}

void sendData() {

    const uint8_t *peer_addr = masterAddress;
    esp_err_t result = esp_now_send(peer_addr, &response, sizeof(response));
    Serial.print("Send Status: ");
    if (result == ESP_OK) {
      Serial.println("Success: " + String(response));
    } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
      // How did we get so far!!
      Serial.println("ESPNOW not Init.");
    } else if (result == ESP_ERR_ESPNOW_ARG) {
      Serial.println("Invalid Argument");
    } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
      Serial.println("Internal Error");
    } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
      Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
      Serial.println("Peer not found.");
    } else if (result == ESP_ERR_ESPNOW_IF) {
      Serial.println("Interface error.");
    } else if (result == ESP_ERR_ESPNOW_EXIST) {
      Serial.println("ESPNOW peer has existed.");
    } else if (result == ESP_ERR_ESPNOW_FULL) {
      Serial.println("ESPNOW peer list is full.");
    } else {
      Serial.println("Not sure what happened");
    }
    M5.dis.clear();
    delay(100);
  
}

uint8_t registerMaster() {
    // Register peer
  
  memcpy(masterPeerInfo.peer_addr, masterAddress, 6);
  masterPeerInfo.channel = CHANNEL;  
  masterPeerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&masterPeerInfo) != ESP_OK){
    Serial.println("Failed to add MASTER");
    return 0;
  }

  return 1;
}


void loop() {
  M5.update();
  char macStr[18];
//  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
//  masterAddress[0], masterAddress[1], masterAddress[2], masterAddress[3], masterAddress[4], masterAddress[5]);
//  Serial.print("MaterAddress is: "); Serial.println(macStr);
  if (M5.Btn.wasPressed()) {
    sendData();
    //Serial.print("A button is pressed.");
  }  
  delay(10);
}
