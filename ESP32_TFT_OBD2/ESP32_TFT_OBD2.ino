/*
ESP32_TFT_ODB2.ino: OBD2 Program for ESP32 & ELM327. It is based on libraries from other creators.
By Olme Matias
08/20/2023 - 09/29/2023
*/
#include <SD.h>               // SparkFun
#include <SPI.h>              // SparkFun
#include <TFT_eSPI.h>         // Bodmer
#include "Free_Fonts.h"       // Adafruit
#include "ELMduino.h"         // PowerBroker2
#include "BluetoothSerial.h"  // Evandro Luis
#include "ES.h"               // EwoShy


//‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ DISPLAY SETTINGS, AVERAGE, & BLUETOOTH/ELM
TFT_eSPI tft = TFT_eSPI();
BluetoothSerial SerialBT;
ELM327 myELM327;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial
#define TFT_GRAY 0xef7d
#define TFT_DARKRED 0xe000
#define TFT_LIGHTBLUE 0x067f
#define TFT_PURPLE 0xfb3f
#define TFT_SKYBLUE 0x5d7f


//‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ DEFINED VALUES
typedef enum {RPM, MPH, MPG, FUEL} obdState; // OBD2 states
obdState obdGet = RPM; // Starting OBD2 state 
uint16_t rpm;    //////////////////////////
uint16_t mph;    //////////////////////////
uint16_t kph;    //    Used to assign    //
float_t maf;     //  data from ELMduino  //
uint16_t mpg;    //////////////////////////
uint16_t fuel;   //////////////////////////
uint32_t fuelTime = 0; // Used to time obdGet fuel PID
uint16_t maxRPM = 0;   // Used to get max RPM
uint16_t maxMPH = 0;   // Used to get max MPH
uint16_t averageRPM;  /////////////////////////
uint64_t indexRPM;    //  Used to calculate  //
uint64_t totalRPM;    //      average        //
uint16_t averageMPH;  /////////////////////////
uint64_t indexMPH;    /////////////////////////
uint64_t totalMPH;    //  Used to calculate  //
uint16_t averageMPG;  //      average        //
uint64_t indexMPG;    /////////////////////////
uint64_t totalMPG;    /////////////////////////
uint16_t averageFuel; //  Used to calculate  //
uint16_t indexFuel;   //      average        //
uint16_t totalFuel;   /////////////////////////
String timeWhileOn;    ////////////////
uint32_t seconds;      //  Used for  //
uint32_t minutes;      //   timer    //
uint32_t hours;        ////////////////
uint64_t timePrevious;  // Used to calculate delay
bool lock = false;      // Used to calculate delay
String timeDelay;       // Used to calculate delay
uint8_t address[6] = {0xaa, 0xbb, 0xcc, 0x11, 0x22, 0x33}; // OBD2 MAC address, used by SerialBT


void setup() {
  //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ DISPLAY & SERIAL INITIALIZATION
  tft.init();
  tft.setRotation(1);
  DEBUG_PORT.begin(115200);
  ELM_PORT.begin("404 Unknown", true);

  
  //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ BOOTUP SCREEN
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.pushImage(160, 85, 150, 150, es);
  tft.setSwapBytes(false);
  connect();
  initSD();
  delay(3000);
  

  //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ DISPLAY DATA INITIALIZATION
  tft.fillScreen(TFT_BLACK);
  tft.drawFastHLine(250, 160, 480, TFT_GRAY);
  tft.drawFastVLine(250, 0, 320, TFT_GRAY);
  tft.setTextColor(TFT_GRAY, TFT_BLACK);
  tft.setFreeFont(FF41);
  tft.setCursor(5, 15); tft.print("MPG");
  tft.setCursor(255, 15); tft.print("MPH");
  tft.setCursor(255, 178); tft.print("RPM");
}

