### Source Code Files

This folder contains the primary files for the TM1638-MQTT application.  All three files are required. You must modify at least the Credentials.h file with your particular wifi and MQTT broker information before installation.

* Credentials.h - contains connection details for wifi and MQTT
* Settings.h - contains other settings and options, including MQTT topics to use
* tm1638_mqtt.ino - primary application file

Other supporting libraries, such as the TM1638Plus and PubSubClient libraries, may also need to be installed in your environment along with support for the ESP8266 boards.  **Please see the wiki for full details on installation, setup and other requirements**.

This application is still a very-early work-in-progress.
