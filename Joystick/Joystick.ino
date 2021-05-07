// VID=0x2341, PID=0x8031, usb_product="Flight Stick"

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
  7, 0,                  // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  true, true, false,   // Rx, Ry, but no Rz
  true, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering
#endif


// SETTINGS -----------------------------------------------------------------------
int trigger=5; //Pin for the trigger button
int thumb=7; //Pin for the joystick click
bool joyXhat=0; //0 Analog thumb joystick - 1 Act like arrow button
bool joyYhat=1;
int16_t filterDistanceX=50; //If the handle goes past this distance (raw messurment of magnetic sensors) then it is a valid move. Else don't move the axis
int16_t filterDistanceY=30;

// Deadzones {Handle X, Handle Y, Joystick X, Joystick Y}
int deadzone[]={400,400,500,500}; //Defaults: 300,300,500,500
int deadzoneHat=350; //If the thumb joystick goes past the center value by this much, then accumulate an arrow press
int reverse[]={1,1,1,1}; // -1 reversed, 1 NOT reversed

// Ranges {{Handle X: Min, Middle, Max}{Handle Y: Min, Middle, Max}{Joystick X: Min, Middle, Max}{Joystick Y: Min, Middle, Max}}
int16_t limit[][3]={{9500,12700,15600},{8500,13200,16700},{500,12000,23500},{500,12000,23500}};
int limit_analogs[][2]={{250,990},{220,990}};
// SETTINGS -----------------------------------------------------------------------


int16_t adc[4];
int16_t last_adc[4];
int analogs[2];
int16_t mapped_adc[4];
int mapped_analogs[2];
bool toebrake=0;
int16_t range=512;
bool filterChecked[]={false,false};

