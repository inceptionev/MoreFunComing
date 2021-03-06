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
unsigned int vcycle = 0; //unsigned so that it will wrap


void setup(){
  Serial.begin(115200);
  
  if (role == role_ping_out){
    //SETUP LinMot CONTROLLER on ADDR 0x02
    byte lm_clear_ctrl_word[] = {0x01, 0x02, 0x05, 0x02, 0x00, 0x01, 0x00, 0x00, 0x04};
    byte lm_set_op_home[] = {0x01, 0x02, 0x05, 0x02, 0x00, 0x01, 0x3F, 0x08, 0x04};
    byte lm_set_op[] = {0x01, 0x02, 0x05, 0x02, 0x00, 0x01, 0x3F, 0x00, 0x04};
    Serial.begin(115200);
    delay(4000); //wait for Accelnet to boot up
    Serial.write(lm_clear_ctrl_word, sizeof(lm_clear_ctrl_word));
    delay(SDLY);
    Serial.write(lm_set_op_home, sizeof(lm_set_op_home));
    delay(4000); //wait for actuator to home
    Serial.write(lm_set_op, sizeof(lm_set_op));
  }

    
  //Serial.println(F("RF24/examples/GettingStarted_CallResponse"));
  //Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));
 
  // Setup and configure radio
  
  //pinMode(3, OUTPUT); //setup analog output pin
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(72);
  radio.setDataRate(RF24_250KBPS);
  
  radio.enableAckPayload();                     // Allow optional ack payloads
  radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
  
  if(radioNumber > 0){                          // i.e., radio is a controller, listen on numbered address, transmit back on address[0]
    radio.openWritingPipe(addresses[0]);        // Both radios listen on the same pipes by default, but opposite addresses
    radio.openReadingPipe(1,addresses[radioNumber]);      // Open a reading pipe on address 0, pipe 1
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
    int pos;  //pos should be an int so it doesn't get tripped up by negative values.
    byte vibe;
    
    radio.stopListening();                                  // First, stop listening so we can talk.      
    //Serial.print(F("Now sending "));                         // Use a simple byte counter as payload
    //Serial.println(counter);
    
    //unsigned long time = micros();                          // Record the current microsecond count   
                                                            
    if ( radio.write(&counter,1) ){                         // Send the counter variable to the other radio 
        if(!radio.available()){                             // If nothing in the buffer, we got an ack but it is blank
            //Serial.print(F("Got blank response. round-trip delay: "));
            //Serial.print(micros()-time);
            //Serial.println(F(" microseconds"));     
        }else{      
            while(radio.available() ){                      // If an ack with payload was received
                radio.read( &gotByte, 2 );                  // Read it, and display the response time
                //pos = int(gotByte[0]/10)+int(VAMT*sin(0.01*(float(255-gotByte[1])/10.0-11.5)*float(vcycle%628))/10);  //modulo at n to make sure it wraps nicely at 2pi, multiplier set at 2*pi/n
                pos = int(gotByte[0]/10)+int(((255-gotByte[1])/2-20)*sin(0.01*vcycle))/10;  //modulo at n to make sure it wraps nicely at 2pi, multiplier set at 2*pi/n
                //pos = gotByte/10;
                byte lm_vai_goto_prefix[] = {0x01, 0x11, 0x09, 0x02, 0x00, 0x02, 0x01, 0x02, 0xA0};
                byte lm_vai_goto_postfix[] = {0x01, 0x00, 0x04}; 
                Serial.write(lm_vai_goto_prefix, sizeof(lm_vai_goto_prefix)); //update command
                Serial.write(byte(pos));
                Serial.write(lm_vai_goto_postfix, sizeof(lm_vai_goto_postfix)); //carriage return
                delay(CDLY);                
            }
        }
    
    }else{        }//Serial.println(F("Sending failed.")); }          // If no ack response, sending failed
    
    //delay(1000);  // Try again later
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



/****************** Change Roles via Serial Commands ***************************/

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == role_pong_back ){      
      Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
      role = role_ping_out;  // Become the primary transmitter (ping out)
      counter = 1;
   }else
    if ( c == 'R' && role == role_ping_out ){
      Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));      
       role = role_pong_back; // Become the primary receiver (pong back)
       radio.startListening();
       counter = 1;
       radio.writeAckPayload(1,&counter,1);
       
    }
  }
}
