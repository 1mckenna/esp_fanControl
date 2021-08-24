//Configuration Settings for the ESP FanControl Device
//These codes are specific to the Fanimation Slinger V2 with DIP switches set to [OFF] [OFF] [ON] [OFF] [ON]
#define DELETE(ptr) { if (ptr != nullptr) {delete ptr; ptr = nullptr;} }
//SERIAL LOGGING LEVEL FOR DEBUGGING (default: 1)
#define FAN_DEBUG_LVL 2
// Frequency of Fanimation Slinger V2 Fan Remote
#define FREQUENCY     304.25
// RC-switch settings
#define RF_PROTOCOL 6
#define RF_REPEATS  8

//Enable learning mode initally to capture the correct signals for each of the different codes in the below section 
#define LEARNING_MODE false
//RF CODE SECTION (GET THESE VALUES USING LEARNING MODE AND THE REMOTE)

//LIGHT RELATED CODES
#define LIGHT_ON 6834 //RF CODE FOR LIGHT_ON 
#define LIGHT_OFF 6784  //RF CODE FOR LIGHT_OFF
#define LIGHT_MIN 6789 //RF CODE FOR DIMMEST LIGHT LEVEL
#define LIGHT_MAX 6830 //RF CODE FOR MAX LIGHT LEVEL
//GENERIC FAN RELATED CODES
#define FAN_OFF 6656 //RF CODE FOR TURNING FAN OFF (Both in Summer and Winter Mode)
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

//MQTT SETTINGS
#define MQTT_SERVER              "127.0.0.1"
#define MQTT_SERVERPORT          "1883"
#define MQTT_USERNAME            "mqtt"
#define MQTT_PASSWORD            "password"
#define MQTT_BASETOPIC           "homeassistant" //If you are using Home Assistant you should set this to your discovery_prefix (default: homeassistant)

//WIFI SETTINGS
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
#define WIFICHECK_INTERVAL           1000L
#define LED_INTERVAL                 2000L
#define HEARTBEAT_INTERVAL          10000L
#define FANCONTROLHEARTBEAT_INTERVAL   60000L