/*  esp_FanControl
Code used to control a Fanimation Slinger V2 RF Fan via an ESP8266 and a TI CC1101 chip over MQTT

ESP8266 CC1101 Wiring: https://github.com/LSatan/SmartRC-CC1101-Driver-Lib/blob/master/img/Esp8266_CC1101.png
*/

#include "config.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>

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
RCSwitch mySwitch = RCSwitch();

void FANCONTORL_LOGGER(String logMsg, int requiredLVL)
{
  if(requiredLVL <= FAN_DEBUG_LVL)
    Serial.printf("%s\n", logMsg.c_str());
}

void sendRFCommand(int code)
{
  //Turn Off Listening
  mySwitch.disableReceive();
  //Enable RF TX
  mySwitch.enableTransmit(TX_PIN);                                                   
  ELECHOUSE_cc1101.SetTx();
  FANCONTORL_LOGGER("[TX] Transmitting RF Code " + String(code), 2);
  mySwitch.setRepeatTransmit(RF_REPEATS);
  mySwitch.setProtocol(RF_PROTOCOL);
  mySwitch.setPulseLength(382);
  mySwitch.send(code, 24);
  FANCONTORL_LOGGER("[TX] Transmission Complete!", 2);
  mySwitch.disableTransmit();
}

//Call this function to get the codes for the buttons we want to use
void listenForCodes()
{
  if(mySwitch.available())
  {
    FANCONTORL_LOGGER("[LEARN] !!! BUTTON PRESS DETECTED !!!", 1);
    FANCONTORL_LOGGER("[LEARN] Code: "+String(mySwitch.getReceivedValue()), 1);
    FANCONTORL_LOGGER("[LEARN] Bit: "+String(mySwitch.getReceivedBitlength()), 1);
    FANCONTORL_LOGGER("[LEARN] Protocol: "+String(mySwitch.getReceivedProtocol()), 1);
    FANCONTORL_LOGGER("[LEARN] Delay: "+String(mySwitch.getReceivedDelay()), 1);
    mySwitch.resetAvailable();
  }
}

void setup() 
{
  //Start Serial Connection
  Serial.begin(115200);
  delay(2000);
  // Check the CC1101 Spi connection is working.
  if (ELECHOUSE_cc1101.getCC1101())
  {
    FANCONTORL_LOGGER("CC1101 SPI Connection: [OK]", 1);
    CC1101_CONNECTED = true;
  }
  else
  {
    FANCONTORL_LOGGER("CC1101 SPI Connection: [ERR]", 1);
    CC1101_CONNECTED = false;
  }
  //Initialize the CC1101 and Set the Frequency
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(FREQUENCY); 
  if(LEARNING_MODE)
  {
    //Put Device in Listening Mode
    mySwitch.enableReceive(RX_PIN);
    ELECHOUSE_cc1101.SetRx();
  }
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
      if(counter == 0)
        sendRFCommand(SUMMER_FAN_ON);
    }
    delay(1000);
  }
}