void connect() {
  //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ WAIT UNTIL CONNECTED
  if (!ELM_PORT.connect(address)) {
    DEBUG_PORT.println("CONNECTION ERROR ->> PHASE #1\nRESTARTING IN 4 SECONDS...");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setFreeFont(FF41);
    tft.setCursor(5, 290); tft.print("CONNECTION ERROR ->> PHASE #1");
    tft.setCursor(5, 310); tft.print("RESTARTING IN 2 SECONDS...");
    delay(2000); ESP.restart();}
  if (!myELM327.begin(ELM_PORT, true, 2000)) {
    Serial.println("CONNECTION ERROR ->> PHASE #2\nRESTARTING IN 4 SECONDS...");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setFreeFont(FF41);
    tft.setCursor(5, 290); tft.print("CONNECTION ERROR ->> PHASE #2");
    tft.setCursor(5, 310); tft.print("RESTARTING IN 2 SECONDS...");
    delay(2000); ESP.restart();}
  Serial.println("CONNECTED TO ELM327!");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(FF41);
  tft.setCursor(5, 290); tft.print("CONNECTED TO ELM327!");
}

void initSD() {
  //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ SD CARD INITIALIZATION
  bool connect = false;
  int count = 0;
  while (!connect) {
    if (SD.begin()) {
      tft.fillRect(5, 310, 400, 15, TFT_BLACK);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setFreeFont(FF41);
      tft.setCursor(5, 310); tft.print("SD CARD INITIALIZED!");
      Serial.println("SD CARD INITIALIZED!!");
      connect = true;}
    else {
      Serial.println("SD CARD INITIALIZATION FAILED!!");
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(5, 310); tft.print("SD CARD INITIALIZATION FAILED!!");
      if (count < 5) {
        count++;}
      else {
        connect = true;
      }
    }
  }
}

void loop() {
  timer();
  switch (obdGet) {
    //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ RPM
    case RPM:{
      if (!lock) {timePrevious = millis(); lock = true;}
      tft.setTextColor(TFT_PURPLE, TFT_BLACK);
      float_t tempRPM = myELM327.rpm();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        rpm = (uint16_t)tempRPM;
        if (rpm <= 999) {
          String strrpm = String(rpm);
          strrpm = ("0" + strrpm);
          tft.drawString(strrpm, 300, 230, 7);}
        else {
          tft.drawString(String(rpm), 300, 230, 7);}
        Serial.print("RPM: "); Serial.println(rpm);
        calculateAverage();
        calculateDelay();
        obdGet = MPH;}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        calculateDelay();
        obdGet = MPH;}
      break;
    }

    //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ SPEED
    case MPH:{
      if (!lock) {timePrevious = millis(); lock = true;}
      tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
      uint16_t tempKPH = myELM327.kph();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        kph = (uint16_t)tempKPH;
        mph = round(kph * 0.621371);
        if (mph <= 9) {
          String strmph = String(mph);
          strmph = ("0" + strmph);
          tft.drawString(strmph, 335, 65, 7);}
        else {
          tft.drawString(String(mph), 335, 65, 7);}
        Serial.print("MPH: "); Serial.println(mph);
        calculateAverage();
        calculateDelay();
        obdGet = MPG;}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        calculateDelay();
        obdGet = MPG;}
      break;
    }

    //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ MPG
    case MPG:{
      if (!lock) {timePrevious = millis(); lock = true;}
      tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
      float_t tempMAF = myELM327.mafRate();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        maf = (float_t)tempMAF;
        mpg = round((14.7 * 6.17 * 4.54 * kph * 0.621371) / (3600 * maf / 100));
        if (mpg > 90) {mpg = 90;}
        if (mpg <= 9) {
          String strmpg = String(mpg);
          strmpg = ("0" + strmpg);
          tft.drawString(strmpg, 65, 95, 8);}
        else {
          tft.drawString(String(mpg), 65, 95, 8);}
        Serial.print("KPH: "); Serial.println(kph);
        Serial.print("MAF: "); Serial.println(maf);
        Serial.print("MPG: "); Serial.println(mpg);
        calculateAverage();
        calculateDelay();
        if (fuelTime == minutes) {
          fuelTime++; obdGet = FUEL;}
        else {obdGet = RPM;}}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        calculateDelay();
        obdGet = FUEL;}
      break;
    }

    //‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ FUEL
    case FUEL:{
      float_t tempFuel = myELM327.fuelLevel();
      tft.setTextColor(TFT_DARKGREEN, TFT_BLACK);
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        fuel = (uint16_t)tempFuel;
        Serial.print("FUEL LEVEL: %"); Serial.println(fuel);
        Serial.print("FUEL LEVEL AVERAGE: %"); Serial.println(averageFuel);
        calculateAverage();
        obdGet = RPM;}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
        obdGet = RPM;}
      break;}

    default:
    obdGet = RPM;
    break;
  }
}

