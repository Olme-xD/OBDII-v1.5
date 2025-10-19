/*
ESP32_TFT_ODB2_v2.ino - CAR USE VERSION
By Olme Matias
Last updated 10/19/2025

- This version is intended for use with a real ELM327 adapter.
- All simulation modes, dummy classes, and conditional compilation blocks have been removed.
*/

#include <SPI.h>
#include <TFT_eSPI.h>
#include "Free_Fonts.h"
#include "ES.h" // EwoShy (Must be in the same sketch folder)
#include "ELMduino.h"
#include "BluetoothSerial.h"


//‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗S DISPLAY SETTINGS, AVERAGE, & BLUETOOTH/ELM
TFT_eSPI tft = TFT_eSPI();
BluetoothSerial SerialBT;
ELM327 myELM327;
#define ELM_PORT SerialBT

#define DEBUG_PORT Serial
#define TFT_GRAY 0xAD55
#define TFT_DARKRED 0xB000

//‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗‗ DEFINED VALUES
typedef enum { RPM,
               MPH,
               ALV,
               MPG,
               FUEL } obdState;
obdState obdGet = RPM;
uint16_t loopCounter = 0; // Used in Mode 1

// --- OBD Data Globals ---
uint16_t rpm;
uint16_t mph;
uint16_t kph;
float_t maf;
uint16_t mpg;
uint16_t fuel;
uint16_t load;
uint16_t maxRPM = 0;
uint16_t maxMPH = 0;
uint16_t maxLoad = 0;
uint16_t averageRPM;
uint64_t indexRPM;
uint64_t totalRPM;
uint16_t averageMPH;
uint64_t indexMPH;
uint64_t totalMPH;
uint16_t averageMPG;
uint64_t indexMPG;
uint64_t totalMPG;
uint16_t averageMPG2;
uint64_t indexMPG2;
uint64_t totalMPG2;
uint16_t averageFuel;
uint16_t indexFuel;
uint16_t totalFuel;
uint16_t averageLoad;
uint64_t indexLoad;
uint64_t totalLoad;

// --- Time Globals ---
String timeWhileOn;
uint32_t seconds;
uint32_t minutes;
uint32_t hours;
String timeStoped = "00:00";
uint64_t timeStopedPrevious = 0;

// --- Misc Globals ---
uint64_t timePrevious;
bool lock = false;
String timeDelay;

// --- Hardware/Mode Globals ---
const int modeButtonPin = 34;
const int ldrPin = 35;
const int brightnessControlPin = 22;
int displayMode = 1;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 250;

// --- Layout Globals ---
const int barWidth = 40;

uint8_t address[6] = { 0xaa, 0xbb, 0xcc, 0x11, 0x22, 0x33 };

// --- Function Prototypes ---
void connect();
void mode1();
void mode2();
void mode3();
void mode4();
void checkModeButton();
void checkDimming();
void timer();
void stopedTimer(bool start);
void calculateDelay();
void calculateAverage();


void setup() {
  tft.init();
  tft.setRotation(1);
  Serial.begin(115200);

  ELM_PORT.begin("404 Unknown", true);

  pinMode(modeButtonPin, INPUT_PULLUP);
  pinMode(ldrPin, INPUT);
  pinMode(brightnessControlPin, OUTPUT);
  digitalWrite(brightnessControlPin, LOW);

  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.pushImage(160, 85, 150, 150, es);
  tft.setSwapBytes(false);

  connect();
  delay(1000);

  tft.fillScreen(TFT_BLACK);

  if (displayMode == 1) {
    tft.drawFastHLine(250, 170, 320, TFT_GRAY);
    tft.drawFastVLine(250, 0, 170, TFT_GRAY);
    tft.drawFastVLine(350, 0, 170, TFT_GRAY);
  }
}

