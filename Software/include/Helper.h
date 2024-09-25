#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// needed for WiFiManger
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

// needed for MQTT
#include <PubSubClient.h>



WiFiClient espClient;

PubSubClient client(espClient);



/******************************************************************************************************************
* Sendet Degug information auf den MQTT-Kanal
* void mqttDebugInfo(String load ) 
*******************************************************************************************************************/

void DebugInfo(String load ) {
     #ifdef __mqtt_DEBUG
        #define MLEN 100   
        char msg[MLEN];

        load.toCharArray(msg,MLEN);
        client.publish(mqtt_pub_Debug, msg);
     #endif
     #ifdef __serial_DEBUG
        Serial.println(load);
     #endif
        
     }


void init_WiFiManger()
{
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
    
  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(HOTSPOTNAME);
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

    
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //OTA Config
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");


}


void init_OTA()
{
    // OTA Setup   
    ArduinoOTA.onStart([]()
        {
            Serial.println("Start");
        });
    ArduinoOTA.onEnd([]()
        {
            Serial.println("\nEnd Device =");
            Serial.println(HOSTNAME);
        });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
        {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
    ArduinoOTA.onError([](ota_error_t error)
        {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });
    ArduinoOTA.begin();

    ArduinoOTA.setHostname(HOSTNAME.c_str());
    Serial.println("OTA Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());



}


/******************************************************************************************************************
  * Steuert die buildin LED (blau) für eine definierte Zeit an
  * void int(int ontime ) 
*******************************************************************************************************************/

void ping(int ontime, int repeat)
      {

                      if (ontime < 50) ontime=1000;
                      if (repeat < 1) repeat=1;
                      for (int i=0; i<repeat; i++)
                      {
                        digitalWrite(BUILDIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
                        delay(ontime);
                        digitalWrite(BUILDIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
                        delay(ontime);
                      }
         }



/******************************************************************************************************************
  * void setup_mqtt()
  * mqtt Setup & connect
*******************************************************************************************************************/
void setup_mqtt()
      {

          //generate unique Clientname for mqtt-Server

          IPAddress ip = WiFi.localIP();
          String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
          String MQTT_HostName=String(HOSTNAME)+"_"+ipStr;
          char MQTT_HostNameChar[MQTT_HostName.length()];
          MQTT_HostName.toCharArray(MQTT_HostNameChar,sizeof(MQTT_HostNameChar));

          int loop=0;
          delay(10);
          // We start by connecting to a WiFi network
          Serial.print("This Client = ");
          Serial.println(MQTT_HostName);
          Serial.println("");
          Serial.print(".. MQTT .. ");
          Serial.print("Connecting to ");
          Serial.println(MQTT_SERVER);
        

  
          // Verbinden mit mqtt-Server und einrichten Callback event        
          client.setServer(MQTT_SERVER, 1883);
          
  
          while (!client.connected()) 
          {
            
            client.connect((char*)MQTT_HostNameChar, MQTT_USER, MQTT_PW);

          
            Serial.print(".");
            loop++;
            if (loop > 2) break;
          }
        if (loop > 2) 
        {

            Serial.println("");
            Serial.println("NQTT-Server not found ...");
            delay(2000);
            // ESP.restart();
        }
        else  
        {
          
          String str = " MQTT found address: " +  MQTT_HostName;
          Serial.println("");
          Serial.print(str);
                    
          DebugInfo(str);

        }
    }






