/**
   ESPNOW - Basic communication - Master
   Date: 26th September 2017
   Author: Arvind Ravulavaru <https://github.com/arvindr21>
   Purpose: ESPNow Communication between a Master ESP32 and multiple ESP32 Slaves
   Description: This sketch consists of the code for the Master module.
   Resources: (A bit outdated)
   a. https://espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf
   b. http://www.esploradores.com/practica-6-conexion-esp-now/

   << This Device Master >>

   Flow: Master
   Step 1 : ESPNow Init on Master and set it in STA mode
   Step 2 : Start scanning for Slave ESP32 (we have added a prefix of `slave` to the SSID of slave for an easy setup)
   Step 3 : Once found, add Slave as peer
   Step 4 : Register for send callback
   Step 5 : Start Transmitting data from Master to Slave(s)

   Flow: Slave
   Step 1 : ESPNow Init on Slave
   Step 2 : Update the SSID of Slave with a prefix of `slave`
   Step 3 : Set Slave in AP mode
   Step 4 : Register for receive callback and wait for data
   Step 5 : Once data arrives, print it in the serial monitor

   Note: Master and Slave have been defined to easily understand the setup.
         Based on the ESPNOW API, there is no concept of Master and Slave.
         Any devices can act as master or salve.


  // Sample Serial log with 1 master & 2 slaves
      Found 12 devices 
      1: Slave:24:0A:C4:81:CF:A4 [24:0A:C4:81:CF:A5] (-44)
      3: Slave:30:AE:A4:02:6D:CC [30:AE:A4:02:6D:CD] (-55)
      2 Slave(s) found, processing..
      Processing: 24:A:C4:81:CF:A5 Status: Already Paired
      Processing: 30:AE:A4:2:6D:CD Status: Already Paired
      Sending: 9
      Send Status: Success
      Last Packet Sent to: 24:0a:c4:81:cf:a5
      Last Packet Send Status: Delivery Success
      Send Status: Success
      Last Packet Sent to: 30:ae:a4:02:6d:cd
      Last Packet Send Status: Delivery Success

*/

#include <esp_now.h>
#include <WiFi.h>
#include <M5StickC.h>

// Global copy of slave
#define NUMSLAVES 20
#define MODE_SCAN 0
#define MODE_SELECT 1
#define MODE_PLAY 2

#define PLAY_SINGLE 0
#define PLAY_MULTIPLE 1
#define PLAY_PROGRAM1 2


#define CHANNEL 1
#define PRINTSCANRESULTS 0

esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;
uint8_t DataRecv = 0;
uint8_t mode = MODE_SCAN;
int playMode = PLAY_SINGLE;
uint8_t requestSent = 0;
uint8_t request = 0;
int lastRequest = 0;
uint8_t response = 0;



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

// Scan for slaves in AP mode
void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  //reset slaves
  memset(slaves, 0, sizeof(slaves));
  SlaveCnt = 0;
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
///    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

