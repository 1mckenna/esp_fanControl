/*  esp_FanControl
Code used to control a Fanimation Slinger V2 RF Fan via an ESP8266 and a TI CC1101 chip over MQTT

ESP8266 CC1101 Wiring: https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/blob/master/img/Esp8266_CC1101.png
*/

#include "config.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h> //for MQTT
#include <ArduinoJson.h>

// Set receive and transmit pin numbers (GDO0 and GDO2)
#ifdef ESP32 // for esp32! Receiver on GPIO pin 4. Transmit on GPIO pin 2.
  #define RX_PIN 4 
  #define TX_PIN 2
#elif ESP8266  // for esp8266! Receiver on pin 4 = D2. Transmit on pin 5 = D1.
  #define RX_PIN 4
  #define TX_PIN 5
#else // for Arduino! Receiver on interrupt 0 => that is pin #2. Transmit on pin 6.
  #define RX_PIN 0
  #define TX_PIN 6
#endif 

#define DELETE(ptr) { if (ptr != nullptr) {delete ptr; ptr = nullptr;} }

String INFO_TOPIC = (String)MQTT_BASETOPIC + "/fancontrol_" + String(ESP.getChipId()) + "/info";
String AVAILABILITY_TOPIC = (String)MQTT_BASETOPIC + "/fancontrol_" + String(ESP.getChipId()) + "/status";
String LIGHT_STATE_TOPIC = (String)MQTT_BASETOPIC + "/light/fancontrol_" + String(ESP.getChipId()) + "/state";
String LIGHT_COMMAND_TOPIC = (String)MQTT_BASETOPIC + "/light/fancontrol_" + String(ESP.getChipId()) + "/set";
String LIGHT_CONFIG_TOPIC = (String)MQTT_BASETOPIC + "/light/fancontrol_" + String(ESP.getChipId())+"/config";
String FAN_STATE_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId()) + "/state";
String FAN_COMMAND_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId()) + "/set";
String FAN_PERCENT_STATE_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId()) +"/speed/percentage_state";
String FAN_PERCENT_COMMAND_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId()) + "/speed/percentage";
String FAN_MODE_STATE_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId()) + "/preset/preset_mode_state";
String FAN_MODE_COMMAND_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId()) + "/preset_mode";
String FAN_CONFIG_TOPIC = (String)MQTT_BASETOPIC + "/fan/fancontrol_" + String(ESP.getChipId())+"/config";
bool SUMMER_MODE = true; //Default Fan state
bool CURRENT_LIGHT_STATE = false;
bool CURRENT_FAN_STATE = false;
int CURRENT_FAN_SPEED = 0;

bool CC1101_CONNECTED = false;
RCSwitch fanControlClient = RCSwitch();

WiFiClient *client = nullptr;
PubSubClient *mqtt_client = nullptr;
static String deviceStr ="";


String getSystemUptime()
{
  long millisecs = millis();
  int systemUpTimeMn = int((millisecs / (1000 * 60)) % 60);
  int systemUpTimeHr = int((millisecs / (1000 * 60 * 60)) % 24);
  int systemUpTimeDy = int((millisecs / (1000 * 60 * 60 * 24)) % 365);
  return String(systemUpTimeDy)+"d:"+String(systemUpTimeHr)+"h:"+String(systemUpTimeMn)+"m";
}


void FANCONTORL_LOGGER(String logMsg, int requiredLVL, bool addNewLine)
{
  if(requiredLVL <= FAN_DEBUG_LVL)
  {
    if(addNewLine)
      Serial.printf("%s\n", logMsg.c_str());
    else
      Serial.printf("%s", logMsg.c_str());
  }
}

void sendRFCommand(int code)
{
  fanControlClient.disableReceive(); //Turn Off Listening
  fanControlClient.enableTransmit(TX_PIN); //Enable RF TX                                                  
  ELECHOUSE_cc1101.SetTx();
  FANCONTORL_LOGGER("[TX] Transmitting RF Code " + String(code), 2, true);
  fanControlClient.setRepeatTransmit(RF_REPEATS);
  fanControlClient.setProtocol(RF_PROTOCOL);
  fanControlClient.setPulseLength(382);
  fanControlClient.send(code, 24);
  FANCONTORL_LOGGER("[TX] Transmission Complete!", 2, true);
  fanControlClient.disableTransmit();
  //Put Device Back in Listening Mode
  fanControlClient.enableReceive(RX_PIN);
  ELECHOUSE_cc1101.SetRx();
}