void connect() {
  if (!ELM_PORT.connect(address)) {
    Serial.println("Connection Error 1");
    delay(2000);
    ESP.restart();
  }
  if (!myELM327.begin(ELM_PORT, true, 2000)) {
    Serial.println("Connection Error 2");
    delay(2000);
    ESP.restart();
  }
  Serial.println("CONNECTED TO ELM327!");
}

// ----------------------------------------------------------------
// -------- FUNCTIONS CALLED BY LOOP (DEFINED BEFORE LOOP) --------
// ----------------------------------------------------------------

void checkModeButton() {
  /*
    Checks if the mode button is pressed and cycles the displayMode.
    Includes a 250ms debounce and resets the state machine.
  */
  if (millis() - lastButtonPress > debounceDelay) {
    if (digitalRead(modeButtonPin) == LOW) {
      lastButtonPress = millis();
      displayMode++;
      if (displayMode > 4) displayMode = 1;

      tft.fillScreen(TFT_BLACK);

      tft.setTextColor(TFT_GRAY, TFT_BLACK);
      if (displayMode == 1) {
        tft.drawFastHLine(250, 170, 320, TFT_GRAY);
        tft.drawFastVLine(250, 0, 170, TFT_GRAY);
        tft.drawFastVLine(350, 0, 170, TFT_GRAY);
      }

      obdGet = RPM;
      if (displayMode == 4) obdGet = ALV;
      lock = false;
      loopCounter = 0;

      Serial.println("Mode set to: " + String(displayMode));
    }
  }
}

void checkDimming() {
  /*
    Reads the LDR and sets the display brightness HIGH or LOW
    by controlling the transistor switch in the VCC path.
  */
  int lightLevel = analogRead(ldrPin);
  int threshold = 2000;

  if (lightLevel < threshold) {
    digitalWrite(brightnessControlPin, HIGH);
  } else {
    digitalWrite(brightnessControlPin, LOW);
  }
}

// ----------------------------------------------------------------
// ---------------------- MAIN LOOP & MODES -----------------------
// ----------------------------------------------------------------

void loop() {
  timer();
  checkModeButton();
  checkDimming();

  switch (displayMode) {
    case 1: mode1(); break;
    case 2: mode2(); break;
    case 3: mode3(); break;
    case 4: mode4(); break;
  }
}

// ==================== MODE FUNCTIONS ====================
// ============ (Contain Fetching & Drawing Logic) ========

