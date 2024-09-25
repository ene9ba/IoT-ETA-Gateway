/*
Version 1.0 v. 20.11.2023

Dieses Programm ist für den ESP NodeMCU
Aufabe ist es, die Informationen, die vom ETA Heizkessel kommen via mqtt zu transferieren
 

Version 1.1 v. 19.01.2023
Negative Temperaturwerte führten zu Überlauf, wird jetzt abgefangen
in der Werteberechnung

Version 1.20 v. 25.09.2024 optimiertes reconnect und mqtt mit user und passwort

*/


#include <Arduino.h>
#include <SoftwareSerial.h>


#define __mqtt_DEBUG
#define __serial_DEBUG

//Portzuordnung
#define BUILDIN_LED         2     // LED auf Board
#define RX_PIN              4     // serial receive pin
#define TX_PIN              5     // serial transceive pin

// config softwareserial
#define BAUD_RATE           19200
#define RX_BUFF_SIZE        200
#define ANZ_GET_VALUES      11    // Anzahl der Werte, die vom ETA geholt werden sollen
#define CYCLETIME           60   // ETA sendet alle n Sekunden ein Protokoll



String          Version                       = "V1.20: ";
String          AppName                       = "ETA-Transceiver";


String          HOSTNAME                      = "IoT-ETA-Transceive.castle";
const  char*    HOTSPOTNAME                   = "ETA-API";

//MQTT-Server Einstellungen

const char*     MQTT_SERVER                   = "docker-home";
#define MQTT_USER "mymqtt"
#define MQTT_PW   "yfAlORp1C3k70fy6XSkY"


// mqtt publish values
const char*   mqtt_pub_Version                  = "/openHAB/ETA/Version";
const char*   mqtt_pub_Debug                    = "/openHAB/ETA/Debug";
const char*   mqtt_pub_RSSI                     = "/openHAB/ETA/RSSI";
const char*   mqtt_pub_raw_answer               = "/openHAB/ETA/raw_answer";
const char*   mqtt_pub_checksum                 = "/openHAB/ETA/chksm";
const char*   mqtt_pub_info                     = "/openHAB/ETA/info";



const char*   mqtt_pub_boiler                   = "/openHAB/ETA/Boilertemp";
const char*   mqtt_pub_air                      = "/openHAB/ETA/Ausentemperatur";
const char*   mqtt_pub_puffer_oben              = "/openHAB/ETA/buff_high";
const char*   mqtt_pub_puffer_mitte             = "/openHAB/ETA/buff_mid";
const char*   mqtt_pub_puffer_unten             = "/openHAB/ETA/buff_low";
const char*   mqtt_pub_soc                      = "/openHAB/ETA/soc";
const char*   mqtt_pub_exhaust                  = "/openHAB/ETA/exhaust";
const char*   mqtt_pub_oven                     = "/openHAB/ETA/oven";
const char*   mqtt_pub_oven_reflow              = "/openHAB/ETA/ovenReflow";
const char*   mqtt_pub_mk1                      = "/openHAB/ETA/MK1";
const char*   mqtt_pub_mk2                      = "/openHAB/ETA/MK2";

// mqtt subscribe values 
const char* mqtt_sub_commandFromOPENhab       = "/openHAB/ETA/Command";


#include <Helper.h>




//Allgemeine Variablen 
long timediff=0;
long times=0;

long    lastMsg = 0;
long    lastMsg1 = 0;
long    lastMsg2 = 0;
char    msg[150];
char    msg1[150];
// Kommandos
int openHAB_Command     = -1;
int heatContrl_Command  = 0;

char  rcv_buffer[RX_BUFF_SIZE];
unsigned int  writeptr = 0;
bool stx_set = false; // Serielles Protkoll vom ETA Start erkannt





EspSoftwareSerial::UART swSer1;

char startsequenz[3]          = {'{','M','C'};
char endsequenz               = '}';


// Steuerwerte, ein triple pro record  0x08 ist ETA Scheitkessel, die nächsten zwei stehen für den gewünschten Record nach Tabelle ETA doc
char monitorinfo[ANZ_GET_VALUES][3]             =   {
                                     {0x08,00,0x0c},  {0x08,00,0x0b}, {0x08,00,0x0a}
                                    ,{0x08,00,0x0d},  {0x08,00,70},   {0x08,00,75}
                                    ,{0x08,00,15},    {0x08,00,8},    {0x08,00,9}
                                    ,{0x08,00,68},    {0x08,00,69}
                                 };