//Call this function to get the codes for the buttons we want to use
void startLearningMode()
{
  fanControlClient.enableReceive(RX_PIN);
  ELECHOUSE_cc1101.SetRx();
  if(fanControlClient.available())
  {
    FANCONTORL_LOGGER("[LEARN] !!! BUTTON PRESS DETECTED !!!", 1, true);
    FANCONTORL_LOGGER("[LEARN] Code: "+String(fanControlClient.getReceivedValue()), 1, true);
    FANCONTORL_LOGGER("[LEARN] Bit: "+String(fanControlClient.getReceivedBitlength()), 1, true);
    FANCONTORL_LOGGER("[LEARN] Protocol: "+String(fanControlClient.getReceivedProtocol()), 1, true);
    FANCONTORL_LOGGER("[LEARN] Delay: "+String(fanControlClient.getReceivedDelay()), 1, true);
    fanControlClient.resetAvailable();
  }
}

void listenForCodes()
{
  if(fanControlClient.available())
  {
    String rxCode = String(fanControlClient.getReceivedValue());
    FANCONTORL_LOGGER("[RX] Code: " + rxCode, 3, true);
    if(rxCode == String(LIGHT_ON))
      mqtt_client->publish(LIGHT_STATE_TOPIC.c_str(),"on",true);
    else if(rxCode == String(LIGHT_OFF))
      mqtt_client->publish(LIGHT_STATE_TOPIC.c_str(),"off",true);
    else if(rxCode == String(FAN_OFF))
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"off",true);
    else if(rxCode == String(SUMMER_FAN_ON) )
    {
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"on",true);
      mqtt_client->publish(FAN_MODE_STATE_TOPIC.c_str(),"Summer",true);
    }
    else if(rxCode == String(WINTER_FAN_ON))
    {
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"on",true);
      mqtt_client->publish(FAN_MODE_STATE_TOPIC.c_str(),"Winter",true);
    }
    else if( rxCode.toInt() >= LIGHT_MIN  && rxCode.toInt() <= LIGHT_MAX )
      mqtt_client->publish(LIGHT_STATE_TOPIC.c_str(),"on",true);
    else if( rxCode.toInt() >= SUMMER_FAN_MIN  && rxCode.toInt() <= SUMMER_FAN_MAX )
    {
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"on",true);
      int speed = rxCode.toInt() - SUMMER_FAN_MIN;
      mqtt_client->publish(FAN_PERCENT_STATE_TOPIC.c_str(),String(speed).c_str(),true);
    }
    else if( rxCode.toInt() >= WINTER_FAN_MIN  && rxCode.toInt() <= WINTER_FAN_MAX )
    {
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"on",true);
      int speed = rxCode.toInt() - WINTER_FAN_MIN;
      mqtt_client->publish(FAN_PERCENT_STATE_TOPIC.c_str(),String(speed).c_str(),true);
    }
    else if(rxCode == String(SUMMER_FAN_MODE))
    {
      mqtt_client->publish(FAN_MODE_STATE_TOPIC.c_str(),"Summer",true);
    }
    else if(rxCode == String(WINTER_FAN_MODE))
    {
      mqtt_client->publish(FAN_MODE_STATE_TOPIC.c_str(),"Winter",true);
    }
    fanControlClient.resetAvailable();
  }
}