void mode1() {
  switch (obdGet) {
    case RPM:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        float_t tempRPM = myELM327.rpm();
        if (myELM327.nb_rx_state == ELM_SUCCESS) rpm = (uint16_t)tempRPM;

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          calculateAverage();
          calculateDelay();
          if (timeDelay.toInt() <= 200) tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
          else tft.setTextColor(TFT_DARKRED, TFT_BLACK);
          tft.setTextDatum(TL_DATUM);
          tft.drawString(timeDelay, 400, 5, 2);
          if (rpm > 3000) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
          } else if (rpm > 2500) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
          } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
          }
          tft.setTextDatum(MC_DATUM);
          tft.drawString(String(rpm), 420, 80, 6);
          tft.setTextColor(TFT_GRAY, TFT_BLACK);
          tft.setFreeFont(FF41);
          tft.setTextDatum(TC_DATUM);
          tft.drawString("RPM", 375, 5);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.drawString(String(maxRPM), 420, 106, 4);
          tft.drawString(String(averageRPM), 420, 130, 4);
        } else {
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("----", 420, 80, 6);
          myELM327.printError();
        }
        if (myELM327.nb_rx_state != ELM_GETTING_MSG) obdGet = ALV;
        break;
      }

    case MPH:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        uint16_t tempKPH = myELM327.kph();
        if (myELM327.nb_rx_state == ELM_SUCCESS) kph = tempKPH;

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          if (kph > 200) {
            Serial.println("!! BAD KPH DATA");
            calculateDelay();
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("--", 300, 80, 6);
          } else {
            mph = round(kph * 0.621371);
            (mph <= 5) ? stopedTimer(true) : stopedTimer(false);
            calculateAverage();
            calculateDelay();
            if (timeDelay.toInt() <= 200) tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
            else tft.setTextColor(TFT_DARKRED, TFT_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.drawString(timeDelay, 305, 5, 2);
            if (mph > 78) {
              tft.setTextColor(TFT_RED, TFT_BLACK);
            } else if (mph >= 74) {
              tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            } else {
              tft.setTextColor(TFT_WHITE, TFT_BLACK);
            }
            tft.setTextDatum(MC_DATUM);
            tft.drawString((mph < 10 ? "0" : "") + String(mph), 300, 80, 6);
            tft.setTextColor(TFT_GRAY, TFT_BLACK);
            tft.setFreeFont(FF41);
            tft.setTextDatum(TC_DATUM);
            tft.drawString("MPH", 280, 5);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(String(maxMPH), 300, 106, 4);
            tft.drawString(String(averageMPH), 300, 130, 4);
          }
        } else {
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("--", 300, 80, 6);
          myELM327.printError();
        }
        if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          obdGet = MPG;
        }
        break;
      }

    case ALV:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        float_t tempLoad = myELM327.engineLoad();
        if (myELM327.nb_rx_state == ELM_SUCCESS) load = (uint16_t)tempLoad;

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          calculateAverage();
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("ALV:" + String(load), 40, 275, 4);
        } else {
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("LOAD:--", 50, 275, 4);
          myELM327.printError();
        }
        if (myELM327.nb_rx_state != ELM_GETTING_MSG) obdGet = FUEL;
        break;
      }

    case FUEL:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        float_t tempFuel = myELM327.fuelLevel();
        if (myELM327.nb_rx_state == ELM_SUCCESS) fuel = (uint16_t)tempFuel;

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          calculateAverage();
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("FUEL:" + String(fuel), 50, 305, 4);
        } else {
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("FUEL:--", 50, 305, 4);
          myELM327.printError();
        }
        if (myELM327.nb_rx_state != ELM_GETTING_MSG) obdGet = MPH;
        break;
      }

    case MPG:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        float tempMAF = myELM327.mafRate();
        if (myELM327.nb_rx_state == ELM_SUCCESS && tempMAF > 0) {
          maf = tempMAF;
          mpg = round((14.7 * 6.17 * 4.54 * kph * 0.621371) / (3600 * maf / 100));
        } else {
          myELM327.nb_rx_state = 0;
        }

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          if (mpg > 99) { mpg = 99; }
          calculateAverage();
          calculateDelay();
          if (mpg < 20) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
          } else if (mpg <= 30) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
          } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
          }
          tft.setTextDatum(MC_DATUM);
          tft.drawString((mpg < 10 ? "0" : "") + String(mpg), 120, 130, 8);
          tft.setTextColor(TFT_GRAY, TFT_BLACK);
          tft.setFreeFont(FF41);
          tft.setTextDatum(TC_DATUM);
          tft.drawString("MPG", 21, 5);
          if (timeDelay.toInt() <= 200) tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
          else tft.setTextColor(TFT_DARKRED, TFT_BLACK);
          tft.drawString(timeDelay, 60, 5, 2);

          if (mph > 5) {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextDatum(BC_DATUM);
            if (minutes < 15) {
              tft.drawString(String(averageMPG2), 190, 220, 6);  // Show 34-cap
            } else {
              tft.drawString(String(averageMPG), 190, 220, 6);  // Show 37-cap
            }
          }
        } else {
          calculateDelay();
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("--", 120, 130, 8);
          myELM327.printError();
        }
        if (myELM327.nb_rx_state != ELM_GETTING_MSG) obdGet = RPM;
        break;
      }
    default: obdGet = RPM; break;
  }
}

