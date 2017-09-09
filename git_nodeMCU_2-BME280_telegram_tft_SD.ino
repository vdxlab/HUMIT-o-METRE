VERSIÓ PER A PENJAR AL GIT.
CAL REVISAR LES VARIABLES DE L'SSID I PASWORD DE LA WIFI
AIXÍ COM EL BOT_TOKEN I EL CHAT_ID PER FER FUNCIONAR EL TELEGRAMbot.

//nodeMCU v0.9 (blue) with Arduino IDE
//stream temperature and humity data from TWO BME280 with I2C on ESP8266 ESP12 (nodeMCU v0.9)
// data is stored in SD card, shown on tft oled screen and send it through telegrambot
//nodemcu pinout https://github.com/esp8266/Arduino/issues/584

/*
IO index     ESP8266 pin                    FUNCIO HUITOMETRE
  0 [*]       GPIO   16   --->   pulsador
  1           GPIO    5   --->   I2C                      SCL  
  2           GPIO    4   --->   I2C                      SDA 
  3           GPIO    0   --->           TFT_retroLED
  4           GPIO    2   --->   SPI                                SD_CS  

  5           GPIO  14   --->   SPI      TFT_SCLK                   SD_SCK
  6           GPIO  12   --->   SPI      TFT_DC   (A0)              SD_MISO
  7           GPIO  13   --->   SPI      TFT_MOSI                   SD_MOSI
  8           GPIO  15   --->   SPI      TFT_CS                 

  9          GPIO    3   --->   RX
 10          GPIO    1   --->   TX
 11          GPIO    9   --->   ???
 12          GPIO  10   --->   ???

A0  ------------------------------------------>  POTENCIOMETRE / SELECTOR ROTATIU                            
*/

//  nomes queda un pin digital lliure (D0=GPIO16), potser per un boto amb multifuncio ?
//    1 toque rapid encen o apaga l'iluminacio de la pantalla
//    1 toque prolongat (8 segons) reinicia l'humitometre

//  al pin A0 li podirem posar un potenciomete o selector rotatiu amb diferents resistències
//  que ens faci canviar les dades a la pantalla.
//  100ohms-->
//  200ohms-->
//  300ohms-->
//  400ohms-->
//  500ohms-->
//  ...
//
//  i amb combinació amb el pulsador possibilitats infinites..
// 
// 
// possibilitat d'afegir-hi un RTC per registrar el temps de les mesures. 
// ¿ potser podriem preguntar la hora mitjançant el robot del telegram ?.. pero nomes si tenim connexió a internet, clar..




#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include "BlueDot_BME280.h"
#include <SD.h>

extern "C" {                        // WATCHDOG PEL ESP8862
#include "user_interface.h"         // WATCHDOG PEL ESP8862
}                                   // WATCHDOG PEL ESP8862

#include <Adafruit_GFX.h>           // Core graphics library
#include <Adafruit_ST7735.h>        // Hardware-specific library
#include <SPI.h>



//Def
#define myPeriodic 60               //in sec | TEMPS ENTRE MOSTRES

#define scl 5    // D1 = GPIO5;      // variables per definir els pins de l'I2C al nodeMCU   ///// I2C PINS !!!!
#define sda 4    // D2 = GPIO4;      // variables per definir els pins de l'I2C al nodeMCU   ///// I2C PINS !!!!

BlueDot_BME280 bme1;                //Object for Sensor 1
BlueDot_BME280 bme2;                //Object for Sensor 2

int bme1Detected = 0;               //Checks if Sensor 1 is available
int bme2Detected = 0;               //Checks if Sensor 2 is available

float temp_1 ;
float humi_1 ;
float temp_2 ;
float humi_2 ;

float ajustST1 = 0;                  //  ajusta el desfase del sensor de temperatura
float ajustST2 = 0;
float ajustSH1 = 0;                  //  ajusta el desfase del sensor d'humitat
float ajustSH2 = 1;

int test = 0;                       //numero de mesures
int sent = 0;                       //numero de missatges enviats
int SDerror = 0;                    //detecta l'error en l'escriptura a la SD
int SDerrorCOUNT = 0;                // conta el numero d'errors de la SD

