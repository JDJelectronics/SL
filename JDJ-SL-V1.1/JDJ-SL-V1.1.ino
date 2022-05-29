// Made by Robert van den Breemen for Jelmer de Jong
// version 0.0.1
// when 24 juli 2020

#include <WiFi.h>
#include <DNSServer.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
WiFiServer server(80);

String responseHTML = ""
  "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
  "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
  "be redirected here.</p></body></html>";


//Als je een SSD1306 schermpje hebt, dan de volgende uncomment, anders zo laten.
#define USE_SSD1306
#ifdef USE_SSD1306
  #include <Wire.h>  
  #include <U8g2lib.h>
  #define pinSDA  21
  #define pinSCL  22
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, pinSCL, pinSDA, U8X8_PIN_NONE);
  //wat zou de 128 bij 64 
  //welk font
#endif 
       


// setting PWM properties
const int freq        = 5000;
const int resolution  = 8;      //Resolution 8, 10, 12, 15
// ESP32 heeft 16 kanalen PMW, van 0 tot 15.


//wachttijden spreekd voor zich
#define FADE_IN_DELAY          1     //ms = deplay per verhoging
#define FADE_IN_AMOUNT         1     //per xx ophogen
#define DELAY_NEXT_TREDE_OFF   1   //ms = tijd tussen tredes 

#define FADE_OUT_DELAY         1     //ms = delay per verlaging
#define FADE_OUT_AMOUNT        5     //per xx verlagen
#define DELAY_NEXT_TREDE_ON    70   //ms = tijd tussen tredes 

#define DELAY_BEFORE_FADEOUT   10000  //ms = wachttijd dat de leds aan blijven staan

//tredeLeds array van AANTAL_TREDES, van 0-AANTAL_TREDES = { gpio 36, gpio 39, etc. }
#define AANTAL_TREDES 13             //maximaal 16
//int tredeLeds[AANTAL_TREDES] = {25, 26, 27, 14, 12, 13, 15, 4, 16, 17, 5, 18, 19}; //moeten wel 13 pin zijn het er
//int tredeLeds[AANTAL_TREDES] = {32, 33, 25, 26, 27, 14, 12, 13, 15, 4, 16, 17, 5}; //moeten wel 13 pin zijn het er
int tredeLeds[AANTAL_TREDES] = {32, 33, 25, 26, 27, 14, 12, 13, 15, 4, 16, 17, 5}; //moeten wel 13 pin zijn het 
//PIR sensors
#define SENSOR_BOVEN_PIN  35                       // Pir boven
#define SENSOR_ONDER_PIN  34                       // Pir onder 
boolean sensorUpperActive = false;
boolean sensorBottomActive = false;                                 

void initOled()
{
    u8g2.begin();
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFont(u8g2_font_helvB08_tr); // choose a suitable font
    u8g2.drawStr(25, 10, "JDJ electronics");
    u8g2.drawStr(2, 30, "Model: SL");
    u8g2.drawStr(2, 40, "FW: v1.1");
    u8g2.drawStr(2, 50, "D: 20072020");
    u8g2.drawStr(2, 60, "DEBUG Set 0-1");
    u8g2.sendBuffer();          // transfer internal memory to the display
    delay(2000);
}

void ClearOled()
{
  
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_helvB08_tr); // choose a suitable font
  u8g2.sendBuffer();          // transfer internal memory to the display
  
}

void PrintFadeIn(int t)
{
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_helvB08_tr); // choose a suitable font
  u8g2.drawStr(20, 10, "Turn ON Stair nr");
  u8g2.setFont(u8g2_font_helvB14_tr);
  u8g2.setCursor(50, 40); 
  u8g2.println(t);
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void PrintFadeOut(int t)
{
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_helvB08_tr); // choose a suitable font  u8g2.drawStr(25, 20, "FadeOut Trede =");
  u8g2.drawStr(20, 10, "Turn OFF Stair nr");
  u8g2.setFont(u8g2_font_helvB14_tr);
  u8g2.setCursor(50, 40); 
  u8g2.println(t);
  u8g2.sendBuffer();          // transfer internal memory to the display
}


