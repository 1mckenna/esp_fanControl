# ESP fanControl
Control and Monitor a Fanimation Slinger V2 RF Fan via an ESP8266 and a TI CC1101 chip over MQTT.

*This code base should work for other fans controlled via RF with small changes to the protocols/codes being sent.*

# Table of Contents
- [ESP fanControl](#esp-fancontrol)
- [Features](#features)
- [ESP8266 Wiring Setup (i.e. Connecting to the TI CC1101)](#esp8266-wiring-setup--ie-connecting-to-the-ti-cc1101-)
- [Ardunio IDE/VSCode Board Specific Settings Setup](#ardunio-ide-vscode-board-specific-settings-setup)
  * [Arduino IDE](#arduino-ide)
  * [VSCode](#vscode)
  * [Install Required Libraries](#install-required-libraries)
- [Clone, Initial Configuration, and Flash ESP8266](#clone--initial-configuration--and-flash-esp8266)
  * [Clone the Repo](#clone-the-repo)
  * [Perform Initial Configuration](#perform-initial-configuration)
  * [Flash the device](#flash-the-device)
- [Learning Mode](#learning-mode)
- [Home Assistant Information](#home-assistant-information)
  * [Device View](#device-view)
  * [Detailed View](#detailed-view)
  * [Entity Views](#entity-views)
    + [Sensor](#sensor)
    + [Light](#light)
    + [Fan](#fan)
- [MQTT Information](#mqtt-information)
  * [MQTT Topics Published](#mqtt-topics-published)
  * [MQTT Retained Topics](#mqtt-retained-topics)
---
# Features
* Learning Mode
  * Useful for determining what codes you need to send to your fan
* Light ON/OFF Control
* Fan ON/OFF Control
* Fan Speed Control
* Fan Direction Control (i.e. winter/summer modes)
* Fan State Monitoring
  * Monitor for changes to fan state being made with official Fanimation RF Remote
* MQTT Autodiscovery
  * Supports MQTT Autodiscovery for automatic entity creation in Home Assistant and other Home Automation Systems
---
# ESP8266 Wiring Setup (i.e. Connecting to the TI CC1101)
To see how to connect the TI CC1101 chip to the ESP8266 please see the following wiring diagram.
* [ESP8266 TICC1101 Wiring Diagram](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/blob/master/img/Esp8266_CC1101.png)
---
# Ardunio IDE/VSCode Board Specific Settings Setup
In order to flash this code you need to enable C++ exceptions for your board if not already enabled.

## Arduino IDE
1. Loading the Sketch
2. Select the correct board (Generic ESP8266 Module)
3. Tools -> C++ Exceptions -> Enabled

## VSCode
1. Load the Sketch
2. Open the Board Manager
3. Scroll Down to the Section C++ Exceptions and set to Enabled

## Install Required Libraries
You can install these libraries via the Library Manager in Arduino IDE or VSCode
* [SmartRC-CC1101-Driver-Lib by LSatan](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib)
* [rc-switch by sui77](https://github.com/sui77/rc-switch)
* [PubSubClient by Nick O'Leary](https://pubsubclient.knolleary.net/)

---
# Clone, Initial Configuration, and Flash ESP8266
## Clone the Repo
1. Open the termnial and cd to your Arduino project folder
    * `cd ~/Ardunio`
2. Clone this repo
    * `git clone https://github.com/1mckenna/esp_fanControl.git`

## Perform Initial Configuration
Before flashing the code to your device you first need to make some updates to the `config.h` file.

## Flash the device
After saving any changes you've made to the `config.h` file you can now flash the device in the Arduino IDE.

**Required Modifications:**
* MQTT Settings (`config.h` [lines 36-40] )
    ```
    #define MQTT_SERVER              "127.0.0.1"
    #define MQTT_SERVERPORT          "1883"
    #define MQTT_USERNAME            "mqtt"
    #define MQTT_PASSWORD            "password"
    #define MQTT_BASETOPIC           "homeassistant" 
    ```
* Wifi Settings (`config.h` [lines 43-44] )
    ```
    #define WIFI_SSID "your_wifi_ssid"
    #define WIFI_PASS "your_wifi_password"
    ```

**If you are using another fan or have configured your DIP switches differently please see the Learning Mode section to ensure you are sending the proper commands**

---
# Learning Mode
To put the device into learning mode you first need to edit line 13 in your `config.h` file to look like the following.
```
#define LEARNING_MODE true
```

Once you have enabled learning mode, connect the ESP8266 you will want to flash the modified code, and then connect over the serial port. 

Then use the RF remote that came with your fan and note down the information displayed for each button press you want to map.

**Learning Mode Sample Output**
```
[Starting] Opening the serial port - /dev/ttyUSB0
CC1101 SPI Connection: [OK]
[LEARN] !!! BUTTON PRESS DETECTED !!!
[LEARN] Code: 6784
[LEARN] Bit: 24
[LEARN] Protocol: 6
[LEARN] Delay: 383
[LEARN] !!! BUTTON PRESS DETECTED !!!
[LEARN] Code: 6834
[LEARN] Bit: 24
[LEARN] Protocol: 6
[LEARN] Delay: 383
```

Once you have all the required information make the necessary changes in the `config.h` file to ensure you are sending the proper codes for each of the commands listed (lines 17-33).
```
#define LIGHT_ON 6834   //RF CODE FOR LIGHT_ON 
#define LIGHT_OFF 6784  //RF CODE FOR LIGHT_OFF
#define LIGHT_MIN 6789  //RF CODE FOR DIMMEST LIGHT LEVEL
#define LIGHT_MAX 6830  //RF CODE FOR MAX LIGHT LEVEL
//GENERIC FAN RELATED CODES
#define FAN_OFF 6656    //RF CODE FOR TURNING FAN OFF (Both in Summer and Winter Mode)
//FAN MODE RELATED CODES
#define SUMMER_FAN_MODE 6754
#define WINTER_FAN_MODE 6755
//SUMMER SPECIFIC (i.e. fan pushing air down)
#define SUMMER_FAN_ON 6687
#define SUMMER_FAN_MAX 6687
#define SUMMER_FAN_MIN 6657
//WINTER SPECIFIC (i.e. fan pulling air up)
#define WINTER_FAN_ON 6751
#define WINTER_FAN_MAX 6751
#define WINTER_FAN_MIN 6721
```

Additionally, if you are not using a Fanimation Slinger V2 fan or the Protocol is not 6 you will need to make modifications to lines 8-10 in the `config.h` file.
```
// RC-switch settings
#define RF_PROTOCOL 6
#define RF_REPEATS  8
```
---
# Home Assistant Information
After you have configured the esp_fanControl Client and have it connected to the same MQTT Broker as your Home Assistant instance the device should automatically appear.

## Device View
![esp_fanControl_ha_device.png](https://github.com/1mckenna/esp_fanControl/blob/main/images/esp_fanControl_ha_device.png?raw=true)
## Detailed View
![esp_fanControl_ha_device_details.png](https://github.com/1mckenna/esp_fanControl/blob/main/images/esp_fanControl_ha_device_details.png?raw=true)

## Entity Views
![esp_fanControl_ha_entities.png](https://github.com/1mckenna/esp_fanControl/blob/main/images/esp_fanControl_ha_entities.png?raw=true)
### Sensor
![esp_fanControl_ha_sensor.png](https://github.com/1mckenna/esp_fanControl/blob/main/images/esp_fanControl_ha_sensor.png?raw=true)

### Light
![esp_fanControl_ha_light.png](https://github.com/1mckenna/esp_fanControl/blob/main/images/esp_fanControl_ha_light.png?raw=true)

### Fan
![esp_fanControl_ha_fan.png](https://github.com/1mckenna/esp_fanControl/blob/main/images/esp_fanControl_ha_fan.png?raw=true)

---
# MQTT Information
## MQTT Topics Published
| **MQTT Topic** | **Value(s)** |
| :--------------------: | :--------------------: |
|MQTT_BASETOPIC/sensor/fancontrol_<i>espChipID</i>/status | online: MQTT Connected</br>offline: MQTT Disconnected</br>**This is the availability topic**|
|MQTT_BASETOPIC/sensor/fancontrol_<i>espChipID</i>/info/attributes | System Information about esp_fanControl Device</br><i>(Device Name, Uptime, Wifi Network, Wifi Signal Strength, IP Address)</i>|
|MQTT_BASETOPIC/sensor/fancontrol_<i>espChipID</i>/info/config | MQTT Autoconfiguration Settings for fanControl Device Sensor |
|MQTT_BASETOPIC/light/fancontrol_<i>espChipID</i>/state | Current Light State (on,off) |
|MQTT_BASETOPIC/light/fancontrol_<i>espChipID</i>/set | Light Command Topic (on,off) |
|MQTT_BASETOPIC/light/fancontrol_<i>espChipID</i>/config | MQTT Autoconfiguration Settings for fanControl Light  |
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/state | Current Fan Power State (on,off) |
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/set | Fan Command Topic (on,off) |
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/config | MQTT Autoconfiguration Settings for fanControl Fan |
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/speed/percentage_state | Current Fan Speed State (min: 1, max: 30) |
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/speed/percentage | Fan Speed Command Topic (min: 1, max: 30)|
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/preset/preset_mode_state | Current Fan Mode State ( Summer,Winter) |
|MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/preset_mode | Fan Mode Command Topic ( Summer,Winter)|

## MQTT Retained Topics
The below are the list of topics that have the retain flag set on them.
* MQTT_BASETOPIC/sensor/fancontrol_<i>espChipID</i>/info/attributes
* MQTT_BASETOPIC/sensor/fancontrol_<i>espChipID</i>/info/config
* MQTT_BASETOPIC/light/fancontrol_<i>espChipID</i>/config
* MQTT_BASETOPIC/light/fancontrol_<i>espChipID</i>/state 
* MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/config
* MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/preset/preset_mode_state
* MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/speed/percentage_state
* MQTT_BASETOPIC/fan/fancontrol_<i>espChipID</i>/state
 