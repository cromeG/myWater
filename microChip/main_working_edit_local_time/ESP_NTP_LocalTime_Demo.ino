/////////////////////////////////////////////////////////
// Demo for Local Time with NTP
// for ESP8266 (with Arduino IDE Integration)
// Stefan Thesen 08/2017
//
// Free for anybody - no warranties
//
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////

// network name of the esp8266
#define HOSTNAME "localtime"
// Wifi credentials
#define MYSSID "YOU WIFI NAME HERE"
#define MYPSK "YOUR PASSWORD HERE"

// Required libs
// TimeLib Library by Michael Margolis 
//      --> install via menu
// Timezone Library 
//      --> https://github.com/JChristensen/Timezone


#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <Timezone.h>
#include "time_ntp.h"

// counters
unsigned long ulReqcount=0;
unsigned long ulReconncount=0;

// ntp flag
bool bNTPStarted=false;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

// manage wifi state
bool gbIsConnecting=false;

  

//////////////////////////////////////////
// for time conversion
//Central European Time (Frankfurt, Paris)
//////////////////////////////////////////
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);


///////////////////////////////////////////
// get UTC time referenced to 1970 by NTP
///////////////////////////////////////////
time_t getNTP_UTCTime1970() 
{ 
  bNTPStarted=false;  // invalidate; time-lib functions crash, if not initalized poperly
  unsigned long t = getNTPTimestamp();
  if (t==0) return(0);

  // scale to 1970 
  // may look like back & forth with ntp code; wrote it to make needed conversions more clear
  return(t+946684800UL);
}


