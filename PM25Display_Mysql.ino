#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <PMS.h>
#include <MD_MAX72xx.h>

// Parameter LED DOt Matrix
#define  PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
#define MAX_DEVICES 4
#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D4  // or SS

// Text parameters
#define  CHAR_SPACING  1 // pixels between characters

// Config Sensor PM2.5
PMS pms(Serial);
PMS::DATA data;

MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  10
char message[BUF_SIZE] = {"Hello!"};
bool newMessageAvailable = true;

//Config WiFi
const char* ssid = "bank";              // Your wifi Name       
const char* password = "ExatIts2018";          // Your wifi Password

//const char *host = "172.20.3.178"; //Your pc or server (database) IP, example : 192.168.0.0 , if you are a windows os user, open cmd, then type ipconfig then look at IPv4 Address.

// Config IP Address
IPAddress ip(172, 20, 3, 181);
IPAddress gateway(172, 20 ,3 ,254);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  pms.passiveMode();
  mx.begin();
  Serial.begin(9600);
  WiFi.mode(WIFI_OFF);        //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA);        //This line hides the viewing of ESP as wifi hotspot

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);     //Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    delay(250);
  }
  
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.println("Connected to Network/SSID");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
}

void loop() {  
  // put your main code here, to run repeatedly:
  HTTPClient http;    //Declare object of class HTTPClient

  pms.wakeUp();
  delay(5000);

  String PM25ValueSend,PM10ValueSend,PM100ValueSend;
  
  // Reading PM2.5
  int pm25value = data.PM_AE_UG_2_5;
  PM25ValueSend = String(pm25value);    //String to interger conversion

  // Reading PM1.0
  int pm10value = data.PM_AE_UG_1_0;
  PM10ValueSend = String(pm10value);    // String to interger conversion

  // Reading PM10.0
  int pm100value = data.PM_AE_UG_10_0;
  PM100ValueSend = String(pm100value);  // String to interger conversion
  
  // Value Show on Display LED dot Matrix
  int a=0;
  int b=0;
  int c=0;

  if(pm25value%10 != 0) 
    a = pm25value%10;
  if(pm25value/10 != 0)
    b = (pm25value/10)%10;
  if(pm25value/100 != 0)
    c = pm25value/100;

 // String url = "http://172.20.3.3/Nodemcu_db_record_view/InsertDB.php?station=bk01&r="+String(pm25value);
  String url = "http://172.20.3.3/dust/InsertDB.php?&n=RAE01&r="+String(pm25value)+"&s="+String(pm10value)+"&t="+String(pm100value);
  Serial.println(url);
  
  if(pms.readUntil(data)){
    http.begin(url);  //Specify request destination
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
   
    int httpCode = http.GET();   //Send the request
    String payload = http.getString();    //Get the response payload

    if( httpCode > 0){        
      Serial.println(httpCode);   //Print HTTP return code
      Serial.println(payload);    //Print request response payload
      Serial.println("PM2.5=" + PM25ValueSend);
      Serial.println("PM1.0=" + PM10ValueSend);
      Serial.println("PM10.0=" + PM100ValueSend);
    }
    else{
      Serial.printf("[HTTP] GET ... Failed, error: %s\n", http.errorToString(httpCode).c_str());
      Serial.print("HttpCode = ");
      Serial.println(httpCode);
    }
    if(b == 0 && c == 0){
      message[0]=' ';
      message[1]='-';
      message[2]='0';
      message[3]='0';
      message[4]=char(a)+48;
      message[5]='-';
      message[6]='\0';
      printText(0, MAX_DEVICES-1, message);
    }
    else if(b == 0 & c != 0){
      message[0]=' ';
      message[1]='-';
      message[2]='0';
      message[3]=char(b)+48;
      message[4]=char(a)+48;
      message[5]='-';
      message[6]='\0';
      printText(0, MAX_DEVICES-1, message);
    }
    else{
      message[0]=' ';
      message[1]='-';
      message[2]=char(c)+48;
      message[3]=char(b)+48;
      message[4]=char(a)+48;
      message[5]='-';
      message[6]='\0';
      printText(0, MAX_DEVICES-1, message);
    }
  }
  else{
    Serial.println("No Data");
    message[0]='N';
    message[1]='O';
    message[2]='D';
    message[3]='A';
    message[4]='T';
    message[5]='A';
    message[6]='\0';
    printText(0, MAX_DEVICES-1, message);
  }
  http.end();  //Close connection

  delay(5000);
}

void printText(uint8_t modStart, uint8_t modEnd, char *pMsg)

{
  uint8_t   state = 0;
  uint8_t    curLen;
  uint16_t  showLen;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do     // finite state machine to print the characters in the space available
  {
    switch(state)
    {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0')
        {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen)  
        {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3: // display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen) 
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}