unsigned long previousMilis = 0 ;
unsigned long currentMillis = 0 ;
int contectionTime = 15000 ;


// Initialize Wifi connection to the router
char ssid_1[] = "ssid_1";       // your network SSID (name)       // RUBI
char password_1[] = "password_1";     // your network key
char ssid_2[] = "ssid_2";          // your network SSID (name)         //  TALAIA
char password_2[] = "password_2";    // your network key
//char ssid_3[] = "ssid_3";       // your network SSID (name)         // TALARN
//char password_3[] = "password_3"; // your network key
byte nowifi = 0 ;
IPAddress ip;                         // the IP address of your shield




#define BOTtoken "BOT_token:token_BOT"  // Bot Token (Get from Botfather)     //  HUMITROMETRE_BOT
#define chatID "chat_ID" // https://stackoverflow.com/questions/32423837/telegram-Termometre_rubi_botbot-how-to-get-a-group-chat-id-ruby-gem-telegram-bot


WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

bool Start = false;


// For the breakout, you can use any 2 or 3 pins                                      // TFT
// These pins will also work for the 1.8" TFT shield                                  // TFT
#define TFT_CS     15       // D8 = GPIO15;                                           // TFT
#define TFT_RST    -1  // you can also connect this to the Arduino reset              // TFT
                      // in which case, set this #define pin to 0! (OR -1)            // TFT
#define TFT_DC     12       // D6 = GPIO12                                            // TFT
                                                                          
// Option 1 (recommended): must use the hardware SPI pins                             // TFT
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be                          // TFT
// an output. This is much faster - also required if you want                         // TFT
// to use the microSD card (see the image drawing example)                            // TFT
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);                    // TFT

// Option 2: use any pins but a little slower!                                        // TFT
#define TFT_SCLK 14   // set these to be whatever pins you like!                      // TFT
#define TFT_MOSI 13   // set these to be whatever pins you like!                      // TFT
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);   // TFT

#define TFT_retroLED 0    // D3 = GPIO0;      // pin per iluminar la pantalla         // TFT

#define SD_CS 2          // D4 = GPIO2;      // Chip select line for SD card                   // SD  




