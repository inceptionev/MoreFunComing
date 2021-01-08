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

//process variables
int amplitude = 0;
int prevTick = 0;
int pos = 0;
int pos0 = 0;
int pos1 = 0;
int pos2 = 0;
float freq = 0;

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

  M5.Lcd.clear(BGCOLOR);

  M5.Lcd.fillRect(0, 0, 320, 25, HLCOLOR);
  M5.Lcd.setCursor(70,5);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, HLCOLOR);
  M5.Lcd.printf("More Fun Coming");
  M5.Lcd.setTextColor(WHITE, BGCOLOR);
  
  //timer
  prevTick = millis();

}

void loop() {
  // put your main code here, to run repeatedly:
     
  //amplitude = encoder_value * 8;
  //amplitude = encoder_value * 5 * COUNTS_PER_MM;  //5mm per index

  //freq = 20;

  //pos = round(amplitude*sin(millis()/160.f))+127; // + round(16*sin(millis()/10.f));
  pos0 = 0;
  pos1 = round(amplitude*sin(freq*millis()/160.f))+FSCOUNTS/2;
  pos2 = 0;

  pos = pos0 + pos1 + pos2;

  //dacWrite(G26, pos);
  Serial2.printf("s r0xca %d\rt 1\r", pos); //send position


  //Left Column
  M5.Lcd.fillRect(70, 170-round(140*pos0/FSCOUNTS), 20, round(140*pos0/FSCOUNTS), WHITE);
  M5.Lcd.fillRect(70, 30, 20, 140-round(140*pos0/FSCOUNTS), BLACK);
  M5.Lcd.setCursor(17,173);
  
  //Middle Column
  M5.Lcd.fillRect(150, 170-round(140*pos1/FSCOUNTS), 20, round(140*pos1/FSCOUNTS), WHITE);
  M5.Lcd.fillRect(150, 30, 20, 140-round(140*pos1/FSCOUNTS), BLACK);
  M5.Lcd.setCursor(17,173);

  
  M5.Lcd.printf("Cyc = %d",millis()-prevTick);
  prevTick = millis();

  if (cur_button == 0 && last_button == 1) {
    PARAMSTATE++;
    PARAMSTATE = PARAMSTATE % 3;
  }
  last_button = cur_button;

  GetValue(); //read encoder
  
  switch(PARAMSTATE) {
    case 0:
      M5.Lcd.setTextColor(WHITE, HLCOLOR);
      M5.Lcd.setCursor(140,178);
      M5.Lcd.printf("SIN");
      M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(130,197);
      M5.Lcd.printf("a:%d  ",int(amplitude/COUNTS_PER_MM+0.5));
      M5.Lcd.setCursor(130,216);
      M5.Lcd.printf("f:%.1f",freq);
      break;

    case 1:
      direction ? amplitude -= encoder_increment * AINCREMENT * COUNTS_PER_MM : amplitude += encoder_increment * AINCREMENT * COUNTS_PER_MM;
      M5.Lcd.setCursor(140,178);
      M5.Lcd.printf("SIN");
      M5.Lcd.setTextColor(WHITE, HLCOLOR);
      M5.Lcd.setCursor(130,197);
      M5.Lcd.printf("a:%d  ",int(amplitude/COUNTS_PER_MM+0.5));
      M5.Lcd.setTextColor(WHITE, BGCOLOR);
      M5.Lcd.setCursor(130,216);
      M5.Lcd.printf("f:%.1f",freq);
      break;

    case 2:
      direction ? freq -= encoder_increment * FINCREMENT : freq += encoder_increment * FINCREMENT;
      M5.Lcd.setCursor(140,178);
      M5.Lcd.printf("SIN");
      M5.Lcd.setCursor(130,197);
      M5.Lcd.printf("a:%d  ",int(amplitude/COUNTS_PER_MM+0.5));
      M5.Lcd.setTextColor(WHITE, HLCOLOR);
      M5.Lcd.setCursor(130,216);
      M5.Lcd.printf("f:%.1f",freq);
      M5.Lcd.setTextColor(WHITE, BGCOLOR);
      break;

    default:
      PARAMSTATE = 0;
      break; 
  }
  
}