void timer() {
  seconds = millis() / 1000UL;
  seconds = seconds % (24 * 3600);
  hours = seconds / 3600;
  seconds %= 3600;
  minutes = seconds / 60;
  seconds %= 60;

  if (seconds <= 9 && minutes <= 9) {
    String tempSeconds = "0" + String(seconds);
    String tempMinutes = "0" + String(minutes);
    timeWhileOn = (String(hours)+":"+tempMinutes+":"+tempSeconds);}
  else if (seconds <= 9) {
    String tempSeconds = "0" + String(seconds);
    timeWhileOn = (String(hours)+":"+String(minutes)+":"+tempSeconds);}
  else if (minutes <= 9) {
    String tempMinutes = "0" + String(minutes);
    timeWhileOn = (String(hours)+":"+tempMinutes+":"+String(seconds));}
  else { 
    timeWhileOn = (String(hours)+":"+String(minutes)+":"+String(seconds));
  }

  tft.setTextColor(TFT_GRAY, TFT_BLACK);
  tft.drawString(timeWhileOn, 30, 265, 7);
}

void calculateDelay() {
  lock = false;
  timeDelay = String(millis() - timePrevious);
  Serial.println("Delay: " + timeDelay);

  if (timeDelay.toInt() <= 200) {
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);}
  else {
    tft.setTextColor(TFT_DARKRED, TFT_BLACK);
  }
  
  if (timeDelay.toInt() <= 9) {
    timeDelay = ("00" + timeDelay);}
  else if (timeDelay.toInt() <= 99) {
    timeDelay = ("0" + timeDelay);}
  else if (timeDelay.toInt() > 999) {
    timeDelay = "999";
  }

  Serial.println("Calculated delay: " + timeDelay);
  Serial.println("---------------------------\n\n");

  switch (obdGet) {
    case RPM:{
      tft.drawString(timeDelay, 256, 181, 2);
      break;}
    case MPH:{
      tft.drawString(timeDelay, 256, 18, 2);
      break;}
    case MPG:{
      tft.drawString(timeDelay, 6, 18, 2);
      break;}
    case FUEL:{
      break;}
  }
}

void calculateAverage() {
  switch (obdGet) {
    case RPM:{
      totalRPM += rpm;
      indexRPM++;
      averageRPM = totalRPM / indexRPM;
      if (averageRPM <= 999) {
          String stravgrpm = String(averageRPM);
          stravgrpm = ("0" + stravgrpm);
          tft.drawString(stravgrpm, 442, 182, 1);}
      else {
          tft.drawString(String(averageRPM), 442, 182, 1);}
      if (rpm > maxRPM) {
        maxRPM = rpm;
        tft.drawString(String(maxRPM), 442, 165, 1);}
      break;}

    case MPH:{
      if (mph != 0) {
        totalMPH += mph;
        indexMPH++;
        averageMPH = totalMPH / indexMPH;
        if (averageMPH <= 9) {
          String stravgmph = String(averageMPH);
          stravgmph = ("0" + stravgmph);
          tft.drawString(stravgmph, 460, 22, 1);}
        else {
          tft.drawString(String(averageMPH), 460, 22, 1);}
        if (mph > maxMPH) {
          maxMPH = mph;
          tft.drawString(String(maxMPH), 460, 5, 1);}}
      break;}

    case MPG:{
      if (mph != 0) {
        totalMPG += mpg;
        indexMPG++;
        averageMPG = totalMPG / indexMPG;
        if (averageMPG <= 9) {
          String stravgmpg = String(averageMPG);
          stravgmpg = ("0" + stravgmpg);
          tft.drawString(stravgmpg, 65, 95, 8);}
        else {
          tft.drawString(String(averageMPG), 215, 27, 4);}}
      break;}

    case FUEL:{
      totalFuel += fuel;
      indexFuel++;
      averageFuel = totalFuel / indexFuel;
      if (averageFuel >= 100) {
        tft.drawString(String(averageFuel), 200, 5, 4);}
      else {
        tft.fillRect(200, 5, 45, 20, TFT_BLACK);
        tft.drawString(String(averageFuel), 215, 5, 4);}
      if (indexFuel >= 5) {
        totalFuel = 0;
        indexFuel = 0;}
      break;}
  }
}