void setup() {
  Serial.begin(115200);
    Serial.println("SETUP :");  
  
  Wire.begin( sda, scl);

  Serial.println(F("Detectant sensors BME280"));
  //*********************************************************************
  //*************BASIC SETUP - SAFE TO IGNORE**************************** 
  //This program is set for the I2C mode
    bme1.parameter.communication = 0;                    //I2C communication for Sensor 1 (bme1)
    bme2.parameter.communication = 0;                    //I2C communication for Sensor 2 (bme2)
  //*********************************************************************
  //*************BASIC SETUP - SAFE TO IGNORE****************************
  //Set the I2C address of your breakout board  
    bme1.parameter.I2CAddress = 0x76;                    //I2C Address for Sensor 1 (bme1)
    bme2.parameter.I2CAddress = 0x77;                    //I2C Address for Sensor 2 (bme2)
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE*************************
  //Now choose on which mode your device will run
  //On doubt, just leave on normal mode, that's the default value
  //0b00:     In sleep mode no measurements are performed, but power consumption is at a minimum
  //0b01:     In forced mode a single measured is performed and the device returns automatically to sleep mode
  //0b11:     In normal mode the sensor measures continually (default value)
    bme1.parameter.sensorMode = 0b11;                    //Setup Sensor mode for Sensor 1
    bme2.parameter.sensorMode = 0b11;                    //Setup Sensor mode for Sensor 2 
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE*************************
  //Great! Now set up the internal IIR Filter
  //The IIR (Infinite Impulse Response) filter suppresses high frequency fluctuations
  //In short, a high factor value means less noise, but measurements are also less responsive
  //You can play with these values and check the results!
  //In doubt just leave on default
  //0b000:      factor 0 (filter off)
  //0b001:      factor 2
  //0b010:      factor 4
  //0b011:      factor 8
  //0b100:      factor 16 (default value)
    bme1.parameter.IIRfilter = 0b100;                   //IIR Filter for Sensor 1
    bme2.parameter.IIRfilter = 0b100;                   //IIR Filter for Sensor 2
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE*************************
  //Next you'll define the oversampling factor for the humidity measurements
  //Again, higher values mean less noise, but slower responses
  //If you don't want to measure humidity, set the oversampling to zero
  //0b000:      factor 0 (Disable humidity measurement)
  //0b001:      factor 1
  //0b010:      factor 2
  //0b011:      factor 4
  //0b100:      factor 8
  //0b101:      factor 16 (default value)
    bme1.parameter.humidOversampling = 0b101;            //Humidity Oversampling for Sensor 1
    bme2.parameter.humidOversampling = 0b101;            //Humidity Oversampling for Sensor 2
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE*************************
  //Now define the oversampling factor for the temperature measurements
  //You know now, higher values lead to less noise but slower measurements
  //0b000:      factor 0 (Disable temperature measurement)
  //0b001:      factor 1
  //0b010:      factor 2
  //0b011:      factor 4
  //0b100:      factor 8
  //0b101:      factor 16 (default value)
    bme1.parameter.tempOversampling = 0b101;              //Temperature Oversampling for Sensor 1
    bme2.parameter.tempOversampling = 0b101;              //Temperature Oversampling for Sensor 2
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE*************************
  //Finally, define the oversampling factor for the pressure measurements
  //For altitude measurements a higher factor provides more stable values
  //On doubt, just leave it on default
  //0b000:      factor 0 (Disable pressure measurement)
  //0b001:      factor 1
  //0b010:      factor 2
  //0b011:      factor 4
  //0b100:      factor 8
  //0b101:      factor 16 (default value)  
    bme1.parameter.pressOversampling = 0b101;             //Pressure Oversampling for Sensor 1
    bme2.parameter.pressOversampling = 0b101;             //Pressure Oversampling for Sensor 2 
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE*************************
  //For precise altitude measurements please put in the current pressure corrected for the sea level
  //On doubt, just leave the standard pressure as default (1013.25 hPa);
    bme1.parameter.pressureSeaLevel = 1013.25;            //default value of 1013.25 hPa (Sensor 1)
    bme2.parameter.pressureSeaLevel = 1013.25;            //default value of 1013.25 hPa (Sensor 2)
  //Also put in the current average temperature outside (yes, really outside!)
  //For slightly less precise altitude measurements, just leave the standard temperature as default (15°C and 59°F);
    bme1.parameter.tempOutsideCelsius = 15;               //default value of 15°C
    bme2.parameter.tempOutsideCelsius = 15;               //default value of 15°C
    bme1.parameter.tempOutsideFahrenheit = 59;            //default value of 59°F
    bme2.parameter.tempOutsideFahrenheit = 59;            //default value of 59°F
  //*********************************************************************
  //*************ADVANCED SETUP - SAFE TO IGNORE!************************
  //The Watchdog Timer forces the Arduino to restart whenever the program hangs for longer than 8 seconds.
  //This is useful when the program enters an infinite loop and stops giving any feedback on the serial monitor.
  //However the Watchdog Timer may also be triggered whenever a single program loop takes longer than 8 seconds.
  //Per default the Watchdog Timer is turned off (commented out).
  //Do you need to run the Arduino for a long time without supervision and your program loop takes less than 8 seconds? Then remove the comments below!
  //wdt_enable(WDTO_8S);                                 //Watchdog Timer counts for 8 seconds before starting the reset sequence
  //*********************************************************************
  
  //*************ADVANCED SETUP IS OVER - LET'S CHECK THE CHIP ID!*******
  if (bme1.init() != 0x60)
  {    
    Serial.println(F("                              Ops! First BME280 Sensor not found!"));
    bme1Detected = 0;
  }
  else
  {
    Serial.println(F("First BME280 Sensor detected!"));
    bme1Detected = 1;
  }
  if (bme2.init() != 0x60)
  {    
    Serial.println(F("                              Ops! Second BME280 Sensor not found!"));
    bme2Detected = 0;
  }
  else
  {
    Serial.println(F("Second BME280 Sensor detected!"));
    bme2Detected = 1;
  }
  if ((bme1Detected == 0)&(bme2Detected == 0))
  {
    Serial.println();
    Serial.println();
    Serial.println(F("Troubleshooting Guide"));
    Serial.println(F("*************************************************************"));
    Serial.println(F("1. Let's check the basics: Are the VCC and GND pins connected correctly? If the BME280 is getting really hot, then the wires are crossed."));
    Serial.println();
    Serial.println(F("2. Did you connect the SDI pin from your BME280 to the SDA line from the Arduino?"));
    Serial.println();
    Serial.println(F("3. And did you connect the SCK pin from the BME280 to the SCL line from your Arduino?"));
    Serial.println();
    Serial.println(F("4. One of your sensors should be using the alternative I2C Address(0x76). Did you remember to connect the SDO pin to GND?"));
    Serial.println();
    Serial.println(F("5. The other sensor should be using the default I2C Address (0x77. Did you remember to leave the SDO pin unconnected?"));
    Serial.println();
    while(1);
  }
  Serial.println();
  Serial.println(F("-----------------------------------------------------------------------------"));




// Use this initializer if you're using a 1.8" TFT                                  // TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab              // TFT
  // Use this initializer (uncomment) if you're using a 1.44" TFT                   // TFT
  //tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab         // TFT

  pinMode(TFT_retroLED, OUTPUT);
  digitalWrite(TFT_retroLED, HIGH);   // turn the LED on (HIGH is the voltage level)

  Serial.println("TFT Initialized");                                                 // TFT

  tft.fillScreen(ST7735_BLACK);   // pantalla a NEGRE                                // TFT
  tft.setRotation(tft.getRotation()-1);                                              // TFT



  
  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected                                                                  // WIFI
  WiFi.mode(WIFI_STA);                                                          // WIFI
  WiFi.disconnect();                                                            // WIFI
delay(100); 

 // attempt to connect to Wifi network:                                         // WIFI
  Serial.println();
  Serial.print("Connecting Wifi_1: ");                                          // WIFI
  Serial.println(ssid_1);
  tft.setCursor(0, 0);                                                        
  drawtext(" Connecting Wifi_1:  ", ST7735_WHITE);                              
  tft.setCursor(10, 10);                                              
  drawtext(ssid_1, ST7735_WHITE);
  WiFi.begin(ssid_1, password_1);                                             

  previousMilis = millis();

  tft.setCursor(0, 20);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    drawtext(" . ", ST7735_WHITE);
    delay(500);
    currentMillis = millis();
    if(currentMillis - previousMilis > contectionTime) {
       // attempt to connect to Wifi network:
        Serial.println("");
        Serial.print("Connecting Wifi_2: ");
        Serial.println(ssid_2);
        tft.fillScreen(ST7735_BLACK);   // pantalla a NEGRE                         // TFT
        tft.setCursor(0, 0);                                                        // TFT
        drawtext(" Connecting Wifi_2:  ", ST7735_WHITE);                            // TFT
        tft.setCursor(10, 10);                                                      // TFT
        drawtext(ssid_2, ST7735_WHITE);                                             // TFT
        tft.setCursor(0, 20);                                                       // TFT
        WiFi.begin(ssid_2, password_2);    
        nowifi++ ;  
        previousMilis = millis();    
    }
      if(currentMillis - previousMilis > contectionTime && nowifi > 1) {
        goto nowifimode;
      }
  }

  if (WiFi.status() == WL_CONNECTED) {
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  tft.setCursor(0, 70);
  drawtext(" WiFi connected ", ST7735_WHITE);
  }

