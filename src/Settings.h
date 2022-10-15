/* =====================================================================================
   Update/Add any #define values to match your build and board type if not using D1 Mini
   =====================================================================================
   See the wiki for more information on these settings: https://github.com/Resinchem/TM1638-MQTT/wiki
*/
// pin definitions
#define  STROBE_TM D5         // strobe = GPIO connected to strobe line of module
#define  CLOCK_TM D6          // clock = GPIO connected to clock line of module
#define  DIO_TM D7            // data = GPIO connected to data line of module

// WIFI MODE must be 2 and MQTTMODE must be 1 for this application to provide MQTT functionality
#define WIFIMODE 2                            // 0 = Only Soft Access Point, 1 = Only connect to local WiFi network with UN/PW, 2 = Both
#define MQTTMODE 1                            // 0 = Disable MQTT, 1 = Enable (will only be enabled if WiFi mode = 1 or 2 - broker must be on same network)

#define SERIAL_DEBUG 1                        // 0 = Disable (must be disabled if using RX/TX pins), 1 = enable

//You may change any of these as desired for your environment
#define WIFIHOSTNAME "TM1638_mqtt"            // Host name for WiFi/Router
#define MQTTCLIENT "tm1638mqtt"               // MQTT Client Name
#define MQTT_TOPIC_SUB "cmnd/tm1638"          // Default MQTT subscribe topic for commands to this device
#define MQTT_TOPIC_PUB "stat/tm1638"          // Default MQTT publish topic
#define OTA_HOSTNAME "TM1638_mqtt_OTA"        // Hostname to broadcast as port in the IDE of OTA Updates

// ---------------------------------------------------------------------------------------------------------------
// Options - Defaults upon boot-up or any other custom ssttings
// ---------------------------------------------------------------------------------------------------------------
// OTA Settings
bool ota_flag = true;                    // Must leave this as true for board to broadcast port to IDE upon boot
uint16_t ota_boot_time_window = 2500;    // minimum time on boot for IP address to show in IDE ports, in millisecs
uint16_t ota_time_window = 20000;        // time to start file upload when ota_flag set to true (after initial boot), in millsecs

// Other options
bool high_freq = true;                  // If using a high freq CPU > ~100 MHZ set to true (e.g ESP8266, ESP32). 
uint8_t default_brightness = 5;         // default brightness at boot.  Valid values are 0-7.
byte displayMode = 1;                   // default mode at boot. 0 = none/updates via MQTT, 1 = clock, 2 = date // Add your own after this
bool showAlerts = false;                // Show alerts (flash LEDs) - reserved for future use
uint16_t blinkInterval = 1000;          // Sets interval speed for blinking LEDs, in milliseconds
