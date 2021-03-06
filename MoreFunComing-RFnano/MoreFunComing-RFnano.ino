/*
   Dec 2014 - TMRh20 - Updated
   Derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads 
 * 
 * This example continues to make use of all the normal functionality of the radios including 
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well. 
 * This allows very fast call-response communication, with the responding radio never having to 
 * switch out of Primary Receiver mode to send back a payload, but having the option to switch to 
 * primary transmitter if wanting to initiate communication instead of respond to a commmunication. 
 */
 
#include <SPI.h>
#include "RF24.h"
#define CDLY 2
#define SDLY 500
#define NUM_RADIOS 4 //set the number of controllers to look for
//#define VAMT 50.0 //in 255 scale

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
byte radioNumber = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(10,9);
/**********************************************************/
                                                                           // Topology
byte addresses[][6] = {"1Node","2Node","3Node","4Node","5Node"};                           // 2node-5node are base tx to select a controller, round-robin style, 1Node is the response pipe, 
//byte addresses[][6] = {"1Node","2Node"};                                 // Radio pipe addresses for the 2 nodes to communicate.

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  
typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles
role_e role = role_ping_out;                                              // The role of the current running sketch

byte counter = 1;                                                          // A single byte to keep track of the data being sent back and forth
unsigned int vcycle[NUM_RADIOS]; //unsigned so that it will wrap


void setup(){
  Serial.begin(115200);
  
  if (role == role_ping_out){
    //SETUP ACCELNET CONTROLLER
    Serial.begin(9600);
    delay(4000); //wait for Accelnet to boot up
    Serial.print(F("s r0x90 115200\r")); //set Accelnet baud rate
    delay(SDLY);
    Serial.begin(115200); //switch to 115200 baud rate
    Serial.print(F("s r0x24 21\r")); //Enable in trajectory generator position mode
    delay(CDLY);
    Serial.print(F("t 2\r")); //tell Accelnet to home in current position
    delay(SDLY); //wait for home to complete
    Serial.print(F("s r0xc8 0\r")); //absolute position, trapezoidal profile
    delay(CDLY);
    Serial.print(F("s r0xca 2\r")); //demo indicator
    delay(CDLY);
    Serial.print(F("t 1\r")); //execute command
    delay(SDLY);
    Serial.print(F("s r0xca 0\r")); //demo indicator
    delay(CDLY);
    Serial.print(F("t 1\r")); //execute command
    delay(SDLY);
    Serial.print(F("s r0xca 2\r")); //demo indicator
    delay(CDLY);
    Serial.print(F("t 1\r")); //execute command
    delay(SDLY);
    Serial.print(F("s r0xca 0\r")); //demo indicator
    delay(CDLY);
    Serial.print(F("t 1\r")); //execute command
    delay(SDLY);
  }

 
  // Setup and configure radio
  
  //pinMode(3, OUTPUT); //setup analog output pin
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(72);
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(0,1);  //no retries for round-robin
  
  radio.enableAckPayload();                     // Allow optional ack payloads
  radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
  
  if(radioNumber > 0){                          // i.e., radio is a controller, listen on numbered address, transmit back on address[0]
    radio.openWritingPipe(addresses[0]);        // Both radios listen on the same pipes by default, but opposite addresses
    radio.openReadingPipe(1,addresses[radioNumber]);      // Open a reading pipe on address per radionumber, pipe 1
  }else{                                        // i.e., if radio is the base, listen on address[0], transmit round-robin on addresses 1-4
    radio.openWritingPipe(addresses[1]);        // start on address[1]
    radio.openReadingPipe(1,addresses[0]);      // listen for replies on address[0]
  }
  radio.startListening();                       // Start listening  
  
  radio.writeAckPayload(1,&counter,1);          // Pre-load an ack-paylod into the FIFO buffer for pipe 1
  //radio.printDetails();
}

void loop(void) {

  
/****************** Ping Out Role ***************************/

  if (role == role_ping_out){                               // Radio is in ping mode

    byte gotByte[2];                                           // Initialize a variable for the incoming response
    int pos[NUM_RADIOS]={0,0,0,0};  //pos should be an int so it doesn't get tripped up by negative values.
    int spos=0;
    
    radio.stopListening();                                  // First, stop listening so we can talk.      
 
    for(int counter = 0; counter < NUM_RADIOS; counter++) {   //ping controllers round-robin style
      radio.openWritingPipe(addresses[1]); 
      if ( radio.write(&counter,1) ){                         // Send the counter variable to the other radio 
          if(!radio.available()){                             // If nothing in the buffer, we got an ack but it is blank                  
          }else{      
              while(radio.available() ){                      // If an ack with payload was received
                  radio.read( &gotByte, 2 );                  // Read it
                  //pos[counter] = int(gotByte[0]/10)+int(((255-gotByte[1])/2-20)*sin(0.01*vcycle[counter]))/10;  //modulo at n to make sure it wraps nicely at 2pi, multiplier set at 2*pi/n                           
                  pos[counter] = int(gotByte[0]/10);
                  vcycle[counter]=(vcycle[counter]+int((255-gotByte[1])/5.0-24.5))%628;//equals 2pi when it reaches 628
              }
          }
      
      }else{        }          // If no ack response, sending failed
    }

    for(int counter = 0; counter < NUM_RADIOS; counter++) {
      spos = spos + pos[counter];
    }
    spos = spos/NUM_RADIOS;
    Serial.print(F("s r0xca ")); //update command
    Serial.print(spos);
    Serial.print("\r"); //carriage return
    delay(CDLY);
    Serial.print(F("t 1\r")); //execute command
    delay(CDLY);     
      
    
  }


/****************** Pong Back Role ***************************/

  if ( role == role_pong_back ) {
    byte pipeNo, gotByte, yreply[2];                          // Declare variables for the pipe and the byte received
    while( radio.available(&pipeNo)){              // Read all available payloads
      radio.read( &gotByte, 1 );                   
                                                   // Since this is a call-response. Respond directly with an ack payload.
      gotByte += 1;                                // Ack payloads are much more efficient than switching to transmit mode to respond to a call
      yreply[0] = lowByte(analogRead(0)>>2);
      yreply[1] = lowByte(analogRead(1)>>2);
      radio.writeAckPayload(pipeNo,&yreply, 2 );  // This can be commented out to send empty payloads.
      Serial.print(F("Loaded next response "));
      Serial.print(yreply[0]);
      Serial.print(" ");
      Serial.println(yreply[1]);  
   }
 }
}