nowifimode:


    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("");  Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("WiFi NOOOOOT connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      tft.fillScreen(ST7735_BLACK);   // pantalla a NEGRE                         // TFT
      tft.setCursor(0, 50);
      drawtext("  IP address:  ", ST7735_WHITE);
      tft.setCursor(0, 60);
      tft.print(WiFi.localIP());
      tft.setCursor(0, 70);
      drawtext(" WiFi NOOOOT connected ", ST7735_WHITE);
    }
  
  delay(100);



  Serial.println(F("-----------------------------------------------------------------------------"));


inicialitzacioSD();
   
   
   Serial.println(F("-----------------------------------------------------------------------------"));




}









  //*********************************************************************
  //*************NOW LET'S START MEASURING*******************************


void loop() {

  test++;
  
  wdt_reset();                                               //This function resets the counter of the Watchdog Timer. Always use this function if the Watchdog Timer is enabled.
  
//  LECTURA I AJUST DEL SENSOR
  temp_1 = bme1.readTempC()+ajustST1;
  humi_1 = bme1.readHumidity()+ajustSH1;
  temp_2 = bme2.readTempC()+ajustST2;
  humi_2 = bme2.readHumidity()+ajustSH2;


// GUARDANT DADES A SD
// make a string for assembling the data to log:
  String dataString = "";
    dataString += String(test);
    dataString += ",";
    dataString += String(temp_1);
    dataString += ",";
    dataString += String(humi_1);
    dataString += ",";
    dataString += String(temp_2);
    dataString += ",";
    dataString += String(humi_2);
    dataString += ";";

  SD.begin(SD_CS);                                        // sense aquesta lina cada cop al loop , no funca la SD
  File dataFile = SD.open("HUMIlog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    SDerror = 0;
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    SDerror = 1;
    SDerrorCOUNT++;
    Serial.print(SDerrorCOUNT);
    Serial.print("  error opening datalog.txt           ");
    Serial.println(dataString);
  }


  
//  SERIAL PRINT
  Serial.println(F("-----------------------------------------------------------------------------"));
  Serial.print(F("Duration in Seconds:  "));
  Serial.println(float(millis())/1000);

  if (bme1Detected)
  {
    Serial.print(F("Temperature Sensor 1 [°C]:\t\t")); 
    Serial.println(temp_1,1);
    //Serial.print(F("Temperature Sensor 1 [°F]:\t\t"));            
    //Serial.println(bme1.readTempF());
    Serial.print(F("Humidity Sensor 1 [%]:\t\t\t\t")); 
    Serial.println(humi_1,1);
    Serial.println(F("****************************************"));    
  }

  else
  {
    Serial.print(F("Temperature Sensor 1 [°C]:\t\t")); 
    Serial.println(F("Null"));
    //Serial.print(F("Temperature Sensor 1 [°F]:\t\t")); 
    //Serial.println(F("Null"));
    Serial.print(F("Humidity Sensor 1 [%]:\t\t\t")); 
    Serial.println(F("Null"));
    Serial.println(F("****************************************"));   
  }

  if (bme2Detected)
  {
    Serial.print(F("Temperature Sensor 2 [°C]:\t\t")); 
    Serial.println(temp_2,1);
    //Serial.print(F("Temperature Sensor 2 [°F]:\t\t")); 
    //Serial.println(bme2.readTempF());
    Serial.print(F("Humidity Sensor 2 [%]:\t\t\t\t")); 
    Serial.println(humi_2,1);
  }

  else
  {
    Serial.print(F("Temperature Sensor 2 [°C]:\t\t")); 
    Serial.println(F("Null"));
    //Serial.print(F("Temperature Sensor 2 [°F]:\t\t")); 
    //Serial.println(F("Null"));
    Serial.print(F("Humidity Sensor 2 [%]:\t\t\t")); 
    Serial.println(F("Null"));
  }
   
   Serial.println();

// AJUST DEL SENSOR (serial print)
    Serial.print("ajust del sensor 2, TEMPERATURA :  ");
    Serial.println(bme1.readTempC()-bme2.readTempC());
    Serial.print("ajust del sensor 2, HUMITAT :  ");
    Serial.println(bme1.readHumidity()-bme2.readHumidity());
   
   Serial.println();

   delay(100);
   
  
                                                                      //if (temp != prevTemp)
                                                                      //{
                                                                      //sendTeperatureTS(temp);
                                                                      //prevTemp = temp;
                                                                      //}
//  SEND MESSAGES
  //if (temp != prevTemp)
  sendMessage();
  sent++;
  //prevTemp = temp;



//  TFT PRINT

  iniciaTFT();

  tft.setTextColor(ST7735_WHITE); //Color de texto
  tft.setTextSize(1);         // Tamaño texto
  tft.setCursor(0, 0);
  tft.print(test);   
  tft.setCursor(125, 0);
  tft.print(sent);  

//  TEMPERATURA
  tft.setCursor(0, 10);
  tft.setTextColor(ST7735_GREEN); //Color de texto
  tft.setTextSize(2);         // Tamaño texto
  tft.print(" Temperatura: ");

  tft.setCursor(0, 30);
  tft.setTextColor(ST7735_MAGENTA); //Color de texto
  tft.setTextSize(3);         // Tamaño texto
  tft.print(temp_1,1);

  tft.setCursor(80, 30);
  tft.setTextColor(ST7735_RED); //Color de texto
  tft.setTextSize(3);         // Tamaño texto
  tft.print(temp_2,1);


// HUMITAT    
  tft.setCursor(0, 60);
  tft.setTextColor(ST7735_CYAN); //Color de texto
  tft.setTextSize(2);         // Tamaño texto
  tft.print(" Humitat: ");

  tft.setCursor(0, 80);
  tft.setTextColor(ST7735_MAGENTA); //Color de texto
  tft.setTextSize(3);         // Tamaño texto
  tft.print(humi_1,1);

  tft.setCursor(80, 80);
  tft.setTextColor(ST7735_RED); //Color de texto
  tft.setTextSize(3);         // Tamaño texto
  tft.print(humi_2,1);


   if (SDerror != 0) {                                    // NO funca l'SD
       tft.setCursor(20,0);
       tft.setTextColor(ST7735_YELLOW); //Color de texto
       tft.setTextSize(1);         // Tamaño texto
       tft.print("SD ERROR !! ");        
       tft.print(SDerrorCOUNT);        
   }

   if (nowifi > 1) {                                    // NO WIFI
       tft.setCursor(20,110);
       tft.setTextColor(ST7735_YELLOW); //Color de texto
      tft.setTextSize(1);         // Tamaño texto
       tft.print("NO WIFI CONNECTED !!");
   }


  


  int count = myPeriodic;
  while(count--)
  delay(1000);
}
    /////////  END LOOOP   ////////