//////////////////////////
// create HTTP 1.1 header
//////////////////////////
String MakeHTTPHeader(unsigned long ulLength)
{
  String sHeader;
  
  sHeader  = F("HTTP/1.1 200 OK\r\nContent-Length: ");
  sHeader += ulLength;
  sHeader += F("\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  return(sHeader);
}

////////////////////
// make html footer
////////////////////
String MakeHTTPFooter()
{
  String sResponse;
  
  sResponse  = F("<FONT SIZE=-2><BR>Aufrufz&auml;hler="); 
  sResponse += ulReqcount;
  sResponse += F(" - Verbindungsz&auml;hler="); 
  sResponse += ulReconncount;
  sResponse += F("<BR>Stefan Thesen 08/2017<BR></body></html>");
  
  return(sResponse);
}


////////////////
// setup routine
////////////////
void setup() 
{  
  Serial.begin(115200);
  Serial.println(F("\n\Local Time Demo V.1.0 - S.T. 08/2017"));
}


//////////////////
// Connect to WiFi
//////////////////
void WiFiStart()
{
  if (gbIsConnecting && WiFi.status()==WL_CONNECTED)
  {
    Serial.println("WiFi connected");
          
    // Start the server
    server.begin();
    Serial.print("Server started - IP: ");
    Serial.println(WiFi.localIP());
    gbIsConnecting=false;
  }
  else if (!gbIsConnecting)
  {
    ulReconncount++;
    
    // Connect to WiFi network
    Serial.print("Connecting to ");
    Serial.println(MYSSID);    
    WiFi.begin(MYSSID, MYPSK);
    gbIsConnecting=true;
  }
}


//////////////
//loop routine
//////////////
void loop() 
{  
  /////////////////////////////////////////////////////////////////////
  // check if WIFI not any more connected or if we are about to connect
  /////////////////////////////////////////////////////////////////////
  if ( (gbIsConnecting) || (WiFi.status() != WL_CONNECTED) )
  {
    WiFiStart();
    return;
  }

  ////////////////////////////////////////////
  // below here we got a valid WIFI connection
  ////////////////////////////////////////////

  // connect to NTP and get time each day
  // timelib now synced up to UTC
  if (timeStatus()==timeNotSet)
  {
    bNTPStarted=false;
    Serial.println("Setup sync with NTP service.");
    setSyncProvider(getNTP_UTCTime1970);
    setSyncInterval(86400); // NTP re-sync; i.e. 86400 sec would be once per day
  }
  else
  {
    bNTPStarted=true;   // only position, where we set this flag to true
  }
  yield();

  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) { return; }
  
  // Wait until the client sends some data
  Serial.println("new client");
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
    Serial.println("client connection time-out!");
    return; 
  }
  
  // Read the first line of the request
  String sRequest = client.readStringUntil('\r');
  client.flush();
  
  if(sRequest=="")
  {
    Serial.println("empty request! - stopping client");
    client.stop();
    return;
  }

  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?pin=MOTOR1STOP HTTP/1.1
  String sPath="",sParam="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
    
  ///////////////////////////
  // format the html response
  ///////////////////////////
  String sResponse,sHeader;
  
  ////////////////////////
  // format the html pages
  ////////////////////////
  if(sPath=="/")  // home
  {
    int ii;
    ulReqcount++;
    sResponse  = F("<html><head><title>Demo f&uuml;r lokale Zeit</title>");
    sResponse += F("<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:1;border-radius:1.5rem;background-color:#0080ff;color:#ffffff;line-height:2.5rem;font-size:1.5rem;width:100%;} .q{float: right;width: 64px;text-align: right;} }</style>");
    sResponse += F("</head><div style='text-align:left;display:inline-block;min-width:260px;'><body>");
    sResponse += F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">");
    sResponse += F("<h1>Demo f&uuml;r lokale Uhrzeit</h1>");

    sResponse += F("Anzeige der lokalen Uhrzeit unter Ber&uuml;cksichtigung der Sommer/Winterzeit aus einer NTP Server Abfrage.<br>");
    sResponse += F("Der NTP Server wird in diesem Beispiel jeden Tag abgefragt. Die UTC Uhrzeit wird in der timelib Bibliothek gesetzt.<br>");
    sResponse += F("Zum Zeitpunkt des Aufrufs der Website wird die Zeit mit Hilfe der timezone Bibliothek in die lokale Zeit umgerechnet.<br><br>");
    sResponse += F("<FONT SIZE=+1>");

    if (bNTPStarted)
    {
      // UTC
      time_t tT = now();
      sResponse += F("UTC (Coordinated Universal Time) ");
      sResponse += hour(tT); sResponse += F(":");
      sResponse += minute(tT)/10; sResponse += minute(tT)%10; sResponse += F(":");
      sResponse += second(tT)/10; sResponse += second(tT)%10; sResponse += F(" - ");
      sResponse += day(tT); sResponse += F("."); sResponse += month(tT); sResponse += F("."); sResponse += year(tT);

      // local time
      time_t tTlocal = CE.toLocal(tT);
      sResponse += F("<BR>Lokale Zeit (Mitteleuropa) ");
      sResponse += hour(tTlocal); sResponse += F(":");
      sResponse += minute(tTlocal)/10; sResponse += minute(tTlocal)%10; sResponse += F(":");
      sResponse += second(tTlocal)/10; sResponse += second(tTlocal)%10; sResponse += F(" - ");
      sResponse += day(tTlocal); sResponse += F("."); sResponse += month(tTlocal); sResponse += F("."); sResponse += year(tTlocal);
      sResponse += F("<BR>");
    }
    else
    {
      sResponse += F("Noch keine NTP Zeitabfrage erfolgt.");
    }

    sResponse += MakeHTTPFooter();
    sResponse += F("</body></html>");
    sHeader = MakeHTTPHeader(sResponse.length());
  }
  else
  ////////////////////////////
  // 404 for non-matching path
  ////////////////////////////
  {
    sResponse=F("<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>");
    
    sHeader  = F("HTTP/1.1 404 Not found\r\n");
    sHeader += F("Content-Length: ");
    sHeader += sResponse.length();
    sHeader += F("\r\n");
    sHeader += F("Content-Type: text/html\r\n");
    sHeader += F("Connection: close\r\n");
    sHeader += F("\r\n");
  }

  
  // Send the response to the client
  client.print(sHeader);
  client.print(sResponse);
  client.stop();

  Serial.println("Client disconnected");
}

