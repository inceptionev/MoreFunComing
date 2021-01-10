#include <esp_now.h>
#include <WiFi.h>
#include <M5Stack.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int a0;
  int a1;
  float f1;
  int a2;
  float f2;
} struct_message;

int prevTick=millis();

// Create a struct_message called myData
struct_message funMsg;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&funMsg, incomingData, sizeof(funMsg));
  M5.Lcd.setCursor(0,20);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.printf("a0:%d, a1:%d, f1:%.1f, a2:%d, f2:%.1f", funMsg.a0, funMsg.a1, funMsg.f1, funMsg.a2, funMsg.f2);
  M5.Lcd.setCursor(0,40);
  M5.Lcd.printf("Cycle Time: %d         ", millis()-prevTick);
  prevTick = millis();
}
 
void setup() {

  //Init M5Stack
  M5.begin();
  M5.Power.begin();
  
  // Initialize Serial Monitor
  Serial.begin(115200);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.print("ACTIVE");
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {

}