void drawtext(char *text, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}



void sendMessage() {
      bot.sendMessage(chatID, String(sent) +"                                                                Temp_1: "+ String(temp_1,1)+"ºC "+" Humi_1: "+ String(humi_1,1)+"%" +"                         Temp_2: "+ String(temp_2,1)+"ºC "+" Humi_2: "+ String(humi_2,1)+"%");
}


void inicialitzacioSD(){
  Serial.print("Initializing SD card...");                                 // SD
// see if the card is present and can be initialized:
  if (!SD.begin(SD_CS)) {
    Serial.println("Card failed, or not present");
       iniciaTFT();
       tft.setTextColor(ST7735_YELLOW); //Color de texto
       tft.setCursor(0,20);
       tft.setTextSize(2);         // Tamaño texto
       tft.print("SD ERROR !! ");        
       tft.setCursor(0,60);
       tft.setTextSize(1);         // Tamaño texto
       tft.print("Card failed,");
       tft.setCursor(0,70);
       tft.print("or not present");        
    // don't do anything more:
    return;
  }
    Serial.println("OK!");                                                 // SD
       iniciaTFT();
       tft.fillScreen(ST7735_BLACK);   // pantalla a NEGRE                          // TFT
       tft.setCursor(0,20);
       tft.setTextColor(ST7735_GREEN); //Color de texto
       tft.setTextSize(2);         // Tamaño texto
       tft.print("SD OK !! ");               
}

void iniciaTFT(){
      // Use this initializer if you're using a 1.8" TFT                             // TFT
       tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab         // TFT
       tft.fillScreen(ST7735_BLACK);   // pantalla a NEGRE                          // TFT
       tft.setRotation(tft.getRotation()-1);                                        // TFT
}
