/*








  TESTING ONLY


    480x320
    240x320






*/

#include <SD.h>               // SparkFun
#include <SPI.h>              // SparkFun
#include <TFT_eSPI.h>         // Bodmer
#include <User_Setup_Select.h>
#include "Free_Fonts.h"       // Adafruit
#include "ELMduino.h"         // PowerBroker2
#include "BluetoothSerial.h"  // Evandro Luis
#include "ES.h"               // EwoShy


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


typedef enum {RPM, SPEED, MPG, FUEL} obdState; // OBD2 states
obdState obdGet = RPM; // Starting OBD2 state 
uint16_t rpm;    //////////////////////////
uint16_t mph;    //    Used ot assign    //
uint16_t kph;    //      data from       //
float_t maf;     //       ELMduino       //
uint16_t mpg;    ////////////////////////// 
uint16_t fuel;   
uint16_t maxRPM = 0;   // Used to get max RPM
uint16_t maxMPH = 0;   // Used to get max MPH
uint16_t averageRPM;  /////////////////////////
uint64_t indexRPM;    //  Used to calculate  //
uint64_t totalRPM;    //      average        //
uint16_t averageMPH;  /////////////////////////
uint64_t indexMPH;    /////////////////////////
uint64_t totalMPH;    /////////////////////////
uint16_t averageMPG;  //  Used to calculate  //
uint64_t indexMPG;    //      average        //
uint64_t totalMPG;    /////////////////////////
String timeWhileOn;    ////////////////
uint32_t seconds;      //  Used for  //
uint32_t minutes;      //    timer   //
uint32_t hours;        ////////////////
uint64_t timePrevious;  // Used to calculate delay
bool lock = false;      // Used to calculate delay
String timeDelay;       // Used to calculate delay
uint8_t addressTesting[6] = {0x00, 0x10, 0xcc, 0x4f, 0x36, 0x03}; // TESTING ONLY


void setup() {
  tft.init();
  tft.setRotation(1);
  DEBUG_PORT.begin(115200);
  ELM_PORT.begin("404 Unknown", true);
  connect();


  if (!SD.begin()) {
    Serial.println("SD CARD INITIALIZATION FAILED!!");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 310); tft.print("SD CARD INITIALIZATION FAILED!!");} 
  else {
    tft.setCursor(5, 310); tft.print("SD CARD INITIALIZED!");}
  delay(3000);


  tft.fillScreen(TFT_BLACK);
  tft.drawFastHLine(250/2, 160, 480/2, TFT_GRAY);
  tft.drawFastVLine(250/2, 0, 320, TFT_GRAY);
  tft.setTextColor(TFT_GRAY, TFT_BLACK);
  tft.setFreeFont(FF41);
  tft.setCursor(5, 15); tft.print("MPG");
  tft.setCursor(255/2, 15); tft.print("SPEED");
  tft.setCursor(255/2, 178); tft.print("RPM");
}

void connect() {
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.pushImage(0, 0, 150, 150, es);
  tft.setSwapBytes(false);
  

  if (!ELM_PORT.connect(addressTesting)) {
    DEBUG_PORT.println("CONNECTION ERROR ->> PHASE #1\nRESTARTING IN 4 SECONDS...");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setFreeFont(FF41);
    tft.setCursor(5, 290); tft.print("CONNECTION ERROR ->> PHASE #1");
    tft.setCursor(5, 310); tft.print("RESTARTING IN 4 SECONDS...");
    delay(4000); ESP.restart();}
  if (!myELM327.begin(ELM_PORT, true, 2000)) {
    Serial.println("CONNECTION ERROR ->> PHASE #2\nRESTARTING IN 4 SECONDS...");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setFreeFont(FF41);
    tft.setCursor(5, 290); tft.print("CONNECTION ERROR ->> PHASE #2");
    tft.setCursor(5, 310); tft.print("RESTARTING IN 4 SECONDS...");
    delay(4000); ESP.restart();}
  Serial.println("CONNECTED TO ELM327!");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(FF41);
  tft.setCursor(5, 290); tft.print("CONNECTED TO ELM327!");
}

