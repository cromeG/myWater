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

#define MOISTURE_SENSOR_A A0 // Analog moisture
#define MOISTURE_SENSOR_D D5 // D3 and D4 is already occupied water level
#define ONE_WIRE_BUS D1  //Bestimmt Port an dem der Sensor angeschlossen ist
#define WATERPUMPVOLTAGE D2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

char temperaturStr[6];
char voltageStr[6];

float merketemperatur=0;
float moisturMinVal = 1000;
float moisturMaxVal = 700;
int timeChunkCounter = 0;

int moistureSeVal = 0;
const char* ssid = "StefanWLAN"; //Ihr Wlan,Netz SSID eintragen
const char* pass = "Senfeule1992"; //Ihr Wlan,Netz Passwort eintragen
IPAddress ip(192,168,0,75); //Feste IP des neuen Servers, frei wählbar
IPAddress gateway(192,168,0,1); //Gatway (IP Router eintragen)
IPAddress subnet(255,255,255,0); //Subnet Maske eintragen

ESP8266WebServer server(80);

void setup() {
 pinMode(LED, OUTPUT); // Port aus Ausgang schalten
 Serial.begin(115200);
 DS18B20.begin();
 pinMode(WATERPUMPVOLTAGE, OUTPUT);
 pinMode(MOISTURE_SENSOR_D, INPUT);
 
 WiFi.begin(ssid, pass);
 WiFi.config(ip, gateway, subnet);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
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
}

// What is done when the root website, i.e. the IP is called
void handleRoot() {
  // initialize variable containing the text written on the webpage
  float tempSensorVal = getTemperatur();
  int   watLevReSeVal = 1;
  moistureSeVal = getMoisture();
  String webpageContent = " ";
  String tempSensorCStr = String(tempSensorVal, 2);
  String WatLevReserStr = String(watLevReSeVal);
  String moistureSenStr = String(moistureSeVal);
  String moistureMinStr = String(moisturMinVal);
  String moistureMaxStr = String(moisturMaxVal);
  
  // Fill webpage with information
  webpageContent += "MYWATER Artificial Watering Machine Status Report \n\n";
  webpageContent += "Temperatur       : " + tempSensorCStr + "°C \n";
  webpageContent += "Wasserstand      : " + WatLevReserStr + "?? \n";
  webpageContent += "Feuchtigkeit     : " + moistureSenStr + "?? \n\n"; 
  webpageContent += "Mindestfeuchte   : " + moistureMinStr + "?? \n ";
  webpageContent += "Maximale Feuchte : " + moistureMaxStr + "?? \n"; 

  moistureSeVal = 0;
  if (moistureSeVal == 0)
  {
    webpageContent += "WASSER-RESERVOIR LEER. BITTE NACHFÜLLEN";
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
  return analogRead(MOISTURE_SENSOR_A);
}

void loop() {
 delay(500);
 timeChunkCounter++;
 // Flash up notification LED when server is ready to answer (main webpage is reloaded given a request)
 digitalWrite(LED, LOW); //Led port ausschalten
 delay(1000); //1 Sek Pause
 digitalWrite(LED, HIGH); //Led port einschlaten
 delay(500); 
 server.handleClient();

// 2 seconds

 if ( (timeChunkCounter == 3) | (timeChunkCounter == 10) ) {
   float currentMoisture = getMoisture();
   if ( currentMoisture >= moisturMinVal ) {
     digitalWrite(WATERPUMPVOLTAGE, HIGH);
     delay(5000);
     digitalWrite(WATERPUMPVOLTAGE, LOW);
     if (timeChunkCounter == 3){
       timeChunkCounter = 0;
     }
   }    
 }
 Serial.println(String(getMoisture()));
}
