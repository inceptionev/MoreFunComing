#include <esp_now.h>
#include <WiFi.h>
#include <M5Stack.h>

#define SDLY 500
#define CDLY 2
#define BGCOLOR DARKCYAN
#define HLCOLOR BLUE
#define FSCOUNTS 28000
#define COUNTS_PER_MM 156
#define FINCREMENT 0.1
#define AINCREMENT 2 //in mm

#define Faces_Encoder_I2C_ADDR     0X5E

//limits
#define AMIN 0
#define AMAX 14000
#define FMIN 0.1
#define FMAX 60
#define PMIN 156
#define PMAX 28000

//process variables
int prevTick = 0;
int pos = 0;
int pos0 = 0;
int pos1 = 0;
int pos2 = 0;
int a0 = 0;
int a1 = 0;
int a2 = 0;
float f0 = 0.1;
float f1 = 0.1;
float f2 = 0.1;
float theta1 = 0;
float theta2 = 0;
char sBuffer[10];
int h = 0;

//state variables
int PARAMSTATE = 0;
int PARAMOFFSET = 0;
bool lastPressedA = false;
bool lastPressedB = false;
bool lastPressedC = false;

int encoder_increment;//positive: clockwise nagtive: anti-clockwise
int encoder_value=0;
uint8_t direction;//0: clockwise 1: anti-clockwise
uint8_t last_button, cur_button;

// ESP-NOW message struct
typedef struct struct_message {
  int id;
  int pos;
} struct_message;

// Create a struct_message called myData
struct_message funMsg;


 
void setup() {

  //Init M5Stack
  M5.begin();
  M5.Power.begin();

  //graphics
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BGCOLOR);
  M5.Lcd.setTextColor(WHITE, BGCOLOR);
  M5.Lcd.setCursor(70,90);
  M5.Lcd.printf("More Fun Coming");
  M5.Lcd.setCursor(60,120);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("Establishing Wireless Connection...");
  M5.Lcd.fillRect(78, 138, 164, 24, BLACK);
  
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // register ESP-NOW callback function
  esp_now_register_recv_cb(OnDataRecv);

  //setup servo drive
  M5.Lcd.setCursor(60,120);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("    Initializing Servo Drive...    ");
  delay(1000); //wait for Xenus to boot up
  M5.Lcd.fillRect(80, 140, 20, 20, BGCOLOR);
  delay(1000); //wait for Xenus to boot up
  M5.Lcd.fillRect(80, 140, 40, 20, BGCOLOR);
  delay(1000); //wait for Xenus to boot up
  M5.Lcd.fillRect(80, 140, 60, 20, BGCOLOR);
  delay(1000); //wait for Xenus to boot up
  M5.Lcd.fillRect(80, 140, 80, 20, BGCOLOR);
  
  Serial2.print(F("s r0x90 115200\r")); //set Accelnet baud rate
  delay(SDLY);
  Serial2.begin(115200); //switch to 115200 baud rate
  Serial2.print(F("s r0x24 21\r")); //Enable in trajectory generator position mode
  delay(CDLY);
  Serial2.print(F("t 2\r")); // home in current position

  M5.Lcd.setCursor(60,120);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("    Actuator Homing Position...    ");
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 100, 20, BGCOLOR);
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 120, 20, BGCOLOR);
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 140, 20, BGCOLOR);
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 160, 20, BGCOLOR);

  M5.Lcd.setCursor(60,120);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("               Done!               ");
  Serial2.print(F("s r0xc8 0\r")); //absolute position, trapezoidal profile  
  delay(CDLY);
  Serial2.print(F("s r0xca 1500\r")); //absolute position, trapezoidal profile  
  delay(CDLY);
  Serial2.print(F("t 1\r"));

  //Begin Main program UI
  M5.Lcd.clear(BGCOLOR);

  //Title Block
  M5.Lcd.fillRect(0, 0, 320, 25, HLCOLOR);
  M5.Lcd.setCursor(70,5);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, HLCOLOR);
  M5.Lcd.printf("More Fun Coming");
  M5.Lcd.setTextColor(WHITE, BGCOLOR);

  //Fixed Text
  M5.Lcd.setCursor(10,178);
  M5.Lcd.print("OUT");
  M5.Lcd.setCursor(50,92);
  M5.Lcd.print("=");
  M5.Lcd.setCursor(120,92);
  M5.Lcd.print("+");
  M5.Lcd.setCursor(200,92);
  M5.Lcd.print("+");

  //Controller IDs
  M5.Lcd.setTextSize(6);
  M5.Lcd.drawRect(50, 180, 60, 60, WHITE);
  M5.Lcd.setCursor(65, 190);
  M5.Lcd.print("1");
  M5.Lcd.drawRect(130, 180, 60, 60, WHITE);
  M5.Lcd.setCursor(145, 190);
  M5.Lcd.print("2");
  M5.Lcd.drawRect(210, 180, 60, 60, WHITE);
  M5.Lcd.setCursor(225, 190);
  M5.Lcd.print("3");

  M5.Lcd.setTextSize(2);

  //timer
  prevTick = millis(); 
}
 