void loop() {
  timer();
  switch (obdGet) {
    case RPM:{
      if (!lock) {timePrevious = millis(); lock = true;}
      tft.setTextColor(TFT_PURPLE, TFT_BLACK);
      float_t tempRPM = myELM327.rpm();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        rpm = (uint16_t)tempRPM;
        calculateAverage();
        if (rpm <= 999) {
          String strrpm = String(rpm);
          strrpm = ("0" + strrpm);
          tft.drawString(strrpm, 300/2, 220, 7);}
        else {
          tft.drawString(String(rpm), 300/2, 220, 7);}
        Serial.print("RPM: "); Serial.println(rpm);
        calculateDelay();
        obdGet = SPEED;}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        tft.drawString("0000", 300/2, 220, 7);
        myELM327.printError();
        calculateDelay();
        obdGet = SPEED;}
      break;
    }

    case SPEED:{
      if (!lock) {timePrevious = millis(); lock = true;}
      tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
      uint16_t tempKPH = myELM327.kph();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        kph = (uint16_t)tempKPH;
        mph = round(kph * 0.621371);
        calculateAverage();
        if (mph <= 9) {
          String strmph = String(mph);
          strmph = ("0" + strmph);
          tft.drawString(strmph, 330/2, 55, 7);}
        else {
          tft.drawString(String(mph), 330/2, 55, 7);}
        Serial.print("MPH: "); Serial.println(mph);
        calculateDelay();
        obdGet = MPG;}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        tft.drawString("00", 330/2, 55, 7);
        myELM327.printError();
        calculateDelay();
        obdGet = MPG;}
      break;
    }

    case MPG:{
      if (!lock) {timePrevious = millis(); lock = true;}
      tft.setTextColor(TFT_LIGHTBLUE, TFT_BLACK);
      float_t tempMAF = myELM327.mafRate();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        maf = (float_t)tempMAF;
        mpg = round((14.7 * 6.17 * 4.54 * kph * 0.621371) / (3600 * maf / 100));
        if (mpg > 90) {mpg = 90;}
        calculateAverage();
        if (mpg <= 9) {
          String strmpg = String(mpg);
          strmpg = ("0" + strmpg);
          tft.drawString(strmpg, 60/2, 70, 8);}
        else {
          tft.drawString(String(mpg), 60/2, 70, 8);}
        Serial.print("KPH: "); Serial.println(kph);
        Serial.print("MAF: "); Serial.println(maf);
        Serial.print("MPG: "); Serial.println(mpg);
        calculateDelay();
        if (kph == 0) {
          obdGet = FUEL;}
        else {
          obdGet = RPM;}}
      else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        tft.drawString("00", 60/2, 70, 8);
        myELM327.printError();
        calculateDelay();
        obdGet = RPM;}
      break;
    }

    case FUEL:{
      float_t tempFuel = myELM327.fuelLevel();
      if (myELM327.nb_rx_state == ELM_SUCCESS) {
        fuel = (uint16_t)tempFuel;
        //tft.fillCircle(215, 288, 6, TFT_LIGHTBLUE);
        Serial.print("FUEL LEVEL: %"); Serial.println(fuel);
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
  tft.drawString(timeWhileOn, 5, 265, 7);
}

void calculateDelay() {
  lock = false;
  timeDelay = String(millis() - timePrevious);
  Serial.println("Delay: " + timeDelay);
  
  if (timeDelay.toInt() <= 9) {
    timeDelay = ("00" + timeDelay);}
  else if (timeDelay.toInt() <= 99) {
    timeDelay = ("0" + timeDelay);}
  else if (timeDelay.toInt() > 999) {
    timeDelay = "999";
  }

  switch (obdGet) {
    case RPM:{
      tft.setTextColor(TFT_DARKRED, TFT_BLACK);
      tft.drawString(timeDelay, 257/2, 181, 2);
      Serial.println("Calculated delay: " + timeDelay);
      Serial.println("---------------------------\n\n");
      break;}
    case SPEED:{
      tft.setTextColor(TFT_DARKRED, TFT_BLACK);
      tft.drawString(timeDelay, 257/2, 18, 2);
      Serial.println("Calculated delay: " + timeDelay);
      Serial.println("---------------------------\n\n");
      break;}
    case MPG:{
      tft.setTextColor(TFT_DARKRED, TFT_BLACK);
      tft.drawString(timeDelay, 7/2, 18, 2);
      Serial.println("Calculated delay: " + timeDelay);
      Serial.println("---------------------------\n\n");
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
      tft.drawString(String(averageRPM), 435/2, 185, 1);
      if (rpm > maxRPM) {
        maxRPM = rpm;
        tft.drawString(String(maxRPM), 435/2, 165, 1);}
      break;}
    case SPEED:{
      if (mph != 0) {
        totalMPH += mph;
        indexMPH++;
        averageMPH = totalMPH / indexMPH;
        tft.drawString(String(averageMPH), 450/2, 25, 1);
        if (mph > maxMPH) {
          maxMPH = mph;
          tft.drawString(String(maxMPH), 450/2, 5, 1);}}
      break;}
    case MPG:{
      if (mph != 0) {
        totalMPG += mpg;
        indexMPG++;
        averageMPG = totalMPG / indexMPG;
        tft.drawString(String(averageMPG), 220/2, 5, 1);}
      break;}
    case FUEL:{
      break;}
  }
}