#pragma region MQTT_Related_Functions
void callback(char* topic, byte* payload, unsigned int length) 
{
  String payloadStr = "";
  for (int i=0;i<length;i++)
  {
    payloadStr += (char)payload[i];
  }
  FANCONTORL_LOGGER("Message arrived ["+String(topic)+"] ("+ payloadStr +")", 3, true);
  if(String(topic) == FAN_STATE_TOPIC )
  {
    if(payloadStr == "on")
      CURRENT_FAN_STATE = true;
    else if(payloadStr == "off" )
      CURRENT_FAN_STATE = false;
  }
  else if(String(topic) == LIGHT_STATE_TOPIC )
  {
    if(payloadStr == "on")
      CURRENT_LIGHT_STATE = true;
    else if(payloadStr == "off" )
      CURRENT_LIGHT_STATE = false;
  }
  else if( String(topic) == FAN_PERCENT_STATE_TOPIC )
  {
    CURRENT_FAN_SPEED = payloadStr.toInt();
  }
  else if( String(topic) == FAN_MODE_STATE_TOPIC )
  {
    if(payloadStr == "Summer")
    {
      SUMMER_MODE = true;
    }
    else if(payloadStr == "Winter")
    {
      SUMMER_MODE = false;
    }
  }
  else if(String(topic) == LIGHT_COMMAND_TOPIC )
  {
    if(payloadStr == "on")
    {
      sendRFCommand(LIGHT_ON);
      mqtt_client->publish(LIGHT_STATE_TOPIC.c_str(),"on",true);
    }
    if(payloadStr == "off")
    {
      sendRFCommand(LIGHT_OFF);
      mqtt_client->publish(LIGHT_STATE_TOPIC.c_str(),"off",true);
    }
  }
  else if(String(topic) == FAN_COMMAND_TOPIC )
  {
    if(payloadStr == "on")
    {
      if(SUMMER_MODE)
        sendRFCommand(SUMMER_FAN_ON);
      else
        sendRFCommand(WINTER_FAN_ON);
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"on",true);
    }
    if(payloadStr == "off")
    {
      sendRFCommand(FAN_OFF);
      mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"off",true);
    }      
  }
  else if( String(topic) == FAN_PERCENT_COMMAND_TOPIC )
  {
    int fanSpeed = payloadStr.toInt();
    mqtt_client->publish(FAN_STATE_TOPIC.c_str(),"on",true);
    mqtt_client->publish(FAN_PERCENT_STATE_TOPIC.c_str(),payloadStr.c_str(),true);
    if(SUMMER_MODE)
    {
      int txCode = SUMMER_FAN_MIN + fanSpeed; 
      sendRFCommand(txCode);
    }
    else
    {
      int txCode = WINTER_FAN_MIN + fanSpeed;
      sendRFCommand(txCode);
    }
  }
  else if( String(topic) == FAN_MODE_COMMAND_TOPIC )
  {
    if(payloadStr == "Summer")
    {
      if(CURRENT_FAN_STATE)
        FANCONTORL_LOGGER("!! FAN MUST BE OFF TO CHANGE MODES !!",0,true); 
      else
      {
        sendRFCommand(SUMMER_FAN_MODE);
        SUMMER_MODE = true;
        mqtt_client->publish(FAN_MODE_STATE_TOPIC.c_str(),payloadStr.c_str(),true);
        
      }
    }
    else if(payloadStr == "Winter")
    {
      if(CURRENT_FAN_STATE)
        FANCONTORL_LOGGER("!! FAN MUST BE OFF TO CHANGE MODES !!",0,true); 
      else
      {
        sendRFCommand(WINTER_FAN_MODE);
        SUMMER_MODE = false;
        mqtt_client->publish(FAN_MODE_STATE_TOPIC.c_str(),payloadStr.c_str(),true);
      }
    }
  }
  else
    FANCONTORL_LOGGER("UNKNOWN PAYLOAD: " + payloadStr,0,true);  
}

void disconnectMQTT()
{
  try
  {
    if (mqtt_client != nullptr)
    {
      if(mqtt_client->connected())
      {
        mqtt_client->disconnect();
      }
      DELETE(mqtt_client);
    }
  }
  catch(...)
  {
    FANCONTORL_LOGGER("Error disconnecting MQTT",0,true);
  }
}

