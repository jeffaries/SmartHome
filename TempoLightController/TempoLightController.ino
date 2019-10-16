

/***************************************************
Generic ESP8266 Module, 80Mhz, Flash, ck, 26 Mhz, 40 Mhz, QIO, 1M (128 SPIFFS), 2, v2 Lower Memory, Serial, OTA, All Flash Contents, 115200 on 192.168.3.13
Board : esp8286 by ESP8286 Community. Version 2.5.0
****************************************************/

/*********************/
/*     Pin Mapping   */
/*     for D1 mini   */
/*********************/

// Digital Pin to GPIO mapping for Wemos D1 mini :

#define D0  16  // D0  => GPIO16
#define D1   5  // D1  => GPIO05 (SCL)
#define D2   4  // D2  => GPIO04 (SDA)
#define D3   0  // D3  => GPIO00
#define D4   2  // D4  => GPIO02 (LED builtin)
#define D5  14  // D5  => GPIO14
#define D6  12  // D6  => GPIO12
#define D7  13  // D7  => GPIO13
#define D8  15  // D8  => GPIO15
#define D9   3  // D9  => GPIO03 (RX)
#define D10  1  // D10 => GPIO01 (TX)

/*********************/
/*     Libraries     */
/*********************/

#include <math.h>

// Config file
#include <FS.h>          //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

// WifiManager
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

//Webserver
#include <ESP8266WebServer.h>

// OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif



// MQTT
#include <MQTTClient.h>  // https://github.com/256dpi/arduino-mqtt by JoÃƒÂ«l GÃƒÂ¤hwiler


/*********************/
/*     Parameters    */
/*********************/

#define SERIAL_SPEED          115200

// Module Name
#define MODULE_NAME           "mqttsensor"

// Default MQTT params
#define MQTT_SERVER           "broker.hivemq.com"
#define MQTT_SERVERPORT       "1883" // use 8883 for SSL
#define MQTT_USERNAME         "not_used"
#define MQTT_KEY              "not_used"
#define MQTT_PREFIX           "smarthome"
#define MQTT_PUBLISH_INTERVAL 60 // seconds

// Default Wifi config for AP Mode
#define WLAN_SSID       "SmartHome"
#define WLAN_PASS       "smarthome"



// SHT30 Sensor wiring
//TODO: Inverser DHTPIN et RELAYPIN sur PCB -> Relai sur GPIO2 et DHT sur GPIO0
#define INPUTPIN            D3
#define RELAYPIN            D4 
#define LEDPIN              D5 

/*********************/
/*     Global vars   */
/*********************/

char module_name[40] = MODULE_NAME;
char mqtt_prefix[40] = MQTT_PREFIX;
char mqtt_server[40] = MQTT_SERVER;
char mqtt_port[6] = MQTT_SERVERPORT;
int mqtt_publish_interval = MQTT_PUBLISH_INTERVAL;
unsigned long lightOnTimeout = 60000; //60 seconds
unsigned long switchOffAlertTime = 5000; //5 seconds

unsigned long lastread = 0, lastpublish = 0; // time of last read

//Create the webserver
ESP8266WebServer server(80);

// Create a mqtt MQTTClient class
MQTTClient mqtt;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
// or... use WiFiClientSecure for SSL
WiFiClient wifi;



/**********************/
/*     Global flags   */
/**********************/

bool relayOn = false;

//flag for saving data
bool shouldSaveConfig = false;

// Local
#include "f2s.h"
#include "Config.h"
#include "OTA.h"
#include "MQTT.h"
#include "WebServer.h"


/*************************** Sketch Code ************************************/

void setup() {

  // Initialize serial port
  Serial.begin(SERIAL_SPEED);
  Serial.println("SmartHome");
  Serial.println("--------------------");
  Serial.println("");

  // Read configuration file config.json from SPIFFS
  Serial.println("Mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("Mounted file system");
    LoadConfig();
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  char buffer[7];
  sprintf(buffer, "%f", humidityThreshold);
  WiFiManagerParameter custom_humidity_threshold("humidityThreshold", "Humidity Threshold", buffer, 5);
  WiFiManagerParameter custom_module_name("name", "module name", module_name, 40);
  WiFiManagerParameter custom_mqtt_prefix("prefix", "mqtt prefix", mqtt_prefix, 40);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);

  //add all your parameters here
  wifiManager.addParameter(&custom_module_name);
  wifiManager.addParameter(&custom_mqtt_prefix);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_humidity_threshold);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(WLAN_SSID,WLAN_PASS)) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("Connected!");

  //read updated parameters
  strcpy(module_name, custom_module_name.getValue());
  strcpy(mqtt_prefix, custom_mqtt_prefix.getValue());  
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port,   custom_mqtt_port.getValue());
  humidityThreshold = atof (custom_humidity_threshold.getValue());
  //strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    SaveConfig();
  }

    // Set hostname
  Serial.print("Hello, my name is "); 
  Serial.println(module_name);
  WiFi.hostname(module_name);
  
  
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());

  
  // Connect to MQTT server
  mqtt_connect(mqtt_server,mqtt_port,module_name);

  //CONNECT pin to buttons, and buttons to GND
  pinMode(INPUTPIN, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
      
  // Start with opened relay
    pinMode(RELAYPIN, OUTPUT);
    switchOff();

  // OTA
  ota_init(module_name);

  // Init and start webserver
  setup_webServer();
  publishIPAddress();
}

void loop() {
      // OTA
      ArduinoOTA.handle();
      server.handleClient();

      if (WiFi.status() == WL_CONNECTED) {
      // MQTT
            if (!mqtt.connected()) {
                  mqtt_connect(mqtt_server,mqtt_port,module_name);
            }
            else
            {
                  mqtt.loop();
            }
      }
      else
      {
            Serial.println("Not connected to wifi");
            //delay(3000);
            //ESP.restart();
      }
      
      
      if(digitalRead(INPUTPIN))
      {
         switchOn();
      }
  
      if (relayOn && millis() - startedOn >= lightOnTimeout) {
         switchOff();
      }

}




void switchOn(){
      digitalWrite(RELAYPIN, LOW);
      digitalWrite(LEDPIN, HIGH);
      if(!relayOn){
            publishRelayState(true);
            relayOn = true;
      }
      startedOn = millis();
}

void switchOff(){
      digitalWrite(RELAYPIN, HIGH);
      digitalWrite(LEDPIN, LOW);
      publishRelayState(false);
      relayOn = false;
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}
  
