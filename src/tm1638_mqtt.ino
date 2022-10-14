
/* 
   ===================================================================================================================== 
   TM1638 MQTT for ESP8266
   Version: 0.51 (WIP)
   Last Updated: 10/14/2022
   NOTICE:  The project is released under the GNU Public License v3.0 and as such, it is provided "as-is" without warrantly or guarantee.  
   Use as entirely at your own risk and the developer assumes no liability or responsibility for damage or injury as a result of use of this code.
   =====================================================================================================================  
This sketch basically provides MQTT wrapper functions for some (but not all) of the TM1638Plus library.  
See https://github.com/gavinlyonsrepo/TM1638plus for full details on this libary.

 TM1838plus Libary Built-in functions:
 tm.reset() - Resets display
 tm.brighness(int) - set brightness 0-7
 tm.readButtons();
      buttons contains a byte with values of button s8s7s6s5s4s3s2s1
       HEX  :  Switch no : Binary  : Decimal/Int/Byte
       0x01 : S1 Pressed  0000 0001        1
       0x02 : S2 Pressed  0000 0010        2
       0x04 : S3 Pressed  0000 0100        4
       0x08 : S4 Pressed  0000 1000        8
       0x10 : S5 Pressed  0001 0000       16
       0x20 : S6 Pressed  0010 0000       32
       0x40 : S7 Pressed  0100 0000       64
       0x80 : S8 Pressed  1000 0000      128
       multi-press example:
       0x42 : S2 and s7 Pressed 0100 0010 (66)
       (there are theoretically 254 different possible button press combinations!)
      
 tm.displayText(const char *text)
 tm.displayASCII(uint8_t position, uint8_t ascii)  - display single ASCII char at position 0-7
 tm.displayASCIIwDot(uint8_t position, uint8_t ascii) - displays single ASCII char with dot at position 0-7
 tm.displayHex(uint8_t position, uint8_t hex) - Displays single hex value (passed as decimal 0-15) at position 0-7
 tm.display7Seg(uint8_t position, uint8_t value) - Turns on individual segment(s) as position 0-7.  Example value:  0x11000000 turns on dot and middle segment (dash) (dot)gfedcba
 tm.displayIntNum(unsigned long number, boolean leadingZeros = true) - Displays integer with or without leading zeros
 tm.DisplayDecNumNibble(uint16_t numberUpper, uint16_t numberLower, boolean leadingZeros = true) - display a 4-digit decimal to each of the 4-number segments, with or without leading zeros
 tm.setLEDs(uint16_t greenred); pass byte for on/off e.g. 0xF100  1111 0000 - L8-L1 pass zero for greenred (only model 3) - Note: L8 is right most LED
 tm.setLED((uint8_t position, uint8_t value) - turn individual LED on/off, position, 1 on/0 off.  Pos L1-L8.
*/

#include <TM1638plus.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include "Credentials.h"          //File must exist in same folder as .ino.  Edit as needed for project
#include "Settings.h"             //File must exist in same folder as .ino.  Edit as needed for project
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  #include <PubSubClient.h>
#endif

//GLOBAL VARIABLES (MQTT and OTA Updates)
bool mqttConnected = false;       //Will be enabled if defined and successful connnection made.  This var should be checked upon any MQTT actin.
long lastReconnectAttempt = 0;    //If MQTT connected lost, attempt reconnenct
uint16_t ota_time = ota_boot_time_window;
uint16_t ota_time_elapsed = 0;           // Counter when OTA active

//Delays and intervals
unsigned long debounce_previousMillis = 0;
const long  debounce_interval  = 225; // interval at which to debounce button readback function(milliseconds)
unsigned long blinkPrevTime = 0;

//Other globals
byte oldMode = 0;
short curBrightness = 0;
bool blinkLEDs = false;
bool ledsOn = false;
byte prevLEDs = 0;
uint8_t prevButtons = 0; 
String haTime;        //This global would hold a time value in 24-hour format (e.g. 14:23) passed in via MQTT that can be converted to display AM/PM time via the showTime function
String haDate;        //This global would hold a date value in yyyy-mm-dd format that can be shown as month day via the showDate function

//Setup Local Access point if enabled via WIFI Mode
#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2)
  const char* APssid = AP_SSID;        
  const char* APpassword = AP_PWD;  
#endif

//Setup Wifi if enabled via WIFI Mode
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2)
  const char *ssid = SID;
  const char *password = PW;
#endif

//Setup MQTT if enabled - only available when WiFi is also enabled
#if (WIFIMODE == 1 || WIFIMODE == 2) && (MQTTMODE == 1)    // MQTT only available when on local wifi
  const char *mqttUser = MQTTUSERNAME;
  const char *mqttPW = MQTTPWD;
  const char *mqttClient = MQTTCLIENT;
  const char *mqttTopicSub = MQTT_TOPIC_SUB;
 // const char *mqttTopicPub = MQTT_TOPIC_PUB;
