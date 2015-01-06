
//Include the library LedControl
#include "LedControl.h"

LedControl lc=LedControl(10,8,9,1);


/* Define the pins */
int MUX_ONE  = 11;
int MUX_TWO  = 12;
int MUX_THREE= 13;

int DMUX_ONE  = 5;
int DMUX_TWO  = 6;
int DMUX_THREE= 7;

int HALLIN=4;

/* Arrays for the sensor matrix states */
boolean current_state[8][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}};
  
boolean delta_state[8][8] = {
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0}};
  
/* Delta threshold for recognizing a real delta occuring */
int delta_threshold = 3;

void setup() {
  pinMode(MUX_ONE   , OUTPUT);  
  pinMode(MUX_TWO   , OUTPUT);  
  pinMode(MUX_THREE , OUTPUT);  
  pinMode(DMUX_ONE  , OUTPUT);  
  pinMode(DMUX_TWO  , OUTPUT);  
  pinMode(DMUX_THREE, OUTPUT); 
     Serial.begin(9600);      // open the serial port at 9600 bps:    
  
  pinMode(HALLIN, INPUT_PULLUP); 
            lc.setLed(0, 0, 0, true);

  
  /* Wakey Wakey */
  lc.shutdown(0,false);
  /* Set the brightness to a max values */
  lc.setIntensity(0,15);
  /* and clear the display */
  lc.clearDisplay(0);
  
  initLights();

}


/* All hall sensor reading is done here */
void readSensors(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
  
      /* Write to the mux outputs for choosing a row */
      digitalWrite(MUX_ONE  , (i & 1)   );
      digitalWrite(MUX_TWO  , (i & 3)>>1);
      digitalWrite(MUX_THREE, (i & 7)>>2);
      /* Write to demux for choosing a col */
      digitalWrite(DMUX_ONE  ,(j & 1)   );
      digitalWrite(DMUX_TWO  ,(j & 3)>>1);
      digitalWrite(DMUX_THREE,(j & 7)>>2);
      /* Delay because of reaction time of sensor */
      delay(1);

      /* check the sensors */
      if(!digitalRead(HALLIN)){ // negation because of pull-up resistor
        if(current_state[i][j] != 1){ // Obviously delta happening here
          delta_state[i][j] += 1;
        } 
      } else {
        if(current_state[i][j] != 0){
          delta_state[i][j] += 1;
        }
      }  
    }
  }
}

/* Check the delta array. If a delta threshold is reached, transfer the change
   into the current state */ 
void processDeltaArray(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
      if(delta_state[i][j] >= delta_threshold){
        delta_state[i][j] = 0;
        int current = current_state[i][j];
        current_state[i][j] = current == 1 ? 0 : 1;     
      }
    }
  }
}

/* All LED stuff is handled by this function */
void doTheFancyLights(){
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
        if(current_state[i][j] == 1)
          lc.setLed(0, j, i, true);
        else
          lc.setLed(0, j, i, false); 
    }
  }  
}

/* Cool initial light effects */
void initLights(){
  int myDelay = 20;
  for(int row=0;row<8;row++) {
    for(int col=0;col<8;col++) {
      lc.setLed(0,row,col,true);
      delay(myDelay);     
    }
  }
  
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,15 - i); delay(myDelay);}
  for(int i = 0; i < 16; i++){lc.setIntensity(0,i);      delay(myDelay);}
  
  for(int row=0;row<8;row++) {
      for(int col=0;col<8;col++) {
        lc.setLed(0,row,col,false);
        delay(myDelay);
      }
    }
}

void loop() {
    readSensors();
    processDeltaArray();
    doTheFancyLights();
}