void mode2() {
  // --- Draw static layout & placeholders ONCE when mode is selected ---
  if (loopCounter == 0) {
    tft.setTextColor(TFT_GRAY, TFT_BLACK);
    tft.setFreeFont(FF41);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("RPM", 375, 5);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("----", 420, 80, 6);
    tft.drawString("---", 420, 106, 4);
    tft.drawString("---", 420, 130, 4);
    tft.drawString("ALV:--", 40, 275, 4);
    tft.drawString("FUEL:--", 50, 305, 4);
    tft.setTextColor(TFT_GRAY, TFT_BLACK);
    tft.drawFastHLine(250, 170, 320, TFT_GRAY);
    tft.drawFastVLine(250, 0, 170, TFT_GRAY);
    tft.drawFastVLine(350, 0, 170, TFT_GRAY);
  }

  // --- Handle dynamic data (MPH & MPG) ---
  switch (obdGet) {
    case MPH:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        uint16_t tempKPH = myELM327.kph();
        if (myELM327.nb_rx_state == ELM_SUCCESS) kph = tempKPH;

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          if (kph > 200) {
            Serial.println("!! BAD KPH DATA");
            mph = 0;
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("--", 300, 80, 6);
          } else {
            mph = round(kph * 0.621371);
            (mph <= 5) ? stopedTimer(true) : stopedTimer(false);
            calculateAverage();
            calculateDelay();

            if (timeDelay.toInt() <= 200) tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
            else tft.setTextColor(TFT_DARKRED, TFT_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.drawString(timeDelay, 305, 5, 2);

            if (mph > 78) {
              tft.setTextColor(TFT_RED, TFT_BLACK);
            } else if (mph >= 74) {
              tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            } else {
              tft.setTextColor(TFT_WHITE, TFT_BLACK);
            }
            tft.setTextDatum(MC_DATUM);
            tft.drawString((mph < 10 ? "0" : "") + String(mph), 300, 80, 6);

            tft.setTextColor(TFT_GRAY, TFT_BLACK);
            tft.setFreeFont(FF41);
            tft.setTextDatum(TC_DATUM);
            tft.drawString("MPH", 280, 5);

            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawString(String(maxMPH), 300, 106, 4);
            tft.drawString(String(averageMPH), 300, 130, 4);
          }
        } else {
          mph = 0;
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("--", 300, 80, 6);
          myELM327.printError();
        }


        if (myELM327.nb_rx_state != ELM_GETTING_MSG) obdGet = MPG;
        break;
      }

    case MPG:
      {
        if (!lock) {
          timePrevious = millis();
          lock = true;
        }
        float tempMAF = myELM327.mafRate();
        if (myELM327.nb_rx_state == ELM_SUCCESS && tempMAF > 0) {
          maf = tempMAF;
          mpg = round((14.7 * 6.17 * 4.54 * kph * 0.621371) / (3600 * maf / 100));
        } else {
          myELM327.nb_rx_state = 0;
        }

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          if (mpg > 99) { mpg = 99; }
          calculateAverage();
          calculateDelay();


          if (mpg < 20) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
          } else if (mpg <= 30) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
          } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
          }
          tft.setTextDatum(MC_DATUM);
          tft.drawString((mpg < 10 ? "0" : "") + String(mpg), 120, 130, 8);

          tft.setTextColor(TFT_GRAY, TFT_BLACK);
          tft.setFreeFont(FF41);
          tft.setTextDatum(TC_DATUM);
          tft.drawString("MPG", 21, 5);

          if (timeDelay.toInt() <= 200) tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
          else tft.setTextColor(TFT_DARKRED, TFT_BLACK);
          tft.drawString(timeDelay, 60, 5, 2);

          if (mph > 5) {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextDatum(BC_DATUM);

            if (minutes < 15) {
              tft.drawString(String(averageMPG2), 190, 220, 6);  // Show 34-cap
            } else {
              tft.drawString(String(averageMPG), 190, 220, 6);  // Show 37-cap
            }
          }
        } else {
          mpg = 0;
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("--", 120, 130, 8);
          myELM327.printError();
        }

        if (myELM327.nb_rx_state != ELM_GETTING_MSG) obdGet = MPH;
        break;
      }

    case RPM:
    case ALV:
    case FUEL:
    default:
      obdGet = MPH;
      lock = false;
      break;
  }

  loopCounter++;
}

