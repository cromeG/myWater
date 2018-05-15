/*
 NodeMCU-DallasDS18B20
 Led an dem Modul ESP8266 bzw. Board NodeMCU blinken lassen
 und Temperatursensor an Pin D1 auslesen
 
 Notwendig ist die angepasste Dallas-Lib: 
   Download hier: https://github.com/milesburton/Arduino-Temperature-Control-Library
   Eine eventuell vorhandene DallasTemperature-Lib sollte gelöscht werden, damit oben 
   genannte von der IDE verwendet wird
 
 Bezugsquelle Temperatursensor: Reichelt / Conrad / Amazon - http://amzn.to/2i3WlRX 
 Bezugsquelle NodeMCU Board: http://amzn.to/2iRkZGi

 Programm erprobt ab Arduino IDE 1.6.13
 Weitere Beispiele unter http://www.mikrocontroller-elektronik.de/
*/

#define LED D0 //Interne Led auf dem NodeMCU Board LED_BUILTIN

#include <DallasTemperature.h> //Siehe Hinweis oben, verwendet wird 
                            //https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <Base64.h>  // required for temperature sensor
#include <OneWire.h> // required for temperature sensor

#include <ESP8266WiFi.h>  // WLAN
#include <ESP8266WebServer.h> // WLAN

#include "Gsender.h"

#define MOISTURE_SENSOR_A A0 // Analog moisture
#define MOISTURE_SENSOR_D D5 // D3 and D4 is already occupied water level
#define MOISTURE_SENSOR_SWITCH D6 // to switch on the moisture sensors
#define ONE_WIRE_BUS D1  //Bestimmt Port an dem der Sensor angeschlossen ist
#define WATERPUMPVOLTAGE D2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

char temperaturStr[6];
char voltageStr[6];

float merketemperatur=0;
float moisturMinVal = 1000;
float moisturMaxVal = 700;
float onlyMonitor = 1;
int timeChunkCounter = 0;
 // Log der Pumphistorie
int pumpcycle = 0;
int pumptime = 2700;
int intervallength = 30;
int  watLevReSeVal = 0;
int moistureSeVal = 0;
int old_watLevReSeVal = 0;
int notification_value = 0;
const char* ssid = "StefanWLAN"; //Ihr Wlan,Netz SSID eintragen "StefanWLAN"
const char* pass = "Senfeule1992"; //Ihr Wlan,Netz Passwort eintragen "Senfeule1992"
IPAddress ip(192,168,0,75); //Feste IP des neuen Servers, frei wählbar 192,168,0,75
IPAddress gateway(192,168,0,1); //Gatway (IP Router eintragen)
IPAddress subnet(255,255,255,0); //Subnet Maske eintragen

ESP8266WebServer server(80);

void setup() {
 pinMode(LED, OUTPUT); // Port aus Ausgang schalten
 Serial.begin(115200);
 DS18B20.begin();
 pinMode(WATERPUMPVOLTAGE, OUTPUT);
 pinMode(MOISTURE_SENSOR_SWITCH, OUTPUT);
 pinMode(MOISTURE_SENSOR_D, INPUT);

  //  Force the ESP into client-only mode
  WiFi.mode(WIFI_STA); 


 WiFi.begin(ssid, pass);
 WiFi.config(ip, gateway, subnet);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 //  Enable light sleep
 //wifi_set_sleep_type(MODEM_SLEEP_T)
 Serial.print(".");
 } 
 Serial.println("");
 Serial.print("Verbunden mit ");
 Serial.println(ssid);
 Serial.print("IP address: ");
 Serial.println(WiFi.localIP());

 //attribute weblinks to the respective methods
 server.on("/",handleRoot) ;
 server.on("/setMoistureInterval/", handleMoisture);
 server.begin();
 
 Serial.println("HTTP Server wurde gestartet!");

//   delay(2000);

//---------- Email Setup -------------
 Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
 Serial.println("instance created");
 String subject = "MyWater System Active";
 if(gsender->Subject(subject)->Send("stefanh.rost@gmail.com", "Setup test")) {
        Serial.println("Message send.");
    } else {
        Serial.print("Error sending message: ");
        Serial.println(gsender->getError());
    }
//--------- End Email Setup -------------------
}

// What is done when the root website, i.e. the IP is called
void handleRoot() {
  // initialize variable containing the text written on the webpage
  float tempSensorVal = getTemperatur();
  //watLevReSeVal = getReservoir();
  //moistureSeVal = getMoisture();
  String webpageContent = " ";
  String tempSensorCStr = String(tempSensorVal, 2);
  String WatLevReserStr = String(watLevReSeVal);
  String moistureSenStr = String(moistureSeVal);
  String moistureMinStr = String(moisturMinVal);
  String moistureMaxStr = String(moisturMaxVal);
  String PumpCounterStr = String(pumpcycle);
  String PumpTimeStr = String(pumptime);
  String IntervalLengthStr = String(intervallength);
  
  // Fill webpage with information
  webpageContent += "MYWATER Artificial Watering Machine Status Report \n\n";
  webpageContent += "Temperatur       : " + tempSensorCStr + "°C \n";
  webpageContent += "Wasserstand      : " + WatLevReserStr + "?? \n";
  webpageContent += "Feuchtigkeit     : " + moistureSenStr + "?? \n\n"; 
  webpageContent += "Mindestfeuchte   : " + moistureMinStr + "?? \n";
  webpageContent += "Maximale Feuchte : " + moistureMaxStr + "?? \n";
  webpageContent += "Anzahl Pumpzyklen : " + PumpCounterStr + " \n";
  webpageContent += "Pumpdauer : " + PumpTimeStr + "ms \n"; 
    webpageContent += "Intervall Länge : " + IntervalLengthStr + "[3s] \n"; 

 // moistureSeVal = 0;
  if (watLevReSeVal == 0)
  {
    webpageContent += "WASSER-RESERVOIR LEER. BITTE NACHFÜLLEN \n\n";
  }
    if (watLevReSeVal == 1)
  {
    webpageContent += "WASSER-RESERVOIR GENÜGEND GEFÜLLT \n\n";
  }
    if (onlyMonitor == 1)
  {
    webpageContent += "NUR MONITORING. ES WIRD NICHT GEGOSSEN !";
  }
    if (onlyMonitor == 0)
  {
    webpageContent += "ES WIRD GEGOSSEN BEI BEDARF";
  }

  server.send(200, "text/plain", webpageContent);
}

