#include <M5Stack.h>
#include <WiFi.h>
#include <esp_now.h>

//Change this per controller!
#define CONTROLLER_ID 1

//Change this to match the controller in the Driver!
#define DRIVERADDR {0xB4, 0xE6, 0x2D, 0xF9, 0xF3, 0xB1}

//K00201900: A4:CF:12:6D:7A:E4
//K00201226
//K00201864: B4:E6:2D:F9:F3:B1

//Program defines
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
bool transitionA = false;
bool transitionB = false;
bool transitionC = false;

int encoder_increment;//positive: clockwise nagtive: anti-clockwise
int encoder_value=0;
uint8_t direction;//0: clockwise 1: anti-clockwise
uint8_t last_button, cur_button;

//ESP-NOW
uint8_t driverAddress[] = DRIVERADDR;

typedef struct struct_message {
  int id;
  int pos;
} struct_message;

struct_message funMsg;

void setup() {
  
  M5.begin();
  M5.Power.begin();
  
  //Splash Screen
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BGCOLOR);
  M5.Lcd.setTextColor(WHITE, BGCOLOR);
  M5.Lcd.setCursor(70,90);
  M5.Lcd.printf("More Fun Coming");
  M5.Lcd.setCursor(120,120);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("Connecting...");
  M5.Lcd.fillRect(78, 138, 164, 24, BLACK);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, driverAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  
  for(int k=0; k < 20; k++) {
    M5.Lcd.fillRect(80, 140, k*8, 20, BGCOLOR);
    delay(100);
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

  //Parameter Display
  writeParam(0,"POS",false);
  writeParam(1,"a:0",false);
  writeParam(2,"SIN",false);
  writeParam(3,"a:0",false);
  writeParam(4,"f:0.1",false);
  writeParam(5,"SIN",false);
  writeParam(6,"a:0",false);
  writeParam(7,"f:0.1",false);
  
  //timer
  prevTick = millis();

}