void BottomTriggerFire() 
{
  //if (analogRead(SENSOR_BOVEN_PIN) >= 0 )           //Depend of the sensor type, if 0 when triggered ,than change the comparison to opposite value
//{      

 //aanzetten van tredes, 1 voor 1, met fadein
  for (int t=0; t<=AANTAL_TREDES; t++)
  { 
    ClearOled();
    TredeFadeIn(t);
    delay(DELAY_NEXT_TREDE_ON);
  }
  //wachttijd voor uitschakelen
  delay(DELAY_BEFORE_FADEOUT);
  //uitzetten van tredes, 1 voor 1, met fadeout
  for (int t=0; t<=12; t++)
  {
    ClearOled();
    TredeFadeOut(t);
    delay(DELAY_NEXT_TREDE_OFF);
  }
  ClearOled();
  //}
}

void UpperTrigerFire() 
{

 // if (analogRead(SENSOR_ONDER_PIN) >= 0) 
  //{


  //aanzetten van tredes van boven naar beneden, 1 voor 1, met fadein
  for (int t=AANTAL_TREDES; t>=0; t--)
  { 
    ClearOled();
    TredeFadeIn(t);
    delay(DELAY_NEXT_TREDE_ON);
  }
  //wachttijd voor uitschakelen
  delay(DELAY_BEFORE_FADEOUT);
  //uitzetten van tredes, 1 voor 1, met fadeout
  for (int t=12; t>=0; t--) 
  {
    ClearOled();
    TredeFadeOut(t);
    delay(DELAY_NEXT_TREDE_OFF);
  }
  ClearOled();

 // }
}


void TredeOn(int i){
  pinMode (tredeLeds[i], OUTPUT);
  digitalWrite (tredeLeds[i], HIGH);  
}

void TredeOff(int i){
  pinMode (tredeLeds[i], OUTPUT);
  digitalWrite (tredeLeds[i], LOW);  
}

void TredeFadeOut(int t){
  //print trede op fade
  PrintFadeOut(t);
  // configure LED PWM functionalitites
  ledcSetup(t, freq, resolution);
  // attach tredeLed[i] aan channel i 
  ledcAttachPin(tredeLeds[t], t);
  //fade in per ledstrip
  for(int i=255; i>=0; i=i-FADE_OUT_AMOUNT)
  {
      ledcWrite(t, i);
      delay(FADE_OUT_DELAY);
  } 
}

void TredeFadeIn(int t){
  //print trede op fade
  PrintFadeIn(t);
  // configure LED PWM functionalitites
  ledcSetup(t, freq, resolution);
  // attach tredeLed[i] aan channel i 
  ledcAttachPin(tredeLeds[t], t);
  //fade in per ledstrip
  for(int i=0; i<=255; i=i+FADE_IN_AMOUNT)
  {
      ledcWrite(t, i);
      delay(FADE_IN_DELAY);
  } 
}

void setup() 
{
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("JDJ-SL");

  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  dnsServer.start(DNS_PORT, "*", apIP);

  server.begin();


  initOled();

  //setup the PIR sensors
  pinMode(SENSOR_BOVEN_PIN, INPUT);
  pinMode(SENSOR_ONDER_PIN, INPUT);

    

  //setup the GPIO for all led strips
  for (int i=0; i<=AANTAL_TREDES; i++)
  { 
    //making sure that the LED is in output mode
    pinMode(tredeLeds[i],OUTPUT);
    TredeOff(i);
    // configure LED PWM functionalitites
    ledcSetup(i, freq, resolution);
    // attach tredeLed[i] aan channel i 
    ledcAttachPin(tredeLeds[i], i);
  }
    
    Serial.begin(9600);

#ifdef USE_SSD1306
    
  

    delay(10);
#endif
  
}

void loop() 
{             

  dnsServer.processNextRequest();
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(responseHTML);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
                                                             
      BottomTriggerFire();                           //Checking bottom sensor
      delay(3000);
      UpperTrigerFire();                             //Checking upper sensor     
      delay(3000);

}




  
    
    

        

  
