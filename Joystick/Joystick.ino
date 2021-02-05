#include <Wire.h>
#include <Adafruit_ADS1015.h>

// https://github.com/YukMingLaw/ArduinoJoystickWithFFBLibrary.git
#include <Joystick.h>

// For debugging set to 1
#define SerialMode 0

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */

#if !SerialMode
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_JOYSTICK,
  2, 0,                  // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  true, true, false,   // Rx, Ry, but no Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering
#endif

int16_t adc[4];
int16_t mapped_adc[4];
int trigger=5;
int thumb=7;

int16_t range=512;

// Deadzones {Handle X, Handle Y, Joystick X, Joystick Y}
int deadzone[]={300,300,500,500};
int reverse[]={1,1,1,1}; // -1 reversed, 1 NOT reversed

// Ranges {{Handle X: Min, Middle, Max}{Handle Y: Min, Middle, Max}{Joystick X: Min, Middle, Max}{Joystick Y: Min, Middle, Max}}
int16_t limit[][3]={{9500,13150,15500},{8700,13400,16700},{500,12000,23500},{500,12000,23500}};

void setup(void) 
{
  for (int i=0;i>4;i++){
    adc[i]=0;
    mapped_adc[i]=0;
  }
  pinMode(trigger,INPUT_PULLUP);
  pinMode(thumb,INPUT_PULLUP);
  #if !SerialMode
    ads.begin();
    Joystick.setXAxisRange(range*(-1), range);
    Joystick.setYAxisRange(range*(-1), range);
    Joystick.setRxAxisRange(range*(-1),range);
    Joystick.setRyAxisRange(range*(-1),range);
    Joystick.begin();
  #else
    ads.begin();
    Serial.begin(115200);
    while (!Serial);
    delay(2000);
  #endif
}

void loop(void) 
{
  // Read the values of the ADS
  for (int i=0;i<4;i++){
    adc[i] = ads.readADC_SingleEnded(i);
  }

  // Filter the values of joystick
  for (int i=2;i<4;i++){
    if (adc[i]>limit[i][1]-deadzone[i] && adc[i]<limit[i][1]+deadzone[i]){
      adc[i]=limit[i][1];
    }
    if (adc[i]>limit[i][2]){
      adc[i]=limit[i][2];
    }
    if (adc[i]<limit[i][0]){
      adc[i]=limit[i][0];
    }
    if (adc[i]<limit[i][1]){
      mapped_adc[i]=map(adc[i],limit[i][0],limit[i][1],range*(reverse[i]*(-1)),0);
    }
    if (adc[i]>limit[i][1]){
      mapped_adc[i]=map(adc[i],limit[i][1],limit[i][2],0,range*(reverse[i]));
    }
    if (adc[i]==limit[i][1]){
      mapped_adc[i]=0;
    }
  }

  // Filter -Handle X- values
  if (adc[0]<limit[0][0]){ //Check minimum
    adc[0]=limit[0][0];
  }
  if (adc[0]>limit[0][2]){ //Check maximum
    adc[0]=limit[0][2];
  }
  if (adc[0]>limit[0][1]-deadzone[0] && adc[0]<limit[0][1]+deadzone[0]){ //Middle deadzone
    adc[0]=limit[0][1];
  }
  if (adc[0]<limit[0][1]){//Map the negative values only (useful if the left range on the joystick is different than the right range)
    mapped_adc[0]=map(adc[0],limit[0][0],limit[0][1],range*(reverse[0]*(-1)),0);
  }
  if (adc[0]>limit[0][1]){//Map the positive values only
    mapped_adc[0]=map(adc[0],limit[0][1],limit[0][2],0,range*(reverse[0]));
  }
  if (adc[0]==limit[0][1]){//Check absolute center
    mapped_adc[0]=0;
  }
  
  // Filter -Handle Y- values
  if (adc[1]<limit[1][0]){
    adc[1]=limit[1][0];
  }
  if (adc[1]>limit[1][2]){
    adc[1]=limit[1][2];
  }
  if (adc[1]>limit[1][1]-deadzone[1] && adc[1]<limit[1][1]+deadzone[1]){
    adc[1]=limit[1][1];
  }
  if (adc[1]<limit[1][1]){
    mapped_adc[1]=map(adc[1],limit[1][0],limit[1][1],range*(reverse[1]*(-1)),0);
  }
  if (adc[1]>limit[1][1]){
    mapped_adc[1]=map(adc[1],limit[1][1],limit[1][2],0,range*(reverse[1]));
  }
  if (adc[1]==limit[1][1]){
    mapped_adc[1]=0;
  }

  #if SerialMode
      Serial.print("Handle X:  ");
      Serial.print(mapped_adc[0]);
      Serial.print(" (");
      Serial.print(adc[0]);
      Serial.print(")");
      
      Serial.print(" || Handle Y:  ");
      Serial.print(mapped_adc[1]);
      Serial.print(" (");
      Serial.print(adc[1]);
      Serial.println(")");
      
      Serial.print("Joystick X:  ");
      Serial.print(mapped_adc[2]);
      Serial.print(" (");
      Serial.print(adc[2]);
      Serial.print(")");
      
      Serial.print(" || Joystick Y:  ");
      Serial.print(mapped_adc[3]);
      Serial.print(" (");
      Serial.print(adc[3]);
      Serial.println(")");

      Serial.print(!digitalRead(trigger) ? "Trigger: Press" : "Trigger: ");
      Serial.print(" || ");
      Serial.println(!digitalRead(thumb) ? "Thumb: Press" : "Thumb: ");

      Serial.println();
      
      /*Serial.print(", Maped Value: ");
      Serial.print(map(adc[i],0,65535,(range*(-1)),range));*/
    delay(100);
  #else
    Joystick.setXAxis(mapped_adc[0]);
    Joystick.setYAxis(mapped_adc[1]);
    Joystick.setRxAxis(mapped_adc[2]);
    Joystick.setRyAxis(mapped_adc[3]);
    Joystick.setButton(0,!digitalRead(trigger));
    Joystick.setButton(1,!digitalRead(thumb));
    delay(1);
  #endif
}
//Test