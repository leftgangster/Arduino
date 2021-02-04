//---------------------------------------------------------------
//  Sample X-Plane UDP Communications for Arduino ESP8266 Variant
//  Copyright(c) 2019 by David Prue <dave@prue.com>
//
//  You may freely use this code or any derivitive for any
//  non-commercial purpose as long as the above copyright notice
//  is included.  For commercial use information email me.
//---------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <stdlib.h>

// Change these two lines to match your Wifi SSID and Password
  
String ssid = "WIFI Name";
String pskey = "password";


WiFiUDP udp;

IPAddress my_IP;
IPAddress multicastIP (239,255,1,1);  // Do not change this
IPAddress xplane_ip;

uint8_t beacon_major_version;
uint8_t beacon_minor_version;
uint32_t application_host_id;
uint32_t versionNumber;
uint32_t receive_port;
uint32_t role;
uint16_t port;
char xp_hostname[32];

#define STATE_IDLE 0
#define STATE_SEARCH 1
#define STATE_READY 2

char buffer[1024];

int state = STATE_IDLE;
unsigned int my_port = 3017;      // Could be anything, this just happens to be my favorite port number
unsigned int xplane_port = 49010; // Don't change this
unsigned int beacon_port = 49707; // or this...
int retry = 30;

void setup() {
  retry = 30;
  Serial.begin(115200);
  Serial.println("\nX-Plane UDP Interface 0.9\n");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pskey);

  Serial.print("Attempting Connection to Network: ");
  Serial.print(ssid);

  // Wait for connection
  while ((retry-- > 0) && (WiFi.status() != WL_CONNECTED)) {
    delay(500);
    Serial.print(".");
  }

  if (retry > 0) {
    my_IP = WiFi.localIP();
    Serial.print("connected.");
    Serial.print("My IP = ");
    Serial.println(my_IP);
  }
  else {
    Serial.println("Unable to connect");
    return;
  }

  Serial.print("Searching for X-Plane");
  retry = 30;
  udp.beginMulticast(my_IP, multicastIP, beacon_port);
  state = STATE_SEARCH;
}

void loop() {

  // If we were unable to connect to WiFi
  // try setting up again
  
  if (state == STATE_IDLE) {
    setup();
    return;
  }
  
  // See if we have a UDP Packet
  int packetSize = udp.parsePacket();

  if (!packetSize) {
    if (state == STATE_SEARCH) {
      Serial.print(".");
      delay(500);
      if (!retry--) {
        Serial.println("not found");
        delay(1000);
        setup();
        return;
      }
    }
  }
  else {
    switch(state) {
    case STATE_SEARCH:
      if (udp.destinationIP() == multicastIP) {
        char *buff = &buffer[1]; // For Alignment
        xplane_ip = udp.remoteIP();
        udp.read(buff, packetSize);
        beacon_major_version = buff[5];
        beacon_minor_version = buff[6];
        application_host_id = *((int *) (buff+7));
        versionNumber = *((int *) (buff+11));
        receive_port = *((int *) (buff+15));
        strcpy(xp_hostname, &buff[21]);
        
        String version = String(versionNumber/10000)+"."+String((versionNumber%10000)/100)+"r"+String(versionNumber%100);
        String heading = " Found Version "+version+" running on "+String(xp_hostname)+" at IP ";
        Serial.print(heading);
        Serial.println(udp.remoteIP());
        
        state = STATE_READY;
        udp.begin(my_port);
  
        // Subscribe to whatever Data Refs you want here
        // The argumants are: subscribe(dataref, freq, unique)
        // The freq is how many times per second you want to get 
        // the data, unique is simply a unique number
        // so that we can identify the data as it comes in.
        
        subscribe("sim/cockpit/autopilot/altitude", 1, 42);
        subscribe("sim/cockpit/radios/com1_stdby_freq_hz", 1, 43);

      }
      break;
      
    case STATE_READY:
      struct {
        char align[3]; // Not sure why this is needed but it is
        char hdr[5]; // The first 4 bytes is the header (RREF) and after this 1 is NULL (0x00)
        int index1;
        float value1;
        int index2;
        float value2;

        // You can have as many as you like
        /*int index1;
        float value1;
        int index1;
        float value1;*/
      }receivedData __attribute__((packed));
      
      udp.read(receivedData.hdr, packetSize);
      Serial.print("Index: ");
      Serial.print(receivedData.index1);
      Serial.print("Value: ");
      Serial.println(receivedData.value1);
      Serial.print("Index: ");
      Serial.print(receivedData.index2);
      Serial.print("Value: ");
      Serial.println(receivedData.value2);
    }
  }
}


void subscribe(char *dref, uint32_t freq, uint32_t index) {
  struct {
    char dummy[3]; // For alignment
    char hdr[5] = "RREF";
    uint32_t dref_freq;
    uint32_t dref_en;
    char dref_string[400];
  } req __attribute__((packed));

  req.dref_freq = freq;
  req.dref_en = index;
  for (int x = 0; x < sizeof(req.dref_string); x++)
    req.dref_string[x] = 0x20;
  strcpy((char *) req.dref_string, (char *) dref);
  
  udp.beginPacket(xplane_ip, xplane_port);
  udp.write(req.hdr, sizeof(req) - sizeof(req.dummy));
  udp.endPacket();
  Serial.print("Subscribed to dref \"");
  Serial.print(req.hdr);
  Serial.println("\"");
}

void setValue(char *dref, uint32_t var, char dref_path[500]){
  struct {
    char hdr[5] = "DREF";
    uint32_t simVar;
    char dref_string[500];
  } dataToSend __attribute__((packed));
  
  for (int i=0;i<500;i++){
    dataToSend.dref_string[i]=0x20;
  }
  strcpy((char *)dataToSend.hdr, (char *)dref);
}
