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

#define MOISTURE_SENSOR_A A0
#define MOISTURE_SENSOR_D D5 // D3 and D4 is already occupied
#define ONE_WIRE_BUS D1  //Bestimmt Port an dem der Sensor angeschlossen ist
#define WATERPUMPVOLTAGE D2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

char temperaturStr[6];
char voltageStr[6];

float merketemperatur=0;

const char* ssid = "foo"; //Ihr Wlan,Netz SSID eintragen
const char* pass = "10"; //Ihr Wlan,Netz Passwort eintragen
IPAddress ip(192,168,2,75); //Feste IP des neuen Servers, frei wählbar
IPAddress gateway(192,168,2,1); //Gatway (IP Router eintragen)
IPAddress subnet(255,255,255,0); //Subnet Maske eintragen

ESP8266WebServer server(80);

void handleRoot() {
 //Temperatur auch bei Url-Aufruf zurückgeben
 String message="*** Ueberwachungs Server - Beispiel von www.mikrocontroller-elektronik.de ***\n";
 String tempstr= String(merketemperatur, 2); 
 message += "Temperatur Innen : " + tempstr +"\n";
 //tempstr= String(merkeaussentemperatur, 2); 
 //message += "Temperatur Aussen: " + tempstr +"\n";
 server.send(200, "text/plain", message);
}


void handleTemperatur() {
 //printUrlArg(); //fuer Debugz Zwecke

 String stemp =server.arg("wert");
 float temperatur = stemp.toFloat();
 /*if (merkeaussentemperatur!=temperatur) {
 zeigeAussenTemperatur(temperatur);
 merkeaussentemperatur=temperatur;
 
 }
*/
  merketemperatur=temperatur;
 //Temperatur auch bei Url-Aufruf zurückgeben
 String message="*** Ueberwachungs Server - Beispiel von www.mikrocontroller-elektronik.de ***\n";
 String tempstr= String(merketemperatur, 2); 
 message += "Temperatur Innen : " + tempstr +"\n";
 //tempstr= String(merkeaussentemperatur, 2); 
// message += "Temperatur Aussen: " + tempstr +"\n";
 server.send(200, "text/plain", message);
}


void printUrlArg() {
 //Alle Parameter seriell ausgeben
 String message="";
 for (uint8_t i=0; i<server.args(); i++){
 message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
 }
 server.send(200, "text/plain", message);
 Serial.println(message);
}


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
 
 server.on("/",handleRoot) ;
 server.on("/sensor/temperatur/", handleTemperatur);
 server.begin();
 
 Serial.println("HTTP Server wurde gestartet!");
}

float getTemperatur() {
 float temp;
 do {
 DS18B20.requestTemperatures(); 
 temp = DS18B20.getTempCByIndex(0);
 delay(100);
 } while (temp == 85.0 || temp == (-127.0));
 return temp;
}

void loop() {

  server.handleClient();
 
 delay(500); 

 digitalWrite(LED, LOW); //Led port ausschalten
 delay(1000); //1 Sek Pause
 digitalWrite(LED, HIGH); //Led port einschlaten
 delay(1000); 

 float temperatur = getTemperatur();
 merketemperatur = temperatur;
 dtostrf(temperatur, 2, 2, temperaturStr);
 Serial.print("Temperatur: "); 
 Serial.println(temperaturStr); 
 delay(1000);
 digitalWrite(WATERPUMPVOLTAGE, HIGH);
 delay(5000);
 digitalWrite(WATERPUMPVOLTAGE, LOW);
 delay(2000);
 float voltagePlant = analogRead(MOISTURE_SENSOR_A);
 int voltageReservoir = digitalRead(MOISTURE_SENSOR_D);
 delay(1000);
 Serial.print("Voltage (moisture of earth): ");
 Serial.println(String(voltagePlant));
 Serial.print("Voltage (water level): ");
 Serial.println(String(voltageReservoir));
 
}