void mode3() {
}

void mode4() {
}


// ----------------------------------------------------------------
// -------------------- HELPER FUNCTIONS --------------------------
// ----------------------------------------------------------------

void timer() {
  seconds = millis() / 1000UL;
  hours = seconds / 3600;
  seconds %= 3600;
  minutes = seconds / 60;
  seconds %= 60;
  if (seconds <= 9 && minutes <= 9) {
    timeWhileOn = String(hours) + ":0" + String(minutes) + ":0" + String(seconds);
  } else if (seconds <= 9) {
    timeWhileOn = String(hours) + ":" + String(minutes) + ":0" + String(seconds);
  } else if (minutes <= 9) {
    timeWhileOn = String(hours) + ":0" + String(minutes) + ":" + String(seconds);
  } else {
    timeWhileOn = String(hours) + ":" + String(minutes) + ":" + String(seconds);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(BL_DATUM);
  tft.drawString(timeWhileOn, 310, 320, 6);
  tft.drawString(timeStoped, 410, 270, 4);
}

void stopedTimer(bool start) {
  uint64_t currentSeconds = millis() / 1000UL;
  if (start) {
    uint32_t elapsedTime = currentSeconds - timeStopedPrevious;
    uint32_t displayMinutes = elapsedTime / 60;
    uint32_t displaySeconds = elapsedTime % 60;
    timeStoped = "";
    if (displayMinutes < 10) timeStoped += "0";
    timeStoped += displayMinutes;
    timeStoped += ":";
    if (displaySeconds < 10) timeStoped += "0";
    timeStoped += displaySeconds;
  } else {
    timeStopedPrevious = currentSeconds;
  }
}

void calculateDelay() {
  lock = false;
  timeDelay = String(millis() - timePrevious);
  Serial.println("Delay: " + timeDelay);
  if (timeDelay.toInt() <= 9) {
    timeDelay = ("00" + timeDelay);
  } else if (timeDelay.toInt() <= 99) {
    timeDelay = ("0" + timeDelay);
  } else if (timeDelay.toInt() > 999) {
    timeDelay = "999";
  }
}

void calculateAverage() {
  switch (obdGet) {
    case RPM:
      totalRPM += rpm;
      indexRPM++;
      averageRPM = totalRPM / indexRPM;
      if (rpm > maxRPM) maxRPM = rpm;
      break;
    case MPH:
      if (mph != 0 && kph <= 300) {
        totalMPH += mph;
        indexMPH++;
        averageMPH = totalMPH / indexMPH;
        if (mph > maxMPH) maxMPH = mph;
      }
      break;
    case ALV:
      if (load > 0) {
        totalLoad += load;
        indexLoad++;
        averageLoad = totalLoad / indexLoad;
        if (load > maxLoad) maxLoad = load;
      }
      break;
    case MPG:
      {
        if (mph > 5) {
          // --- Calculate 34-cap average (averageMPG2) ---
          uint16_t cappedMPG_34 = (mpg > 34) ? 34 : mpg;
          totalMPG2 += cappedMPG_34;
          indexMPG2++;
          averageMPG2 = totalMPG2 / indexMPG2;

          // --- Calculate 37-cap average (averageMPG) ---
          uint16_t cappedMPG_37 = (mpg > 37) ? 37 : mpg;
          totalMPG += cappedMPG_37;
          indexMPG++;
          averageMPG = totalMPG / indexMPG;
        }
        break;
      }
    case FUEL:
      totalFuel += fuel;
      indexFuel++;
      averageFuel = totalFuel / indexFuel;
      if (indexFuel >= 5) {
        totalFuel = 0;
        indexFuel = 0;
      }
      break;
  }
}
