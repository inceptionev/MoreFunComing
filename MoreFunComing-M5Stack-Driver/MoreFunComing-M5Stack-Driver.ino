#include <esp_now.h>
#include <WiFi.h>
#include <M5Stack.h>
#include "Free_Fonts.h"

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

//dynamic settings
int safeMax = 0;
int safeMin = 0;
int numControllers = 1;

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
int PARAMSTATE = 2;  //start in column B, position 0
int PARAMOFFSET = 0;
bool lastPressedA = false;
bool lastPressedB = false;
bool lastPressedC = false;
bool transitionA = false;
bool transitionB = false;
bool transitionC = false;

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

  //Set safety limits
  M5.Lcd.clear(BGCOLOR);

  //Title Block
  M5.Lcd.fillRect(0, 0, 320, 25, HLCOLOR);
  M5.Lcd.setCursor(70,5);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, HLCOLOR);
  M5.Lcd.printf("More Fun Coming");
  M5.Lcd.setTextColor(WHITE, BGCOLOR);

  //Fixed Text
  M5.Lcd.setCursor(10,90);
  M5.Lcd.print("Set Number");
  M5.Lcd.setCursor(10,109);
  M5.Lcd.print("of Players");
  M5.Lcd.setCursor(190,90);
  M5.Lcd.print("And Safety");
  M5.Lcd.setCursor(190,109);
  M5.Lcd.print("Limits");
  M5.Lcd.setCursor(208,216);
  M5.Lcd.print("Confirm");
  //Parameter Display
  writeParam(2,"Players:1",false);
  writeParam(3,"min:0",false);
  writeParam(4,"max:0",false);
  M5.update();
  bool limitConfirm = false;
  while (limitConfirm == false) {
    
    //Confirm to exit
    if(M5.BtnC.wasPressed() == 1) { 
      limitConfirm = true;
    }

    //position set and send
    pos = a0;
    pos = constrain(pos, PMIN, PMAX);
    Serial2.printf("s r0xca %d\rt 1\r", pos); //send position

    //Column B
    M5.Lcd.fillRect(150, 160-round(130*pos/FSCOUNTS), 20, 10, WHITE);
    M5.Lcd.fillRect(150, 30, 20, 130-round(130*pos/FSCOUNTS), BLACK); //top bar
    M5.Lcd.fillRect(150, 160-round(130*pos/FSCOUNTS)+10, 20, round(130*pos/FSCOUNTS), BLACK); //bottom bar

    M5.update(); //read buttons
    if(transitionA == true) {
      PARAMSTATE = 0;
      transitionA = false;
     }
     if(transitionB == true) {
      PARAMSTATE = 2;
      transitionB = false;
     }
     
    if(M5.BtnA.wasPressed() == 1) { 
       PARAMSTATE++; //proper exit state
       if (!lastPressedA) {
        transitionA = true;
       }
       lastPressedA = true;
       lastPressedB = false;
       lastPressedC = false;
    }
    if(M5.BtnB.wasPressed() == 1) { 
       PARAMSTATE++; //proper exit state
       if (!lastPressedB) {
        transitionB = true;
       }
       lastPressedB = true;
       lastPressedC = false;
       lastPressedA = false;
    }
    
    if (cur_button == 0 && last_button == 1) {
     PARAMSTATE++;
    }
    last_button = cur_button;
  
    GetValue(); //read encoder

    switch(PARAMSTATE) {
      case 0:
        break;
  
      case 1: //case 0 exit state - this just returns to state 0 since there are no other options in offset 0.
        PARAMSTATE = 0;
        break;
        
      case 2:
        writeParam(2, "Players:", true);
        break;
  
      case 3: //case 2 exit state
        writeParam(2, "Players:", false);
        PARAMSTATE = 4;
        break;
  
      case 4:
        direction ? a0 -= encoder_increment * AINCREMENT * COUNTS_PER_MM : a0 += encoder_increment * AINCREMENT * COUNTS_PER_MM;
        a0 = constrain(a0, PMIN, PMAX);
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (int(a0/COUNTS_PER_MM+0.5) < 100) ? sprintf(sBuffer,"max:%d ",int(a0/COUNTS_PER_MM+0.5)) : sprintf(sBuffer,"max:%d",int(a0/COUNTS_PER_MM+0.5)); 
        writeParam(3,sBuffer,true);
        break;
  
      case 5: //case 4 exit state
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (int(a0/COUNTS_PER_MM+0.5) < 10) ? sprintf(sBuffer,"max:%d  ",int(a0/COUNTS_PER_MM+0.5)) : sprintf(sBuffer,"max:%d ",int(a0/COUNTS_PER_MM+0.5));
        writeParam(3,sBuffer,false);
        PARAMSTATE = 6;
        break;
  
      case 6:
        direction ? a0 -= encoder_increment * AINCREMENT * COUNTS_PER_MM : a0 += encoder_increment * AINCREMENT * COUNTS_PER_MM;
        a0 = constrain(a0, PMIN, PMAX);
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (int(a0/COUNTS_PER_MM+0.5) < 100) ? sprintf(sBuffer,"min:%d ",int(a0/COUNTS_PER_MM+0.5)) : sprintf(sBuffer,"min:%d",int(a0/COUNTS_PER_MM+0.5)); 
        writeParam(4,sBuffer,true);
        break;
  
      case 7: //case 6 exit state
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (int(a0/COUNTS_PER_MM+0.5) < 10) ? sprintf(sBuffer,"min:%d  ",int(a0/COUNTS_PER_MM+0.5)) : sprintf(sBuffer,"min:%d ",int(a0/COUNTS_PER_MM+0.5));
        writeParam(4,sBuffer,false);
        PARAMSTATE = 2;  //goes back to first param in this offset
        break;
  
      case 8:
        writeParam(5, "SIN", true);
        break;
  
      case 9: //case 8 exit state
        writeParam(5, "SIN", false);
        PARAMSTATE = 10;
        break;
  
      case 10:
        direction ? a2 -= encoder_increment * AINCREMENT * COUNTS_PER_MM : a2 += encoder_increment * AINCREMENT * COUNTS_PER_MM;
        a2 = constrain(a2, AMIN, AMAX);
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (int(a2/COUNTS_PER_MM+0.5) < 10) ? sprintf(sBuffer,"a:%d ",int(a2/COUNTS_PER_MM+0.5)) : sprintf(sBuffer,"a:%d",int(a2/COUNTS_PER_MM+0.5));
        writeParam(6,sBuffer,true);
        break;
  
      case 11: //case 10 exit state
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (int(a2/COUNTS_PER_MM+0.5) < 10) ? sprintf(sBuffer,"a:%d ",int(a2/COUNTS_PER_MM+0.5)) : sprintf(sBuffer,"a:%d",int(a2/COUNTS_PER_MM+0.5));
        writeParam(6,sBuffer,false);
        PARAMSTATE = 12;
        break;
  
      case 12:
        direction ? f2 -= encoder_increment * (f2 < 3 ? 1 : 10) * FINCREMENT : f2 += encoder_increment * (f2 < 2.9 ? 1 : 10) * FINCREMENT; //something weird with < here, 3 should work
        f2 = constrain(f2, FMIN, FMAX);
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (f2 < 9.f) ? sprintf(sBuffer,"f:%.1f ",f2) : sprintf(sBuffer,"f:%.1f",f2); //whyyy doesn't f1 < 10 work?? 10 < 10 should be false.
        writeParam(7,sBuffer,true);
        break;
  
      case 13: //case 12 exit state
        //this makes the extra text clear properly when going from a longer string to a shorter one
        (f2 < 9.f) ? sprintf(sBuffer,"f:%.1f ",f2) : sprintf(sBuffer,"f:%.1f",f2); //whyyy doesn't f1 < 10 work?? 10 < 10 should be false.
        writeParam(7,sBuffer,false);
        PARAMSTATE = 8;  //goes back to first param in this offset
        break;
        
      default:
        PARAMSTATE = 0;
        break; 
    }
  
  }

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


void writeParam(int nParam, char* paramString, bool hl) {
  switch(nParam) {
    case 0: //Column 1 waveform
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 63, 178, GFXFF);
      break;

    case 1: //Column 1 amplitude
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 60, 197, GFXFF);
      break;

    case 2: //Column 2 waveform
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 63, 178, GFXFF);
      break;

    case 3: //Column 2 amplitude
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 110, 197, GFXFF);
      break;

    case 4: //Column 2 freq
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 110, 216, GFXFF);
      break;

    case 5: //Column 3 waveform
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 223, 178, GFXFF);
      break;

    case 6: //Column 3 amplitude
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 220, 197, GFXFF);
      break;

    case 7: //Column 3 freq
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.drawString(paramString, 210, 216, GFXFF);
      break;
  }
}

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
