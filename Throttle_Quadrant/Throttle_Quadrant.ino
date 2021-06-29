// VID=0x2342, PID=0x8032, usb_product="Throttle Quadrant"

#include <Wire.h>

// https://github.com/YukMingLaw/ArduinoJoystickWithFFBLibrary.git
#include <Joystick.h>

// For debugging set to 1
#define SerialMode 0

#if !SerialMode
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_JOYSTICK,
  10, 0,                  // Button Count, Hat Switch Count
  false, false, true,    // X and Y, Z Axis
  true, true, true,      // Rx, Ry, Rz
  true, true,           // Rudder Throttle
  false, false, false);  // Accelerator Brake Steering
#endif


// SETTINGS -----------------------------------------------------------------------
int button[]={2,3,4,5,6}; //Pins for the buttons
int16_t filterDistanceX=50; //If the handle goes past this distance (raw messurment of magnetic sensors) then it is a valid move. Else don't move the axis
int16_t filterDistanceY=30;

bool reverse[]={true,false,false,true}; //Only works for the speed break and flaps atm

// Ranges {{Handle X: Min, Max}{Handle Y: Min, Max}{Joystick X: Min, Max}{Joystick Y: Min, Max}}
int16_t limit[][2]={{100,960},{375,960},{400,965},{100,975}};
int16_t limit_lower[][2]={{100,250},{110,280}}; //The deadzone on the "nipples" of the throttles
// SETTINGS -----------------------------------------------------------------------


int analogs[]={A0,A1,A2,A3};
int adc[4];
int adc_lower[2];
int mapped_adc[4];
int mapped_lower[2];
int last_state[]={0,0,0};

//Ranges {Speedbreak, Throttle 1, Throttle 2, Flaps, Reverse 1, Reverse 2}
int range[]={1024,1024,1024,256,1024,1024};

void setup(void) 
{
  for (int i=0;i>4;i++){
    adc[i]=0;
    mapped_adc[i]=0;
    if (i<2){
      adc_lower[i]=0;
      mapped_lower[i]=0;
    }
  }
  for (int i=0;i<5;i++){
    pinMode(button[i],INPUT_PULLUP);
  }
  #if !SerialMode
    Joystick.setZAxisRange(0, range[0]);
    Joystick.setRxAxisRange(0, range[1]);
    Joystick.setRyAxisRange(0, range[2]);
    Joystick.setRzAxisRange(0, range[3]);
    Joystick.setRudderRange(0, range[4]);
    Joystick.setThrottleRange(0, range[5]);
    Joystick.begin(false);
  #else
    Serial.begin(115200);
    while (!Serial);
    delay(2000);
  #endif
}

void loop(void) 
{
  // Read the analog values
  for (int i=0;i<4;i++){
    adc[i] = analogRead(analogs[i]);

    //Filter analogs
    if (adc[i]<limit[i][0])  {
      adc[i]=limit[i][0];
    }
    if (adc[i]>limit[i][1])  {
      adc[i]=limit[i][1];
    }
    if (!reverse[i]){
      mapped_adc[i]=map(adc[i],limit[i][0],limit[i][1],0,range[i]);
    }else{
      mapped_adc[i]=map(adc[i],limit[i][0],limit[i][1],range[i],0);
    }
  }
  for (int i=0;i<2;i++){
    adc_lower[i] = analogRead(analogs[i+1]);
    
    //Filter analogs
    if (adc_lower[i]>limit_lower[i][1])  {
      adc_lower[i]=limit_lower[i][1];
    }
    if (adc_lower[i]<limit_lower[i][0])  {
      adc_lower[i]=limit_lower[i][0];
    }
    mapped_lower[i]=map(adc_lower[i],limit_lower[i][1],limit_lower[i][0],0,range[i+4]);
  }

  #if SerialMode
      for (int i=0;i<4;i++){
        Serial.print("Handle"+String(i)+":  ");
        Serial.print(mapped_adc[i]);
        Serial.print(" (");
        Serial.print(adc[i]);
        Serial.print(") || ");
      }
      for (int i=0;i<2;i++){
        Serial.print("Handle lower"+String(i+1)+":  ");
        Serial.print(mapped_lower[i]);
        Serial.print(" (");
        Serial.print(adc_lower[i]);
        Serial.print(") || ");
      }
      Serial.println();
      for (int i=0;i<5;i++){
        Serial.print(!digitalRead(button[i]) ? "Button"+String(i)+": ON" : "Button"+String(i)+": OFF");
        Serial.print(" || ");
      }
      Serial.println();
      delay(100);
      
      /*Serial.print(", Maped Value: ");
      Serial.print(map(adc[i],0,65535,(range*(-1)),range));*/
  #else
    Joystick.setZAxis(mapped_adc[0]);
    Joystick.setRxAxis(mapped_adc[1]);
    Joystick.setRyAxis(mapped_adc[2]);
    Joystick.setRzAxis(mapped_adc[3]);
    Joystick.setThrottle(mapped_lower[0]);
    Joystick.setRudder(mapped_lower[1]);

    for (int i=0;i<2;i++){
      Joystick.setButton(i,!digitalRead(button[i]));
      if (analogRead(analogs[i+1])<limit[i+1][0]-50){
        Joystick.setButton(i+8,1);
      }else{
        Joystick.setButton(i+8,0);
      }
    }
    
    for (int i=2;i<5;i++){
      if (last_state[i-2]==0){
        if (!digitalRead(button[i])){
          last_state[i-2]=1;
          Joystick.setButton(i,1);
          Joystick.sendState();
          delay(50);
          Joystick.setButton(i,0);
          Joystick.sendState();
          delay(50);
        }
      }else if (last_state[i-2]==1){
        if (digitalRead(button[i])){
          last_state[i-2]=0;
          Joystick.setButton(i+3,1);
          Joystick.sendState();
          delay(50);
          Joystick.setButton(i+3,0);
          Joystick.sendState();
          delay(50);
        }
      }
    }
    Joystick.sendState();
    delay(1);
  #endif
}