#endif

//Constructor object (GPIO STB , GPIO CLOCK , GPIO DIO, use high freq MCU)
TM1638plus tm(STROBE_TM, CLOCK_TM , DIO_TM, high_freq);

WiFiClient espClient;
ESP8266WebServer server;
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  PubSubClient client(espClient);
#endif

//------------------------------
// Setup WIFI
//-------------------------------
void setup_wifi() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP);  //Disable WiFi Sleep
  delay(200);
  // WiFi - AP Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 0 || WIFIMODE == 2) 
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(APssid, APpassword);    // IP is usually 192.168.4.1
#endif
  // WiFi - Local network Mode or both
#if defined(WIFIMODE) && (WIFIMODE == 1 || WIFIMODE == 2) 
  byte count = 0;

  WiFi.hostname(WIFIHOSTNAME);
  WiFi.begin(ssid, password);
  #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
    Serial.print("Connecting to WiFi");
  #endif
  while (WiFi.status() != WL_CONNECTED) {
    #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
      Serial.print(".");
    #endif
    // Stop if cannot connect
    if (count >= 60) {
      // Could not connect to local WiFi 
      #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
        Serial.println();
        Serial.println("Could not connect to WiFi.");   
      #endif  
      return;
    }
    delay(500);
    count++;
  }
  #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
    Serial.println();
    Serial.println("Successfully connected to Wifi");
    IPAddress ip = WiFi.localIP();
    Serial.println(WiFi.localIP());
  #endif
#endif   
};

//------------------------------
// Setup MQTT
//-------------------------------
void setup_mqtt() {
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  byte mcount = 0;
  //char topicPub[32];
  client.setServer(MQTTSERVER, MQTTPORT);
  client.setCallback(callback);
  #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
    Serial.print("Connecting to MQTT broker.");
  #endif
  while (!client.connected( )) {
    #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
      Serial.print(".");
    #endif
    client.connect(mqttClient, mqttUser, mqttPW);
    if (mcount >= 60) {
      #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
        Serial.println();
        Serial.println("Could not connect to MQTT broker. MQTT disabled.");
      #endif
      // Could not connect to MQTT broker
      return;
    }
    delay(500);
    mcount++;
  }
  #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
    Serial.println();
    Serial.println("Successfully connected to MQTT broker.");
  #endif
  client.subscribe(MQTT_TOPIC_SUB"/#");
  client.publish(MQTT_TOPIC_PUB"/mqtt", "connected", true);
  mqttConnected = true;
#endif
}