void loop() {

  pos0 = a0;
  pos0 = constrain(pos0, PMIN, PMAX);
  
  theta1 += f1*(millis()-prevTick)/160.f; //to make freq transitions smooth
  pos1 = round(a1*sin(theta1));
  pos1 = constrain(pos1, -PMAX, PMAX);
  
  theta2 += f2*(millis()-prevTick)/160.f;
  pos2 = round(a2*sin(theta2));
  pos2 = constrain(pos2, -PMAX, PMAX);

  pos = pos0 + pos1 + pos2;
  pos = constrain(pos, PMIN, PMAX);

  //Send Controller State
  funMsg.id = CONTROLLER_ID;
  funMsg.pos = pos;
  esp_now_send(driverAddress, (uint8_t *) &funMsg, sizeof(funMsg));

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
  M5.Lcd.fillRect(150, 95-round(130*pos1/FSCOUNTS), 20, 10, WHITE);
  M5.Lcd.fillRect(150, 30, 20, 65-round(130*pos1/FSCOUNTS), BLACK); //top bar
  M5.Lcd.fillRect(150, 95-round(130*pos1/FSCOUNTS)+10, 20, 65+round(130*pos1/FSCOUNTS), BLACK); //bottom bar

  //Column C
  M5.Lcd.fillRect(230, 95-round(130*pos2/FSCOUNTS), 20, 10, WHITE);
  M5.Lcd.fillRect(230, 30, 20, 65-round(130*pos2/FSCOUNTS), BLACK); //top bar
  M5.Lcd.fillRect(230, 95-round(130*pos2/FSCOUNTS)+10, 20, 65+round(130*pos2/FSCOUNTS), BLACK); //bottom bar

  M5.update(); //read buttons
  if(transitionA == true) {
    PARAMSTATE = 0;
    transitionA = false;
   }
   if(transitionB == true) {
    PARAMSTATE = 2;
    transitionB = false;
   }
   if(transitionC == true) {
    PARAMSTATE = 8;
    transitionC = false;
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
  if(M5.BtnC.wasPressed() == 1) { 
     PARAMSTATE++; //proper exit state
     if (!lastPressedC) {
      transitionC = true;
     }
     lastPressedC = true;
     lastPressedA = false;
     lastPressedB = false;
  }
  
  if (cur_button == 0 && last_button == 1) {
   PARAMSTATE++;
  }
  last_button = cur_button;

  GetValue(); //read encoder
  
  switch(PARAMSTATE) {
    case 0:
      direction ? a0 -= encoder_increment * AINCREMENT * COUNTS_PER_MM : a0 += encoder_increment * AINCREMENT * COUNTS_PER_MM;
      a0 = constrain(a0, PMIN, PMAX);
      h = sprintf(sBuffer,"a:%d ",int(a0/COUNTS_PER_MM+0.5));
      writeParam(1,sBuffer,true);
      break;

    case 1: //case 0 exit state - this just returns to state 0 since there are no other options in offset 0.
      h = sprintf(sBuffer,"a:%d  ",int(a0/COUNTS_PER_MM+0.5));
      writeParam(1,sBuffer,false);
      PARAMSTATE = 0;
      break;
      
    case 2:
      writeParam(2, "SIN", true);
      break;

    case 3: //case 2 exit state
      writeParam(2, "SIN", false);
      PARAMSTATE = 4;
      break;

    case 4:
      direction ? a1 -= encoder_increment * AINCREMENT * COUNTS_PER_MM : a1 += encoder_increment * AINCREMENT * COUNTS_PER_MM;
      a1 = constrain(a1, AMIN, AMAX);
      h = sprintf(sBuffer,"a:%d ",int(a1/COUNTS_PER_MM+0.5));
      writeParam(3,sBuffer,true);
      break;

    case 5: //case 4 exit state
      h = sprintf(sBuffer,"a:%d  ",int(a1/COUNTS_PER_MM+0.5));
      writeParam(3,sBuffer,false);
      PARAMSTATE = 6;
      break;

    case 6:
      direction ? f1 -= encoder_increment * (f1 < 3 ? 1 : 10) * FINCREMENT : f1 += encoder_increment * (f1 < 2.9 ? 1 : 10) * FINCREMENT; //something weird with < here, 3 should work
      f1 = constrain(f1, FMIN, FMAX);
      h = sprintf(sBuffer,"f:%.1f ",f1);
      writeParam(4,sBuffer,true);
      break;

    case 7: //case 6 exit state
      h = sprintf(sBuffer,"f:%.1f  ",f1);
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
      h = sprintf(sBuffer,"a:%d ",int(a2/COUNTS_PER_MM+0.5));
      writeParam(6,sBuffer,true);
      break;

    case 11: //case 10 exit state
      h = sprintf(sBuffer,"a:%d  ",int(a2/COUNTS_PER_MM+0.5));
      writeParam(6,sBuffer,false);
      PARAMSTATE = 12;
      break;

    case 12:
      direction ? f2 -= encoder_increment * (f2 < 3 ? 1 : 10) * FINCREMENT : f2 += encoder_increment * (f2 < 2.9 ? 1 : 10) * FINCREMENT; //something weird with < here, 3 should work
      f2 = constrain(f2, FMIN, FMAX);
      h = sprintf(sBuffer,"f:%.1f ",f2);
      writeParam(7,sBuffer,true);
      break;

    case 13: //case 6 exit state
      h = sprintf(sBuffer,"f:%.1f  ",f2);
      writeParam(7,sBuffer,false);
      PARAMSTATE = 8;  //goes back to first param in this offset
      break;
      
    default:
      PARAMSTATE = 0;
      break; 
  }
  
}

void writeParam(int nParam, char* paramString, bool hl) {
  switch(nParam) {
    case 0: //Column 1 waveform
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(63,178);
      M5.Lcd.print(paramString);
      break;

    case 1: //Column 1 amplitude
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(60,197);
      M5.Lcd.print(paramString);
      break;

    case 2: //Column 2 waveform
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(143,178);
      M5.Lcd.print(paramString);
      break;

    case 3: //Column 2 amplitude
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(140,197);
      M5.Lcd.print(paramString);
      break;

    case 4: //Column 2 freq
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(130,216);
      M5.Lcd.print(paramString);
      break;

    case 5: //Column 3 waveform
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(223,178);
      M5.Lcd.print(paramString);
      break;

    case 6: //Column 3 amplitude
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(220,197);
      M5.Lcd.print(paramString);
      break;

    case 7: //Column 3 freq
      hl ? M5.Lcd.setTextColor(WHITE, HLCOLOR) : M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(210,216);
      M5.Lcd.print(paramString);
      break;
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  
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