//called if webpage setMoistureInterval is called
void handleMoisture() {
  String argContainer = "";
  argContainer = server.arg("minMoisture");
  moisturMinVal = argContainer.toFloat();
  argContainer = server.arg("maxMoisture");
  moisturMaxVal = argContainer.toFloat();
  argContainer = server.arg("onlyMonitor");
  onlyMonitor = argContainer.toFloat();
  argContainer = server.arg("pumptime");
  pumptime = argContainer.toFloat();
  argContainer = server.arg("intervallength");
  intervallength = argContainer.toFloat();

  String message = "Feuchtigkeitsintervall erfolgreich gesetzt!";
  server.send(200, "text/plain", message);
}

float getTemperatur() {
 float temp;
 // Read temperature until the sensor measures something within its functioning interval.
 do {
   DS18B20.requestTemperatures(); 
   temp = DS18B20.getTempCByIndex(0);
   delay(100);
 } while (temp == 85.0 || temp == (-127.0));
 
 return temp;
}

int getMoisture(){
  int moisture = 0;
  digitalWrite(MOISTURE_SENSOR_SWITCH, LOW);
  delay(50); //0,1 Sek Warten
  moisture=analogRead(MOISTURE_SENSOR_A);
  digitalWrite(MOISTURE_SENSOR_SWITCH, HIGH);
  return moisture;
}

int getReservoir(){
  int currentRead =1;
  digitalWrite(MOISTURE_SENSOR_SWITCH, LOW);
  delay(50); //0,1 Sek Warten
  currentRead=digitalRead(MOISTURE_SENSOR_D);
  digitalWrite(MOISTURE_SENSOR_SWITCH, HIGH);
   int currentResLev = 0;
//   if (currentRead == 1){
//     currentResLev = 0;
//     }
   if (currentRead == 0){
    currentResLev = 1;
     }
  return currentResLev;
}

void loop() {
 delay(100);
 timeChunkCounter++;
 // Flash up notification LED when server is ready to answer (main webpage is reloaded given a request)
 digitalWrite(LED, HIGH); //Led port ausschalten
 delay(1000); //1 Sek Pause
 digitalWrite(LED, LOW); //Led port einschlaten
 delay(1000); 
 server.handleClient();
 delay(1000);

 if (timeChunkCounter >= intervallength) {
   old_watLevReSeVal = watLevReSeVal ;
   watLevReSeVal = getReservoir();
   moistureSeVal = getMoisture();
   notification_value = old_watLevReSeVal-watLevReSeVal ;
//  notification_value = 1;
   if (notification_value == 1) {
     String subject = "Der Wassertank ist leer!";
     String messageContent = "MYWATER Artificial Watering Machine Status Report \n\n" ;
      float tempSensorVal = getTemperatur();
      String tempSensorCStr = String(tempSensorVal, 2);
      String WatLevReserStr = String(watLevReSeVal);
      String moistureSenStr = String(moistureSeVal);
      String PumpCounterStr = String(pumpcycle);
      String PumpTimeStr = String(pumptime);
     messageContent += "Temperatur       : " + tempSensorCStr + "°C \n";
     messageContent += "Wasserstand      : " + WatLevReserStr + "?? \n";
     messageContent += "Feuchtigkeit     : " + moistureSenStr + "?? \n\n"; 
     messageContent += "Anzahl Pumpzyklen : " + PumpCounterStr + " \n";
     if (onlyMonitor == 1)
      {
        messageContent += "NUR MONITORING. ES WIRD NICHT GEGOSSEN ! \n \n";
      }
     if (onlyMonitor == 0)
      {
        messageContent += "ES WIRD GEGOSSEN BEI BEDARF \n \n";
      }
     messageContent += "Bitte etwa 2,5 Liter Wasser Nachfüllen \n";
     Gsender *gsender = Gsender::Instance(); 
     gsender->Subject(subject)->Send("stefanh.rost@gmail.com", messageContent) ;
   }
   //float currentMoisture = getMoisture();
   //float currentResLev = getReservoir();
   timeChunkCounter = 0;
   if ( (moistureSeVal >= moisturMinVal ) & (moistureSeVal !=1024) & (watLevReSeVal == 1) & (onlyMonitor == 0)){
     digitalWrite(WATERPUMPVOLTAGE, HIGH);
     delay(pumptime);
     digitalWrite(WATERPUMPVOLTAGE, LOW);
     pumpcycle++;
   }    
 }
 //Serial.println(String(getMoisture()));
}