/******************************************************************************************************************
* Sendet Degug information auf den MQTT-Kanal
* void mqttDebugInfo(String load ) 
*******************************************************************************************************************/

void mqttDebugInfo(String load ) 
      {

        load.toCharArray(msg1,150);
        Serial.println(load);
        client.publish(mqtt_pub_Debug, msg1);
        
      }



/******************************************************************************************************************
  * void mqtt_subscribe()
  * Trägt sich für Botschaften ein
*******************************************************************************************************************/
void mqtt_subscribe()
      {
              // ... and subscribe
              client.subscribe(mqtt_sub_commandFromOPENhab);
              
                            
      }


/******************************************************************************************************************
  *  void callback(char* topic, byte* payload, unsigned int length) void set_WarmColor()
  *  wird über mqtt subscribe getriggert, sobald von OPENHAB rom Event gemeldet wird
  *  
*******************************************************************************************************************/
void callback(char* topic, byte* payload, unsigned int length) 
        {
            // erst mal alles in Strings verwandeln für die Ausgabe und den Vergleich
            String str_topic=String(topic);
            
            char char_payload[20];
            unsigned int i=0;
            for (i = 0; i < length; i++) { char_payload[i]=(char)payload[i];}
            char_payload[i]=0;
            String str_payload=String(char_payload);
                
            
            String str_mqtt_openHab_command=String(mqtt_sub_commandFromOPENhab);

            String out="Message arrived, Topic : [" + str_topic + "] Payload : [" + str_payload + "]";
              
            #ifdef __mqtt_DEBUG
                
                mqttDebugInfo(out);
                Serial.print(out);
            #else
                Serial.print(out);
           #endif
            

            //
            //---- Kommando abfragen
            //
            
            
            
       }


/******************************************************************************************************************
  *  int  read_char_from_ETA()
  *  liest characters von der seriellen Schnittstelle, wartet auf das Startzeichen { , nimmt dann alle Zeichen 
  *  in einer Zeichenkette auf, bis das Endezeichen } erkannt wird. Pufferüberlauf wird abgefangen
*******************************************************************************************************************/

int  read_char_from_ETA() {
      
       char val; 
       int retval = 0;

       if(swSer1.available() == 0) return(retval);
       if(swSer1.available() > 0) {

          // abfangen überlauf Puffer
          if (writeptr >= RX_BUFF_SIZE) {
              writeptr = 0;
              stx_set = false;  
          }

		      val = swSer1.read();
          
          // Suche Startzeichen für das Protokoll
          if (val == '{') {
             // Prtokollstart erkannt, denn Flag setzen, folgende Daten in Puffer speichern, bis Ende kommt oder ein neues Anfang erkannt wird
            writeptr = 0;
            rcv_buffer[writeptr] = val; 
            stx_set = true;
            writeptr++;
          }
          else {
            if(stx_set) {
                rcv_buffer[writeptr] = val;
                writeptr++;  
                if(val == '}') {                 // end of text
                  rcv_buffer[writeptr] = val;
                  writeptr++;
                  rcv_buffer[writeptr] = 0x00;     // end of string tag
                  retval = writeptr;
                  writeptr = 0;
                  stx_set = false;
                } 
             }
          }
	      }
        return(retval);
    }   



/******************************************************************************************************************
  *  void generate_protocl2sendAll() 
  *  Erzeugt einen String, der als Inhalt die Startcode, die gewünschten Werte vom Kessel und den Endecode enthält
  *  Checksumme wird nur über die Nutzdaten im String gebildet, siehe Dokumentation ETA
  *  
*******************************************************************************************************************/

