#include <Wire.h>
#include <Adafruit_ADS1015.h>

// https://github.com/dmadison/ArduinoXInput.git
// https://github.com/dmadison/ArduinoXInput_AVR.git
#include <XInput.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

int32_t adc[4];
int32_t mapped_adc[4];
int analogs[2];
int mapped_analogs[2];
int toebrake_analog=0;
int trigger=5;
int thumb=7;

int32_t range=512;

// Deadzones {Handle X, Handle Y, Joystick X, Joystick Y}
int deadzone[]={400,400,500,500};

// -1 reversed, 1 NOT reversed
int reverse[]={1,-1,-1,-1};

// Ranges {{Handle X: Min, Middle, Max}{Handle Y: Min, Middle, Max}{Joystick X: Min, Middle, Max}{Joystick Y: Min, Middle, Max}}
int16_t limit[][3]={{9500,12900,15300},{8500,13000,16700},{500,12000,23500},{500,12000,23500}};
int limit_analogs[][2]={{250,900},{260,950}};
bool toebrake=0;

void setup(void) 
{
  for (int i=0;i>4;i++){
    adc[i]=0;
    mapped_adc[i]=0;
  }
  for (int i=0;i<2;i++){
    analogs[i]=0;
    mapped_analogs[i]=0;
  }
  pinMode(trigger,INPUT_PULLUP);
  pinMode(thumb,INPUT_PULLUP);
  
  ads.begin();

  XInput.setTriggerRange(range*(-1), range);
  XInput.setJoystickRange(range*(-1), range);
  XInput.setAutoSend(false);  // Wait for all controls before sending
  XInput.begin();
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
  if (adc[0]<limit[0][0]){
    adc[0]=limit[0][0];
  }
  if (adc[0]>limit[0][2]){
    adc[0]=limit[0][2];
  }
  if (adc[0]>limit[0][1]-deadzone[0] && adc[0]<limit[0][1]+deadzone[0]){
    adc[0]=limit[0][1];
  }
  if (adc[0]<limit[0][1]){
    mapped_adc[0]=map(adc[0],limit[0][0],limit[0][1]-deadzone[0]-1,range*(reverse[0]*(-1)),0);
  }
  if (adc[0]>limit[0][1]){
    mapped_adc[0]=map(adc[0],limit[0][1]+deadzone[0]+1,limit[0][2],0,range*(reverse[0]));
  }
  if (adc[0]==limit[0][1]){
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
    mapped_adc[1]=map(adc[1],limit[1][0],limit[1][1]-deadzone[0]-1,range*(reverse[1]*(-1)),0);
  }
  if (adc[1]>limit[1][1]){
    mapped_adc[1]=map(adc[1],limit[1][1]+deadzone[0]+1,limit[1][2],0,range*(reverse[1]));
  }
  if (adc[1]==limit[1][1]){
    mapped_adc[1]=0;
  }

  //Filter analogs
  if (analogs[0]<limit_analogs[0][0]){
    mapped_analogs[0]=limit_analogs[0][0];
  }
  if (analogs[0]>limit_analogs[0][1]){
    mapped_analogs[0]=limit_analogs[0][1];
  }
  mapped_analogs[0]=map(analogs[0],limit_analogs[0][1],limit_analogs[0][0],range*(-1),0);

  if (analogs[1]<limit_analogs[1][0]){
    mapped_analogs[1]=limit_analogs[1][0];
  }
  if (analogs[1]>limit_analogs[1][1]){
    mapped_analogs[1]=limit_analogs[1][1];
  }
  mapped_analogs[1]=map(analogs[1],limit_analogs[1][0],limit_analogs[1][1],0,range);

  if ((mapped_analogs[0]<0)&&(mapped_analogs>0)){
    toebrake=1;
    XInput.setTrigger(TRIGGER_LEFT, 0);
    XInput.setTrigger(TRIGGER_RIGHT, 0);
    XInput.setButton(BUTTON_X, 1);
  }else{
    if (toebrake==true){
      toebrake_analogs=0;
      if ((mapped_analogs[0]==0) && (mapped_analogs[1]==0)){
        toebrake=false;
        XInput.setButton(BUTTON_X, 0);
      }
    }else{
      XInput.setTrigger(TRIGGER_LEFT, mapped_analogs[0]);
      XInput.setTrigger(TRIGGER_RIGHT, mapped_analogs[1]);
    }
  }

  XInput.setJoystickX(JOY_LEFT, mapped_adc[0]);
  XInput.setJoystickY(JOY_LEFT, mapped_adc[1]);
  XInput.setJoystickX(JOY_RIGHT, mapped_adc[2]);
  XInput.setJoystickY(JOY_RIGHT, mapped_adc[3]);
  XInput.setTrigger(TRIGGER_LEFT, mapped_analogs[0]);
  XInput.setTrigger(TRIGGER_RIGHT, mapped_analogs[1]);
  XInput.setButton(BUTTON_A, !digitalRead(trigger));
  XInput.setButton(BUTTON_B, !digitalRead(thumb));
  XInput.send();
  delay(1);
}
