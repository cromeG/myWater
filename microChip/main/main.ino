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
#include <Base64.h>
#include <OneWire.h>

#define ONE_WIRE_BUS D1  //Bestimmt Port an dem der Sensor angeschlossen ist

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

char temperaturStr[6];

void setup() {
 pinMode(LED, OUTPUT); // Port aus Ausgang schalten
 Serial.begin(115200);
 DS18B20.begin();
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
 digitalWrite(LED, LOW); //Led port ausschalten
 delay(1000); //1 Sek Pause
 digitalWrite(LED, HIGH); //Led port einschlaten
 delay(1000); 

 float temperatur = getTemperatur();
 dtostrf(temperatur, 2, 2, temperaturStr);
 Serial.print("Temperatur: "); 
 Serial.println(temperaturStr); 

}