void reconnect() {
  int retries = 0;
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  while (!client.connected()) {
    if(retries < 150)
    {
      #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
        Serial.print("Attempting MQTT connection...");
      #endif
      if (client.connect(mqttClient, mqttUser, mqttPW)) 
      {
        #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
          Serial.println("connected");
        #endif
        // ... and resubscribe
        client.subscribe(MQTT_TOPIC_SUB"/#");
      } 
      else 
      {
        #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
        #endif
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if(retries > 149)
    {
    ESP.restart();
    }
  }
#endif
}

// --- MQTT Message Processing
//  Handle MQTT incoming (subbed) topics.  General format like:
//  if (strcmp(topic, MQTT_TOPIC_SUB"/mytopic") == 0) {
//    myvar = message; OR myvar = message.toInt();
//    //then do something with myvar message
//  }  
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = (char*)payload;
  // Display Text
  if (strcmp(topic, MQTT_TOPIC_SUB"/displaytext") == 0 ) {
    //Clear display and set modes to 0
    clearText();
    displayMode = 0;
    oldMode = displayMode;
    tm.displayText(message.c_str());
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/cleardisplay") == 0) {
    clearText();
  // Display single character at specified position
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/displaychar") == 0 ) {
    int pos = message.substring(0,1).toInt();
    String dispChar = message.substring(2);
    byte msgLen = dispChar.length() + 1;
    char outMsg[msgLen];
    dispChar.toCharArray(outMsg, msgLen);
    uint8_t outChar = ((outMsg[0] - '0') + 48) ;
    tm.displayASCII(pos - 1, outChar);  
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/displaychardot") == 0) {
    int pos = message.substring(0,1).toInt();
    String dispChar = message.substring(2);
    byte msgLen = dispChar.length() + 1;
    char outMsg[msgLen];
    dispChar.toCharArray(outMsg, msgLen);
    uint8_t outChar = ((outMsg[0] - '0') + 48) ;
    tm.displayASCIIwDot(pos - 1, outChar);  
    
  // Reset All - clears display and LEDs
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/reset") == 0) {
    tm.reset();
  // LED Lights
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/setled") == 0) {
    int pos = message.substring(0,1).toInt(); 
    int state = message.substring(1).toInt();
    if (pos > 8) {
      //Turn all on
      tm.setLEDs(0xFF00);
    } else if (pos < 1) {  
      //Turn all off
      tm.setLEDs(0x0000);
    } else {              //LEDs numbered 1-8, but referenced as 0-7 (7 is rightmost LED), so substract 1 from pos
      pos = pos - 1;
      tm.setLED(pos, state); 
    }
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/setleds") == 0) {
    setAllLeds(message);
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/brightness") == 0) {
    byte newBrightness = message.toInt();
    if (newBrightness > 7) {
      newBrightness = 7;
    } else if (newBrightness < 0) {
      newBrightness = 0;
    }
    tm.brightness(newBrightness);
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/mode") == 0) {
     displayMode = message.toInt(); 
  //Date and time (from Home Assistant or other source via MQTT)
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/hatime") == 0) {
      haTime = message;
  } else if (strcmp(topic, MQTT_TOPIC_SUB"/hadate") == 0) {
      haDate = message;
  }
}

void setup() {
  // -----------------------------------------------
  // Setup routines for serial monitor, OTA Updates
  // -----------------------------------------------
  // Serial monitor
  #if defined(SERIAL_DEBUG) && (SERIAL_DEBUG == 1)
    Serial.begin(115200);
    Serial.println("Booting...");
  #endif
  setup_wifi();
  setup_mqtt();
  //-----------------------------
  // Setup OTA Updates
  //-----------------------------
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
  });
  ArduinoOTA.begin();
  // Setup handlers for web calls for OTAUpdate and Restart
  server.on("/restart",[](){
    server.send(200, "text/html", "<h1>Restarting...</h1>");
    delay(1000);
    ESP.restart();
  });
  server.on("/otaupdate",[]() {
    server.send(200, "text/html", "<h1>Ready for upload...<h1><h3>Start upload from IDE now</h3>");
    ota_flag = true;
    ota_time = ota_time_window;
    ota_time_elapsed = 0;
  });
  server.begin();
  
  tm.displayBegin();  // This handles pinmodes
  //Set default brightness and clear display
  tm.brightness(default_brightness);
  curBrightness = default_brightness;
  tm.reset();

//Publish init MQTT states
#if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
  updateMQTTButtonState(0);
  updateMQTTMode();
  updateMQTTBrightness();
#endif
}

// ===================================
//    MAIN LOOP
// ===================================
void loop() {
  // ================================================================
  //  This first section handles OTA updates and MQTT call processing
  // ================================================================
  //Handle OTA updates when OTA flag set via HTML call to http://ip_address/otaupdate
  if (ota_flag) {
    tm.reset();
    tm.displayText("upload");
    uint16_t ota_time_start = millis();
    while (ota_time_elapsed < ota_time) {
      ArduinoOTA.handle();  
      ota_time_elapsed = millis()-ota_time_start;   
      delay(10); 
    }
    ota_flag = false;
    tm.reset();
  }
  //Handle any web calls
  server.handleClient();
  //MQTT Calls
  #if defined(MQTTMODE) && (MQTTMODE == 1 && (WIFIMODE == 1 || WIFIMODE == 2))
    if (!client.connected()) 
    {
      reconnect();
    }
    client.loop();
  #endif
  // ===========================================
  // put rest of your main code here
  // ===========================================


  // =====================================================
  // This section provides a couple default button presses
  //   Button 1 toggles display mode between blank, time and date (time and date must be supplied via MQTT)
  //   Button 2 sets the brightness from 7 down to 0, then it wraps back to 7
  //   Both display mode and brighness can also be set via MQTT in addition to the button presses
  // See the comment section at top for more info on button press values
  // =====================================================
  unsigned long currentMillis = millis();
  uint8_t buttons = buttonsRead();
  if (buttons == 1) {
    // Display mode
    displayMode ++;
    if (displayMode > 2) {  //if you add additional display modes, be sure to update the max value here
      displayMode = 0;
    }
  } else if (buttons == 2) {
    //Brightness
    curBrightness --;
    if (curBrightness < 0) {
      curBrightness = 7;  //loop back to full bright.  Valid brightness values are 0-7
    }
    tm.brightness(curBrightness);
    updateMQTTBrightness();
  }

  // ====================================
  //  Publish button press value via MQTT
  // ====================================
  // If you do not need or want button press value published via MQTT for use in
  // external automations, like Home Assistant, comment out this section
  if (buttons != prevButtons) {
    updateMQTTButtonState(buttons);
    prevButtons = buttons;
  }

  // ===========================================================
  //  Process display mode from either button press or MQTT sub
  // ===========================================================
  if (displayMode != oldMode) {
    clearText();
    oldMode = displayMode;
    updateMQTTMode();
  }
  switch(displayMode) {
    case 0:   //blank (turn off) the display and LEDs. Normally used if display will be set via MQTT
      //tm.reset();
      break;
    case 1:  // Display time (must be receive via MQTT - see docs)
      //Time
      showTime();
      break;
    case 2: 
      //Date
      showDate();
      break;
/*  Add your own custom display modes here  
    case 3:
      break;
    case 4:
      break;
    case 5:
*/  
  }
}


