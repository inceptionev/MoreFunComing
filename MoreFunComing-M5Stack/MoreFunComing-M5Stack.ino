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
char sBuffer[10];
int h = 0;

//state variables
int PARAMSTATE = 0;
int WAVESTATE = 0;

int encoder_increment;//positive: clockwise nagtive: anti-clockwise
int encoder_value=0;
uint8_t direction;//0: clockwise 1: anti-clockwise
uint8_t last_button, cur_button;

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

void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Power.begin();
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  dacWrite(G26, 0);

  //graphics
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BGCOLOR);
  M5.Lcd.setTextColor(WHITE, BGCOLOR);
  M5.Lcd.setCursor(70,90);
  M5.Lcd.printf("More Fun Coming");
  M5.Lcd.setCursor(80,120);
  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("Initializing Servo Drive...");
  M5.Lcd.fillRect(78, 138, 164, 24, BLACK);

  //setup servo drive
  
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
  
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 100, 20, BGCOLOR);
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 120, 20, BGCOLOR);
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 140, 20, BGCOLOR);
  delay(1000); //wait for home to complete
  M5.Lcd.fillRect(80, 140, 160, 20, BGCOLOR);
  
  Serial2.print(F("s r0xc8 0\r")); //absolute position, trapezoidal profile  
  delay(CDLY);
  Serial2.print(F("s r0xca 1500\r")); //absolute position, trapezoidal profile  
  delay(CDLY);
  Serial2.print(F("t 1\r"));

  /*
  Serial2.print(F("s r0x19 84840\r")); //set analog input scaling
  delay(CDLY);
  Serial2.print(F("s r0xcb 7343750\r")); //set max velocity
  delay(CDLY);
  Serial2.print(F("s r0xcc 367187\r")); //set acceleration
  delay(CDLY);
  Serial2.print(F("s r0xcd 367187\r")); //set deceleration
  delay(CDLY);
  Serial2.print(F("s r0x24 22\r")); //set analog position command mode
  delay(CDLY);
  Serial2.print(F("t 1\r")); //execute command
  delay(CDLY);
  */

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
  // put your main code here, to run repeatedly:
     
  //amplitude = encoder_value * 8;
  //amplitude = encoder_value * 5 * COUNTS_PER_MM;  //5mm per index

  //freq = 20;

  //pos = round(amplitude*sin(millis()/160.f))+127; // + round(16*sin(millis()/10.f));
  pos0 = a0;
  pos0 = constrain(pos0, PMIN, PMAX);
  pos1 = round(a1*sin(f1*millis()/160.f))+FSCOUNTS/2;
  pos1 = constrain(pos1, PMIN, PMAX);
  pos2 = round(a2*sin(f2*millis()/160.f))+FSCOUNTS/2;
  pos2 = constrain(pos2, PMIN, PMAX);

  pos = pos0 + pos1 + pos2;
  pos = constrain(pos, PMIN, PMAX);

  //dacWrite(G26, pos);
  Serial2.printf("s r0xca %d\rt 1\r", pos); //send position

  //Total Column
  M5.Lcd.fillRect(20, 170-round(140*pos/FSCOUNTS), 20, round(140*pos/FSCOUNTS), WHITE);
  M5.Lcd.fillRect(20, 30, 20, 140-round(140*pos/FSCOUNTS), BLACK);
  
  //Left Column
  M5.Lcd.fillRect(70, 170-round(140*pos0/FSCOUNTS), 20, round(140*pos0/FSCOUNTS), WHITE);
  M5.Lcd.fillRect(70, 30, 20, 140-round(140*pos0/FSCOUNTS), BLACK);
  
  //Middle Column
  M5.Lcd.fillRect(150, 170-round(140*pos1/FSCOUNTS), 20, round(140*pos1/FSCOUNTS), WHITE);
  M5.Lcd.fillRect(150, 30, 20, 140-round(140*pos1/FSCOUNTS), BLACK);

  //Middle Column
  M5.Lcd.fillRect(230, 170-round(140*pos2/FSCOUNTS), 20, round(140*pos2/FSCOUNTS), WHITE);
  M5.Lcd.fillRect(230, 30, 20, 140-round(140*pos2/FSCOUNTS), BLACK);
  
  //display current cycle time
  M5.Lcd.setCursor(0,5);
  M5.Lcd.printf("%d",millis()-prevTick);
  prevTick = millis();

  if (cur_button == 0 && last_button == 1) {
    PARAMSTATE++;
    PARAMSTATE = PARAMSTATE % 6;
  }
  last_button = cur_button;

  GetValue(); //read encoder
  
  switch(PARAMSTATE) {
    case 0:
      writeParam(2, "SIN", true);
      break;

    case 1: //case 0 exit state
      writeParam(2, "SIN", false);
      PARAMSTATE = 2;
      break;

    case 2:
      direction ? a1 -= encoder_increment * AINCREMENT * COUNTS_PER_MM : a1 += encoder_increment * AINCREMENT * COUNTS_PER_MM;
      a1 = constrain(a1, AMIN, AMAX);
      h = sprintf(sBuffer,"a:%d ",int(a1/COUNTS_PER_MM+0.5));
      writeParam(3,sBuffer,true);
      break;

    case 3: //case 2 exit state
      h = sprintf(sBuffer,"a:%d  ",int(a1/COUNTS_PER_MM+0.5));
      writeParam(3,sBuffer,false);
      PARAMSTATE = 4;
      break;

    case 4:
      direction ? f1 -= encoder_increment * FINCREMENT : f1 += encoder_increment * FINCREMENT;
      f1 = constrain(f1, FMIN, FMAX);
      h = sprintf(sBuffer,"f:%.1f",f1);
      writeParam(4,sBuffer,true);
      break;

    case 5: //case 4 exit state
      h = sprintf(sBuffer,"f:%.1f ",f1);
      writeParam(4,sBuffer,false);
      PARAMSTATE = 0;
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