void loop() {
  pos = pos0 + pos1 + pos2;
  pos = constrain(pos, PMIN, PMAX);

  Serial2.printf("s r0xca %d\rt 1\r", pos); //send position

  //display current cycle time
  M5.Lcd.setCursor(0,5);
  M5.Lcd.printf("%d",millis()-prevTick);
  prevTick = millis();

  //Total Column
  M5.Lcd.fillRect(20, 160-round(130*pos/FSCOUNTS), 20, round(130*pos/FSCOUNTS)+10, WHITE);
  M5.Lcd.fillRect(20, 30, 20, 130-round(130*pos/FSCOUNTS), BLACK);

  //Column A
  M5.Lcd.fillRect(70, 160-round(130*pos0/FSCOUNTS), 20, 10, WHITE);
  M5.Lcd.fillRect(70, 30, 20, 130-round(130*pos0/FSCOUNTS), BLACK); //top bar
  M5.Lcd.fillRect(70, 160-round(130*pos0/FSCOUNTS)+10, 20, round(130*pos0/FSCOUNTS), BLACK); //bottom bar
  
  //Column B
  M5.Lcd.fillRect(150, 160-round(130*pos1/FSCOUNTS), 20, 10, WHITE);
  M5.Lcd.fillRect(150, 30, 20, 130-round(130*pos1/FSCOUNTS), BLACK); //top bar
  M5.Lcd.fillRect(150, 160-round(130*pos1/FSCOUNTS)+10, 20, round(130*pos1/FSCOUNTS), BLACK); //bottom bar

  //Column C
  M5.Lcd.fillRect(230, 160-round(130*pos2/FSCOUNTS), 20, 10, WHITE);
  M5.Lcd.fillRect(230, 30, 20, 130-round(130*pos2/FSCOUNTS), BLACK); //top bar
  M5.Lcd.fillRect(230, 160-round(130*pos2/FSCOUNTS)+10, 20, round(130*pos2/FSCOUNTS), BLACK); //bottom bar
  
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&funMsg, incomingData, sizeof(funMsg));
  switch(funMsg.id) {
    case 1:
      pos0 = funMsg.pos;
      break;
    case 2:
      pos1 = funMsg.pos;
      break;
    case 3:
      pos2 = funMsg.pos;
      break;
  }
}

void GetValue(void){
    int temp_encoder_increment;

    Wire.requestFrom(Faces_Encoder_I2C_ADDR, 3);
    if(Wire.available()){
       temp_encoder_increment = Wire.read();
       cur_button = Wire.read();
    }
    if(temp_encoder_increment > 127){//anti-clockwise
        direction = 1;
        encoder_increment = 256 - temp_encoder_increment;
    }
    else{
        direction = 0;
        encoder_increment = temp_encoder_increment;
    }
}

void Led(int i, int r, int g, int b){
    Wire.beginTransmission(Faces_Encoder_I2C_ADDR);
    Wire.write(i);
    Wire.write(r);
    Wire.write(g);
    Wire.write(b);
    Wire.endTransmission();
}