//      if (PRINTSCANRESULTS) {
//        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
//      }
      delay(10);
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("Slave") == 0) {
        // SSID of interest
        //Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];

        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slaves[SlaveCnt].peer_addr[ii] = (uint8_t) mac[ii];
          }
        }
        slaves[SlaveCnt].channel = CHANNEL; // pick a channel
        slaves[SlaveCnt].encrypt = 0; // no encryption
        SlaveCnt++;
      }
    }
  }

  if (SlaveCnt > 0) {
    //Serial.print(SlaveCnt); Serial.println(" Slave(s) found, processing..");
  } else {
    Serial.println("No Slave Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageSlave() {
  if (SlaveCnt > 0) {
    for (int i = 0; i < SlaveCnt; i++) {
//      Serial.print("Processing: ");
      for (int ii = 0; ii < 6; ++ii ) {
//        Serial.print((uint8_t) slaves[i].peer_addr[ii], HEX);
//        if (ii != 5) Serial.print(":");
      }
     // Serial.print(" Status: ");
      // check if the peer exists
      bool exists = esp_now_is_peer_exist(slaves[i].peer_addr);
      if (exists) {
        // Slave already paired.
//        Serial.println("Already Paired");
      } else {
        // Slave not paired, attempt pair
        esp_err_t addStatus = esp_now_add_peer(&slaves[i]);
        if (addStatus == ESP_OK) {
          // Pair success
          Serial.println("Pair success");
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
          // How did we get so far!!
          Serial.println("ESPNOW Not Init");
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
          Serial.println("Add Peer - Invalid Argument");
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
          Serial.println("Peer list full");
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
          Serial.println("Out of memory");
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
          Serial.println("Peer Exists");
        } else {
          Serial.println("Not sure what happened");
        }
        delay(100);
      }
    }
  } else {
    // No slave found to process
    Serial.println("No Slave found to process");
  }
}




void sendData(uint8_t node) {


    Serial.println("Send to node" + String(node));

    const uint8_t *peer_addr = slaves[node].peer_addr;
//    if (i == 0) { // print only for first slave
//      Serial.print("Sending: ");
//      Serial.println(data);
//    }

    uint8_t data = pow(2, node);

    esp_err_t result = esp_now_send(peer_addr, &node, sizeof(node));

    Serial.print("Send Status: ");
    if (result == ESP_OK) {
      Serial.println("Success");
    } else {
      Serial.println("Not sure what happened");
      reset();
    }
    

  delay(100);
}


// send data
void sendRequest() {

  Serial.println("Request; " + String(request));
  if (requestSent == 0) {    
    if (request & 1) {
      sendData(0);
    }  
    if (request & 2) {
      sendData(1);
    }  
    if (request & 4) {
      sendData(2);
    }  
    if (request & 8) {
      sendData(3);
    }  
    if (request & 16) {
      sendData(4);
    }  
    if (request & 32) {
      sendData(5);
    }  
    if (request & 64) {
      sendData(6);
    }  
    if (request & 128) {
      sendData(7);
    }  

    requestSent = 1;   
  }  
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (mode == MODE_PLAY) {
    char macStr[18];  

    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Last Packet Recv from: "); Serial.println(macStr);
    Serial.print("Last Packet Recv Data: "); Serial.println(*data);
    Serial.println("");


  //  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
  //           slaves[i].peer_add[0], slaves[i].peer_add[1], slaves[i].peer_add[2], slaves[i].peer_add[3], slaves[i].peer_add[4], slaves[i].peer_add[5]);
  //      Serial.print("Comp Packet Recv from: "); Serial.println(macStr);


      response = response + pow(2, *data);


    Serial.println("Response: " + String(response));


    if (response >= request) {
      reset();
    }

    updateDisplay();

  }
}

void reset() {
   request = 0; 
    response = 0;
    requestSent = 0;
}

// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
//  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
//           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  //Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);

  M5.begin();
  
  //Set device in STA mode to begin with
  WiFi.mode(WIFI_STA);
  Serial.println("ESPNow/Multi-Slave/Master Example");
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
//  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void updateDisplay() {
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(15, 5);
  if (mode == MODE_SCAN) {
    M5.Lcd.setTextColor(WHITE);
  } else {
    M5.Lcd.setTextColor(GREEN);
  }
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("MStackFit");

  int line = 10;
  if (mode == MODE_SCAN) {
    line = 20;
    M5.Lcd.setCursor(5, line);
    M5.Lcd.setTextColor(RED,WHITE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("SERACH");
    line = 30;
  }

  if (mode == MODE_SELECT) {
    line = 20;
    M5.Lcd.setCursor(5, line);
    M5.Lcd.setTextColor(RED,WHITE);
    M5.Lcd.setTextSize(2);
    if (playMode == PLAY_SINGLE) {
      M5.Lcd.printf("SINGLE");
    } else if (playMode == PLAY_MULTIPLE) {
      M5.Lcd.printf("MULTI");
    } else if (playMode == PLAY_PROGRAM1) {
      M5.Lcd.printf("PRG 1");
    } 

    
    line = 30;
  }

  if (SlaveCnt > 0) {
    for (int s=1; s <= SlaveCnt; s++){
      M5.Lcd.setCursor(10, line+(s*12));
      if (request & uint8_t(pow(2, s-1))) {
        M5.Lcd.setTextColor(RED, WHITE);
      } else {
        M5.Lcd.setTextColor(RED, BLACK);
      }
      
      M5.Lcd.setTextSize(1);
      String stringOne = "SLAVE " + String(s);
      M5.Lcd.print(stringOne);
    }
  } 
  


}

void play() {
  if (requestSent == 0) {
    
    do  {
      request = pow(2, random(0, SlaveCnt));
      Serial.println("play(): " + String(lastRequest) +" != " + String(request));
    } while (lastRequest == request);

    lastRequest = request;    
    
    sendRequest();
  }
 
}

void select() {
  if (M5.BtnB.wasPressed()) {
    playMode++;
    if (playMode > PLAY_PROGRAM1) {
      playMode = 0;
    }
  }
}


void loop() {
  
  M5.update();
  if (mode == MODE_SCAN) {
    Serial.println('S');
    ScanForSlave();
    if (SlaveCnt > 0) { // check if slave channel is defined
      manageSlave();      
    }
    if (M5.BtnA.isPressed()) {
      Serial.println('A');
      mode = MODE_SELECT;  
    }
    
  }

  if (mode == MODE_SELECT) {    
    select();   
     if (M5.BtnA.isPressed()) {
      Serial.println('A');
      mode = MODE_PLAY;  
    } 
  }    


  if (mode == MODE_PLAY) {
    //Serial.println('P');
    play();
  } 
  updateDisplay();   
  delay(500);
}