// alle zur Verfügung gestellen Daten holen
void generate_protocl2sendAll() {
        char buffer[200]; 
        int n , pruef=0 , ptr = 0;
        unsigned char ch;

        // Kommandostring zusammen bauen

        // Startsequenz {MC
        memcpy(buffer,startsequenz,sizeof(startsequenz));  

        // Datenstruktur asfüllen, jeweils 3 byte pro gewünschten Datenkanal
        ptr += 6; // Pointer auf erste Stelle im Kommandostring, wo Datenkanäle eingetragen werden
        for ( n = 0 ; n < ANZ_GET_VALUES; n++) {
            memcpy(buffer+ptr,monitorinfo[n],3);
            ptr += 3;
        }

        // Abfragezeitintervall eintragen
        buffer[5] = CYCLETIME;           // wiederholungszeit in sec

        //
        for (n=0; n < ptr; n++) {
          if (n >= 5) {
            pruef += buffer[n];
          }
        }
        buffer[3] = ptr-5;
        buffer[4] = pruef%256;
        buffer[ptr]= endsequenz;
        ptr++;
        for (int n=0; n < ptr; n++) {
          ch=buffer[n];
          swSer1.write(ch);
          if(n <= 2 || n == ptr-1) Serial.print(buffer[n]);
          else Serial.print(buffer[n],HEX);
        }
}

/******************************************************************************************************************
  *  void convert_readable()
  *  Die Protokollsttrukut besteht aus characters und Datenbytes. um den von ETA gelesenen String 
  *  lesbar an mqtt zu senden werden hier die datenwerte im String in ASCI-zeichen gewandelt
  *  
  *  
*******************************************************************************************************************/

void convert_readable() {
    
        
        char human_readable[400];
        int read_ptr = 3;
        int write_ptr = 3;
        int nr_data = rcv_buffer[3];  // Anzahl der Nutzdatenbytes
        

        strncpy(&human_readable[0],&rcv_buffer[0],3);
        for (int n=read_ptr; n <= nr_data+5; n++) write_ptr+= sprintf(&human_readable[write_ptr], "-%d",rcv_buffer[n]); //länge Nutzdaten

        strncpy(&human_readable[write_ptr],&rcv_buffer[nr_data+5],1);
        write_ptr++;
        human_readable[write_ptr]= '\0';

        // publish version raw protokol from ETA
        client.publish(mqtt_pub_raw_answer, human_readable);
      
      }

/******************************************************************************************************************
  *  bool checksum_OK()
  *  Berechnet die Checksumme der Nutdatenbytes und vergleicht die mit denen aus dem Protokoll 
  *  auf Richtigkeit
  *  
*******************************************************************************************************************/


      bool checksum_OK() {

        int chcksm = 0;

        // get nr. of messagebytes
        int nr_messagebytes = rcv_buffer[3];
        // get checksum
        int rcvd_cksm = rcv_buffer[4];
         
        // calculate checksum 
        static int startindex = 5;
        for (int n=startindex; n < startindex + nr_messagebytes; n++) chcksm+= rcv_buffer[n];
        chcksm = chcksm % 256;

        // Testausgabe
        //char str_cksm[100];
        //sprintf(str_cksm,"bytes: %d received: %d  calculated: %d",nr_messagebytes,rcvd_cksm,chcksm);
        //client.publish(mqtt_pub_checksum,str_cksm);

        if (rcvd_cksm == chcksm) return(true);
        else return(false);
         
      }