//Connect to MQTT Server
void connectMQTT() 
{
  FANCONTORL_LOGGER("Connecting to MQTT...", 0,true);
  if (client == nullptr)
    client = new WiFiClient();
  if(mqtt_client == nullptr)
  {
    mqtt_client = new PubSubClient(*client);
    mqtt_client->setBufferSize(2048); //Needed as some JSON messages are too large for the default size
    mqtt_client->setKeepAlive(60); //Added to Stabilize MQTT Connection
    mqtt_client->setSocketTimeout(60); //Added to Stabilize MQTT Connection
    mqtt_client->setServer(MQTT_SERVER, atoi(MQTT_SERVERPORT));
    mqtt_client->setCallback(&callback);
  }
  if (!mqtt_client->connect(String(ESP.getChipId()).c_str(), MQTT_USERNAME, MQTT_PASSWORD, AVAILABILITY_TOPIC.c_str(), 1, true, "offline"))
  {
    FANCONTORL_LOGGER("MQTT connection failed: " + String(mqtt_client->state()), 0,true);
    DELETE(mqtt_client);
    delay(1*5000); //Delay for 5 seconds after a connection failure
  }
  else
  {
    FANCONTORL_LOGGER("MQTT connected", 1,true);
    mqtt_client->publish(AVAILABILITY_TOPIC.c_str(),"online");
    delay(500);
    mqtt_client->subscribe(LIGHT_STATE_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(LIGHT_COMMAND_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(FAN_STATE_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(FAN_COMMAND_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(FAN_PERCENT_STATE_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(FAN_PERCENT_COMMAND_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(FAN_MODE_STATE_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
    mqtt_client->subscribe(FAN_MODE_COMMAND_TOPIC.c_str());
    mqtt_client->loop();
    mqtt_client->loop();
  }
}

//Publish fanControl Client info to MQTT
void publishSystemInfo()
{
  if(mqtt_client)
  {
    if(mqtt_client->connected())
    {
      FANCONTORL_LOGGER("==== espFanControl Internal State ==== ",3,true);
      if(SUMMER_MODE)
        FANCONTORL_LOGGER("[MODE]: Summer",3,true);
      else
        FANCONTORL_LOGGER("[MODE]: Winter",3,true);
      if(CURRENT_LIGHT_STATE)
        FANCONTORL_LOGGER("[LIGHT]: ON",3,true);
      else
        FANCONTORL_LOGGER("[LIGHT]: OFF",3,true);
      if(CURRENT_FAN_STATE)
        FANCONTORL_LOGGER("[FAN]: ON",3,true);
      else
        FANCONTORL_LOGGER("[FAN]: OFF",3,true);
      FANCONTORL_LOGGER("[SPEED]: "+String(CURRENT_FAN_SPEED),2,true);
      mqttAnnounce();
    }
    else
      connectMQTT();
  }
  else
    connectMQTT();
}

//Publish MQTT Configuration Topics used by MQTT Auto Discovery
void mqttAnnounce()
{
  String syspayload="";
  String lightPayload ="";
  String fanPayload = "";

  DynamicJsonDocument deviceJSON(1024);
  JsonObject deviceObj = deviceJSON.createNestedObject("device");
  deviceObj["identifiers"] = String(ESP.getChipId());
  deviceObj["manufacturer"] = String(ESP.getFlashChipVendorId());
  deviceObj["model"] = "ESP8266";
  deviceObj["name"] = "fancontrol_" + String(ESP.getChipId());
  deviceObj["sw_version"] = "1.0";
  serializeJson(deviceObj, deviceStr);
  
  DynamicJsonDocument sysinfoJSON(1024);
  sysinfoJSON["device"] = deviceObj;
  sysinfoJSON["name"] = "fancontrol_" + String(ESP.getChipId());
  sysinfoJSON["Uptime"] = getSystemUptime();
  sysinfoJSON["Network"] = WiFi.SSID();
  sysinfoJSON["Signal Strength"] = String(WiFi.RSSI());
  sysinfoJSON["IP Address"] = WiFi.localIP().toString();
  serializeJson(sysinfoJSON,syspayload);

  DynamicJsonDocument lightJSON(1024);
  lightJSON["device"] = deviceObj;
  lightJSON["name"] = "fancontrol_" + String(ESP.getChipId()) + " Light";
  lightJSON["unique_id"]   = "fancontrol_" + String(ESP.getChipId())+"_light";
  lightJSON["state_topic"] = LIGHT_STATE_TOPIC;
  lightJSON["command_topic"] = LIGHT_COMMAND_TOPIC;
  lightJSON["payload_on"] = "on";
  lightJSON["payload_off"] = "off";
  lightJSON["availability_topic"] = AVAILABILITY_TOPIC;
  lightJSON["payload_available"] = "online";
  lightJSON["payload_not_available"] = "offline";
  serializeJson(lightJSON,lightPayload);
  
  DynamicJsonDocument fanJSON(1024);
  fanJSON["device"] = deviceObj;
  fanJSON["name"] = "fancontrol_" + String(ESP.getChipId()) + " Fan";
  fanJSON["unique_id"]   = "fancontrol_" + String(ESP.getChipId())+"_fan";
  fanJSON["state_topic"] = FAN_STATE_TOPIC;
  fanJSON["command_topic"] = FAN_COMMAND_TOPIC;
  fanJSON["payload_on"] = "on";
  fanJSON["payload_off"] = "off";
  fanJSON["percentage_state_topic"]  = FAN_PERCENT_STATE_TOPIC;
  fanJSON["percentage_command_topic"] = FAN_PERCENT_COMMAND_TOPIC;
  fanJSON["speed_range_min"] = 1;
  fanJSON["speed_range_max"] = 30;
  fanJSON["preset_mode_state_topic"] = FAN_MODE_STATE_TOPIC;
  fanJSON["preset_mode_command_topic"]= FAN_MODE_COMMAND_TOPIC;
  JsonArray FAN_PRESET_MODES = fanJSON.createNestedArray("preset_modes");
  FAN_PRESET_MODES.add("Summer");
  FAN_PRESET_MODES.add("Winter");
  fanJSON["availability_topic"] = AVAILABILITY_TOPIC;
  fanJSON["payload_available"] = "online";
  fanJSON["payload_not_available"] = "offline";
  serializeJson(fanJSON,fanPayload);

  if(mqtt_client)
  {
    if(mqtt_client->connected())
    {
      mqtt_client->publish(LIGHT_CONFIG_TOPIC.c_str(),lightPayload.c_str(),true);
      delay(100);
      mqtt_client->publish(FAN_CONFIG_TOPIC.c_str(),fanPayload.c_str(),true);
      delay(100);
      mqtt_client->publish(AVAILABILITY_TOPIC.c_str(),"online");
      delay(100);
      mqtt_client->publish(INFO_TOPIC.c_str(),syspayload.c_str());
      delay(100);
    }
    else
    {
      connectMQTT();
    }
  }
}

#pragma endregion

void check_status()
{
  static ulong checkstatus_timeout  = 0;
  static ulong LEDstatus_timeout    = 0;
  static ulong checkwifi_timeout    = 0;
  static ulong fancontrolheartbeat_timeout = 0;
  
  ulong current_millis = millis();

  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0))
  {
    check_WiFi();
    mqtt_client->loop();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  if ((current_millis > LEDstatus_timeout) || (LEDstatus_timeout == 0))
  {
    // Toggle LED at LED_INTERVAL = 2s
    toggleLED();
    LEDstatus_timeout = current_millis + LED_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
  { 
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }

  // Print FanControl System Info every FANCONTROLHEARTBEAT_INTERVAL (5) minutes.
  if ((current_millis > fancontrolheartbeat_timeout) || (fancontrolheartbeat_timeout == 0))
  {
    publishSystemInfo();
    fancontrolheartbeat_timeout = current_millis + FANCONTROLHEARTBEAT_INTERVAL;
  }
}

//Toggle LED State
void toggleLED()
{
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void heartBeatPrint()
{
  if(mqtt_client != nullptr) //We have to check and see if we have a mqtt client created
  {
    if(!mqtt_client->connected())
    {
      FANCONTORL_LOGGER("MQTT Disconnected", 0, true);
      connectMQTT();
    }
  }
  FANCONTORL_LOGGER(String("free heap memory: ") + String(ESP.getFreeHeap()), 3, true);
}

void check_WiFi()
{
  if ( (WiFi.status() != WL_CONNECTED) )
  {
    FANCONTORL_LOGGER("WiFi Connection Lost!",0,true);
    disconnectMQTT();
    connectWiFi();
  }
}

void connectWiFi() 
{
  delay(10);
  FANCONTORL_LOGGER("Connecting to "+String(WIFI_SSID),0, false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    FANCONTORL_LOGGER(".",0, false);
  }
  FANCONTORL_LOGGER("",0,true);
  randomSeed(micros());
  FANCONTORL_LOGGER("WiFi connected",0,true);
  FANCONTORL_LOGGER("IP: "+ WiFi.localIP().toString(),0,true);
  
}

void setup() 
{
  //Start Serial Connection
  Serial.begin(115200);
  while (!Serial);
  delay(200);
  FANCONTORL_LOGGER("Starting fanControl Client on " + String(ARDUINO_BOARD), 0, true);
  // Initialize the LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Check the CC1101 Spi connection is working.
  if (ELECHOUSE_cc1101.getCC1101())
  {
    FANCONTORL_LOGGER("CC1101 SPI Connection: [OK]", 1, true);
    CC1101_CONNECTED = true;
  }
  else
  {
    FANCONTORL_LOGGER("CC1101 SPI Connection: [ERR]", 1, true);
    CC1101_CONNECTED = false;
  }
  //Initialize the CC1101 and Set the Frequency
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(FREQUENCY); 
  //Put Device in Listening Mode
  fanControlClient.enableReceive(RX_PIN);
  ELECHOUSE_cc1101.SetRx();
  connectWiFi();
  connectMQTT();
}

void loop() 
{
  //Make sure the CC1101 Chip is connected otherwise do nothing
  if(CC1101_CONNECTED)
  {
    int counter = 0;
    if(LEARNING_MODE) 
      startLearningMode();
    else
    {
      //Check to see if someone is using the remote (We only TX when told via MQTT)
      listenForCodes();
    }
  }
  check_status();
}