//==============================
// Various Display Functions
//==============================

// Read and debounce the buttons form TM1638
uint8_t buttonsRead(void)
{
  uint8_t buttons = 0;
  unsigned long currentButtonMillis = millis();
  if (currentButtonMillis - debounce_previousMillis >= debounce_interval) {
    debounce_previousMillis = currentButtonMillis;
    buttons = tm.readButtons();
  }
  return buttons;
}

void clearText() {
  tm.displayText("        ");
}

void showTime() {
  String outTime;
  byte hours = 0;
  byte minutes = 0;
  bool pm = false;
  int colonLoc = haTime.indexOf(":");
  if (colonLoc > 0) {
    hours = (haTime.substring(0, colonLoc)).toInt();
    minutes = (haTime.substring(colonLoc + 1)).toInt();
    if (hours > 12) {
      hours = hours - 12;
      pm = true;
    } else if (hours == 12) {
      pm = true;
    } else if (hours == 0) {
      hours = 12;
      pm = false;
    }
    if (hours > 9) {
      outTime = "  " + String(hours) + ".";
    } else {
      outTime = "   " + String(hours) + ".";
    }
    if (minutes < 10) {
      outTime = outTime + "0" + String(minutes);
    } else {
      outTime = outTime + String(minutes);
    }
    if (pm) {
      outTime = outTime + " P";
    } else {
      outTime = outTime + " A";
    }
    
  } else {
    //No current time - hasn't updated from HA yet
    outTime = "--------";
  }
  tm.displayText(outTime.c_str());
}

void showDate() {
  String outDate;
  byte month = 0;
  String day;
  if (haDate.length() > 7) {
   month = (haDate.substring(5, 7)).toInt();
   day = (haDate.substring(8)).toInt();
   switch(month) {
     case 1:
       outDate = "JAN";
       break;
     case 2: 
       outDate = "FEB";
       break;
     case 3:
       outDate = "MAR";
       break;
     case 4:
       outDate = "APR";
       break;
     case 5:
       outDate = "MAY";
       break;
     case 6:
       outDate = "JUN";
       break;
     case 7:
       outDate = "JUL";
       break;
     case 8:
       outDate = "AUG";
       break;
     case 9:
       outDate = "SEP";
       break;
     case 10:
       outDate = "OCT";
       break;
     case 11:
       outDate = "NOV";
       break;
     case 12:
       outDate = "DEC";
       break;
   }
   if (day.length() > 0) {
     outDate = outDate + " " + day; 
   } else {
    outDate = outDate + " ----";
   }
   
  } else {
    outDate = "JAN .00.";
  }
  tm.displayText(outDate.c_str());
}

void blinkLights( byte prevLights) {
  if (ledsOn) {
    tm.setLEDs(0x0000);
  } else {
    //tm.setLEDs(3);
    //tm.setLEDs(0xFF00);
    if (prevLights == 1) {
      tm.setLEDs(0x0300);
    } else if (prevLights == 2) {
      tm.setLEDs(0x1800);
    } else if (prevLights == 3) {
      tm.setLEDs(0xC000);
    }
  }
  ledsOn = !ledsOn;
}
// =================================
//  MQTT PUBLISH/UPDATE ROUTINES
// =================================
void setAllLeds(String lights) {
  //Process each char of string and set each LED on/off accordingly
  char buf[10];
  lights.toCharArray(buf, 9);
  
  for (int i = 0; i < 8; i++) {
    byte ledState = buf[i];
    tm.setLED(i, ledState);
  }
}

void updateMQTTButtonState(uint8_t buttons) {
  char outMsg[9];
  sprintf(outMsg, "%1u", buttons);
  client.publish(MQTT_TOPIC_PUB"/buttons", outMsg, false);
}
void updateMQTTMode() {
  char outMode[3];
  sprintf(outMode, "%1u", displayMode);
  client.publish(MQTT_TOPIC_PUB"/mode", outMode, true);
}
void updateMQTTBrightness() {
  char outBright[3];
  sprintf(outBright, "%1u", curBrightness);
  client.publish(MQTT_TOPIC_PUB"/brightness", outBright, true);
}