/******************************************************************************************************************
  *  void calc_temperatures()
  *  Rechnet die im Anwortstring enthaltenen Messwwerte in floatwerte um und ordnet diese
  *  dem richtigen Kanal zu
  *  
*******************************************************************************************************************/


      void calc_temperatures() {

        // get nr of datasets
        int nr_sets = rcv_buffer[3]; 
        int startindex = 7;
        float value = 0.0;
        char mqttbuffer[100];
        String tmpstr = "";

        for (int n=startindex; n < startindex + nr_sets; n+=5) {
          int measureindex = rcv_buffer[n];
          // Testausgabe
          //sprintf(mqttbuffer,"n: %d anzprot: %d wert:  %d",n,nr_sets,measureindex);
          //client.publish(mqtt_pub_info,mqttbuffer);
          value = ((rcv_buffer[n+1] * 256.0) +  rcv_buffer[n+2]) / 10.0;
          if (value > 1000.0 ) value = value - 6553.5;
          sprintf(mqttbuffer,"%.1f",value);

          switch (measureindex) {
            // Boilertemperatur
            case 13 :   client.publish(mqtt_pub_boiler,mqttbuffer);
                        break;
            // Ausentemperatur
            case 70 :   client.publish(mqtt_pub_air,mqttbuffer);
                        break;            
            // Puffer oben
            case 12 :   client.publish(mqtt_pub_puffer_oben,mqttbuffer);
                        break;            
            // Puffer mitte
            case 11 :   client.publish(mqtt_pub_puffer_mitte,mqttbuffer);
                        break;            
            // Puffer unten
            case 10 :   client.publish(mqtt_pub_puffer_unten,mqttbuffer);
                        break;            
            // State of Charge
            case 75 :   value = value * 10;
                        sprintf(mqttbuffer,"%.1f",value);
                        client.publish(mqtt_pub_soc,mqttbuffer);
                        break;            
            // Abgastemperatur
            case 15 :   client.publish(mqtt_pub_exhaust,mqttbuffer);
                        break;            
            // Kessel
            case  8 :   client.publish(mqtt_pub_oven,mqttbuffer);
                        break;            
            // Kessel Rücklauf
            case  9 :   client.publish(mqtt_pub_oven_reflow,mqttbuffer);
                        break;            
            // Mischer MK1 Vorlauf
            case 68 :   client.publish(mqtt_pub_mk1,mqttbuffer);
                        break;            
            // Mischer MK2 Vorlauf
            case 69 :   client.publish(mqtt_pub_mk2,mqttbuffer);
                        break;            
           
            default :   tmpstr = "Werte können nicht gelesen werden, falscher Kanalindex";
                        //client.publish(mqtt_pub_info,tmpstr.c_str());
                        break;
          }
        }
      }

/******************************************************************************************************************
  * void setup() 
  * Program Start init
*******************************************************************************************************************/


void setup() {

  pinMode(LED_BUILTIN,        OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  
  lastMsg = millis(); 


  // Serielle Schnittstelle init      
  Serial.begin(19200);
        
  ping(100,3);
        
  //delay(500);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("\n\n\nBooting");
  
  init_WiFiManger();
  init_OTA(); 

  // Verbindung mit mqtt-Server aufbauen
  setup_mqtt();
  client.setCallback(callback);
  mqtt_subscribe();

  // publish version
  String out=Version+AppName;
  Serial.println(out);
  out.toCharArray(msg,75);
  client.publish(mqtt_pub_Version, msg);

  swSer1.begin(BAUD_RATE, EspSoftwareSerial::SWSERIAL_8N1, RX_PIN, TX_PIN, false, 256);
	
  Serial.println("\nSerial connection online");

  generate_protocl2sendAll();

  

  }


   void publishRSSI()

{
    //Zeit seit letztem Durchlauf berechen und die Sekunden in value hochz?hlen  
    long now = millis();

    if (now - lastMsg2 < 5000) return;

    int rssi = WiFi.RSSI();

    ping(80,1);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" db");

    // publish version
    itoa(rssi,msg,10);
    
    client.publish(mqtt_pub_RSSI,msg);

    lastMsg2 = millis();

}   

void sendcommand() {
      long now = millis();
      if (now - lastMsg1 < 3600000) return;
      generate_protocl2sendAll();
      lastMsg1 = millis();
      }

/******************************************************************************************************************
  * void loop() 
  * Program Start init
*******************************************************************************************************************/

void loop() {


  // alle Stunde einmal den Auftrag zum senden von Daten aufrufen;
  sendcommand();

  publishRSSI();
  
  // so lange die bytes lesen, bis } erkannt wird, dann ist ein Telegram angekommen.

  int protlen = read_char_from_ETA();
  String tmp_str = "";
        
  if (protlen > 0) {
    //convert_readable();
    if (checksum_OK()) {
      tmp_str = "OK";
      calc_temperatures();
    }
    else tmp_str = "checksum failed";
   

  }
	
          
  //WiFi Verbindung prüfen
  if (!client.connected()) 
   {
      setup_mqtt();
      //reconnect();
      //client.setCallback(callback);
      //mqtt_subscribe();
   }

  client.loop();

  ArduinoOTA.handle();
}






                                














      
