/*  esp_FanControl
Code used to control a Fanimation Slinger V2 RF Fan via an ESP8266 and a TI CC1101 chip over MQTT

ESP8266 CC1101 Wiring: https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/blob/master/img/Esp8266_CC1101.png
*/

#include "config.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h> //for MQTT

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

bool CC1101_CONNECTED = false;
RCSwitch fanControlClient = RCSwitch();

WiFiClient *client = nullptr;
PubSubClient *mqtt_client = nullptr;

void FANCONTORL_LOGGER(String logMsg, int requiredLVL, bool addNewLine)
{
  if(requiredLVL <= FAN_DEBUG_LVL)
  {
    if(addNewLine)
      Serial.printf("%s", logMsg.c_str());
    else
      Serial.printf("%s\n", logMsg.c_str());
  }
}

void sendRFCommand(int code)
{
  //Turn Off Listening
  fanControlClient.disableReceive();
  //Enable RF TX
  fanControlClient.enableTransmit(TX_PIN);                                                   
  ELECHOUSE_cc1101.SetTx();
  FANCONTORL_LOGGER("[TX] Transmitting RF Code " + String(code), 2, true);
  fanControlClient.setRepeatTransmit(RF_REPEATS);
  fanControlClient.setProtocol(RF_PROTOCOL);
  fanControlClient.setPulseLength(382);
  fanControlClient.send(code, 24);
  FANCONTORL_LOGGER("[TX] Transmission Complete!", 2, true);
  fanControlClient.disableTransmit();
}

//Call this function to get the codes for the buttons we want to use
void listenForCodes()
{
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

#pragma region MQTT_Related_Functions
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
  String lastWillTopic = (String)MQTT_BASETOPIC + "/sensor/fancontrol_" + String(ESP.getChipId()) + "/status";
  FANCONTORL_LOGGER("Connecting to MQTT...", 0,true);
  if (client == nullptr)
    client = new WiFiClient();
  if(mqtt_client == nullptr)
  {
    mqtt_client = new PubSubClient(*client);
    mqtt_client->setBufferSize(1024); //Needed as some JSON messages are too large for the default size
    mqtt_client->setKeepAlive(60); //Added to Stabilize MQTT Connection
    mqtt_client->setSocketTimeout(60); //Added to Stabilize MQTT Connection
    mqtt_client->setServer(MQTT_SERVER, atoi(MQTT_SERVERPORT));
  }
  if (!mqtt_client->connect(String(ESP.getChipId()).c_str(), MQTT_USERNAME, MQTT_PASSWORD, lastWillTopic.c_str(), 1, true, "offline"))
  {
    FANCONTORL_LOGGER("MQTT connection failed: " + String(mqtt_client->state()), 0,true);
    DELETE(mqtt_client);
    delay(1*5000); //Delay for 5 seconds after a connection failure
  }
  else
  {
    FANCONTORL_LOGGER("MQTT connected", 1,true);
    mqtt_client->publish(lastWillTopic.c_str(),"online");
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

void publishSystemInfo()
{

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
      FANCONTORL_LOGGER("MQTT Disconnected", 2, true);
      connectMQTT();
    }
  }
  FANCONTORL_LOGGER(String("free heap memory: ") + String(ESP.getFreeHeap()), 2, true);
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

  if(LEARNING_MODE)
  {
    //Put Device in Listening Mode
    fanControlClient.enableReceive(RX_PIN);
    ELECHOUSE_cc1101.SetRx();
  }

  connectWiFi();
  //connectMQTT();
}

void loop() 
{
  //Make sure the CC1101 Chip is connected otherwise do nothing
  if(CC1101_CONNECTED)
  {
    int counter = 0;
    if(LEARNING_MODE) 
      listenForCodes();
    else
    {
      

    }
  }
  check_status();
}