void setup(void) 
{
  for (int i=0;i>4;i++){
    adc[i]=0;
    mapped_adc[i]=0;
    last_adc[i]=0;
  }
  for (int i=0;i<2;i++){
    analogs[i]=0;
    mapped_analogs[i]=0;
  }
  pinMode(trigger,INPUT_PULLUP);
  pinMode(thumb,INPUT_PULLUP);
  #if !SerialMode
    ads.begin();
    Joystick.setXAxisRange(range*(-1), range);
    Joystick.setYAxisRange(range*(-1), range);
    Joystick.setRxAxisRange(range*(-1),range);
    Joystick.setRyAxisRange(range*(-1),range);
    //Joystick.setThrottleRange(range*(-1), range); // Using the throttle axis as toe brake (when you press both analog pedals together)
    Joystick.setRudderRange(range*(-1), range);
    Joystick.begin(false);

    Joystick.setThrottle(range*(-1));
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
  // Read the normal analog inputs
  analogs[0]=analogRead(A0);
  analogs[1]=analogRead(A1);

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
      mapped_adc[i]=map(adc[i],limit[i][0],limit[i][1]-deadzone[i]-1,range*(reverse[i]*(-1)),0);
    }
    if (adc[i]>limit[i][1]){
      mapped_adc[i]=map(adc[i],limit[i][1]+deadzone[i]+1,limit[i][2],0,range*(reverse[i]));
    }
    if (adc[i]==limit[i][1]){
      mapped_adc[i]=0;
    }
  }

  // Filter -Handle X- values
  #if !SerialMode
  if (adc[0]<limit[0][0]){ //Check minimum
    adc[0]=limit[0][0];
  }
  if (adc[0]>limit[0][2]){ //Check maximum
    adc[0]=limit[0][2];
  }
  #endif
  if (adc[0]>limit[0][1]-deadzone[0] && adc[0]<limit[0][1]+deadzone[0]){ //Middle deadzone
    adc[0]=limit[0][1];
  }


  
  if (adc[0]<limit[0][1]){//Map the negative values only (useful if the left range on the joystick is different than the right range)
    if ((adc[0] > last_adc[0]+filterDistanceX) || (adc[0] < last_adc[0]-filterDistanceX)){
      last_adc[0]=adc[0];
      filterChecked[0]=false;
      mapped_adc[0]=map(adc[0],limit[0][0],limit[0][1]-deadzone[0]-1,range*(reverse[0]*(-1)),0);
    }else{
      if (!filterChecked[0]){
        filterChecked[0]=true;
        last_adc[0]=adc[0];
      }
    }
  }
  if (adc[0]>limit[0][1]){//Map the positive values only
    if ((adc[0] > last_adc[0]+filterDistanceX) || (adc[0] < last_adc[0]-filterDistanceX)){
      filterChecked[0]=false;
      last_adc[0]=adc[0];
      mapped_adc[0]=map(adc[0],limit[0][1]+deadzone[0]+1,limit[0][2],0,range*(reverse[0]));
    }else{
      if (!filterChecked[0]){
        filterChecked[0]=true;
        last_adc[0]=adc[0];
      }
    }
  }
  if (adc[0]==limit[0][1]){//Check absolute center
    mapped_adc[0]=0;
  }
  
  // Filter -Handle Y- values
  #if !SerialMode
  if (adc[1]<limit[1][0]){
    adc[1]=limit[1][0];
  }
  if (adc[1]>limit[1][2]){
    adc[1]=limit[1][2];
  }
  #endif
  if (adc[1]>limit[1][1]-deadzone[1] && adc[1]<limit[1][1]+deadzone[1]){
    adc[1]=limit[1][1];
  }
  if (adc[1]<limit[1][1]){
    if ((adc[1] > last_adc[1]+filterDistanceX) || (adc[1] < last_adc[1]-filterDistanceX)){
      filterChecked[1]=false;
      last_adc[1]=adc[1];
      mapped_adc[1]=map(adc[1],limit[1][0],limit[1][1]-deadzone[1]-1,range*(reverse[1]*(-1)),0);
    }else{
      if (!filterChecked[1]){
        filterChecked[1]=true;
        last_adc[1]=adc[1];
      }
    }
  }
  if (adc[1]>limit[1][1]){
    if ((adc[1] > last_adc[1]+filterDistanceX) || (adc[1] < last_adc[1]-filterDistanceX)){
      filterChecked[1]=false;
      last_adc[1]=adc[1];
      mapped_adc[1]=map(adc[1],limit[1][1]+deadzone[1]+1,limit[1][2],0,range*(reverse[1]));
    }else{
      if (!filterChecked[1]){
        filterChecked[1]=true;
        last_adc[1]=adc[1];
      }
    }
  }
  if (adc[1]==limit[1][1]){
    mapped_adc[1]=0;
  }

  //Filter analogs
  if (analogs[0]<limit_analogs[0][0])  {
    analogs[0]=limit_analogs[0][0];
  }
  if (analogs[0]>limit_analogs[0][1])  {
    analogs[0]=limit_analogs[0][1];
  }
  mapped_analogs[0]=map(analogs[0],limit_analogs[0][1],limit_analogs[0][0],range*(-1),0);
  
  if (analogs[1]<limit_analogs[1][0])  {
    analogs[1]=limit_analogs[1][0];
  }
  if (analogs[1]>limit_analogs[1][1])  {
    analogs[1]=limit_analogs[1][1];
  }
  mapped_analogs[1]=map(analogs[1],limit_analogs[1][0],limit_analogs[1][1],0,range);

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
      if (joyYhat == 1){
        if (mapped_adc[2]<(-deadzoneHat)){
          Serial.print("DOWN");
        }
        if (mapped_adc[2]>=(-512)+deadzoneHat && mapped_adc[2]<=512-deadzoneHat){
          Serial.print("MID");
        }
        if (mapped_adc[2]>deadzoneHat){
          Serial.print("UP");
        }
      }else{
        Serial.print(adc[2]);
      }
      Serial.print(")");
      
      Serial.print(" || Joystick Y:  ");
      Serial.print(mapped_adc[3]);
      Serial.print(" (");
      if (joyXhat == 1){
        if (mapped_adc[3]<(-deadzoneHat)){
          Serial.print("LEFT");
        }
        if (mapped_adc[3]>=(-512)+deadzoneHat && mapped_adc[3]<=512-deadzoneHat){
          Serial.print("MID");
        }
        if (mapped_adc[3]>deadzoneHat){
          Serial.print("RIGHT");
        }
      }else{
        Serial.print(adc[3]);
      }
      Serial.println(")");

      if ((mapped_analogs[0]<0)&&(mapped_analogs[1]>0)){
        Serial.print("Rudder (Toe Brake):  ");
        Serial.print(abs((mapped_analogs[0]*(-1))+mapped_analogs[1])/2);
      }else{
        Serial.print("Rudder:  ");
        Serial.print(mapped_analogs[0]+mapped_analogs[1]);
      }
      Serial.print(" (");
      Serial.print(analogs[0]);
      Serial.print(" - ");
      Serial.print(mapped_analogs[0]);
      Serial.print(", ");
      Serial.print(analogs[1]);
      Serial.print(" - ");
      Serial.print(mapped_analogs[1]);
      Serial.print(")");

      Serial.print(!digitalRead(trigger) ? "Trigger: Press" : "Trigger: ");
      Serial.print(" || ");
      Serial.println(!digitalRead(thumb) ? "Thumb: Press" : "Thumb: ");

      Serial.println();
      
      /*Serial.print(", Maped Value: ");
      Serial.print(map(adc[i],0,65535,(range*(-1)),range));*/
  #else
    Joystick.setXAxis(mapped_adc[0]);
    Joystick.setYAxis(mapped_adc[1]);
    if (joyYhat == 1){
      if (mapped_adc[2]<(-deadzoneHat)){
        Joystick.setButton(2,1);
      }
      if ((mapped_adc[2]>=(-512)+deadzoneHat && mapped_adc[2]<=512-deadzoneHat)){
        Joystick.setButton(2,0);
        Joystick.setButton(3,0);
      }
      if (mapped_adc[2]>deadzoneHat){
        Joystick.setButton(3,1);
      }
    }else{
      Joystick.setRxAxis(mapped_adc[2]);
    }
    if (joyXhat == 1){
      if (mapped_adc[3]<(-deadzoneHat)){
        Joystick.setButton(4,1);
      }
      if ((mapped_adc[2]>=(-512)+deadzoneHat && mapped_adc[2]<=512-deadzoneHat) && (mapped_adc[3]>=(-512)+deadzoneHat && mapped_adc[3]<=512-deadzoneHat)){
        Joystick.setButton(4,0);
        Joystick.setButton(5,0);
      }
      if (mapped_adc[3]>deadzoneHat){
        Joystick.setButton(5,1);
      }
    }else{
      Joystick.setRyAxis(mapped_adc[3]);
    }
    
    if ((mapped_analogs[0]!=0)&&(mapped_analogs[1]!=0)){ //Pressing both pedals activates toe brake mode (both axes are combined to report a single value on ThrottleAxis)
      Joystick.setButton(6,1);
    }
    if ((mapped_analogs[0]==0)||(mapped_analogs[1]==0)){
      Joystick.setButton(6,0);
    }
    Joystick.setRudder(mapped_analogs[0] + mapped_analogs[1]);
  
    Joystick.setButton(0,!digitalRead(trigger));
    Joystick.setButton(1,!digitalRead(thumb));
    Joystick.sendState();
    delay(1);
  #endif
}
