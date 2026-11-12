// TEST OK sur carte LOLIN S3 MIni, le 22/12/2024
// outils > partition > HUGE APP et SPRAM > QSPI
// Source :  https://www.xtronical.com/i2sinternetradio/
// IDE Arduino 2.3.4
// Core  ESP32 d'Espressif en version 3.1.0  
// Bibliothèque : https://github.com/schreibfaul1/ESP32-audioI2S  ( en VERSION  3.0.12)
// Selectionner pour Partition Scheme  une option laissant au moins 2MB pour le code

#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// PM5102 pins
#define I2S_LRC  11
#define I2S_DOUT 9
#define I2S_BCLK 7

// Rotary Encoder Pins
#define ENCODER_CLK 5
#define ENCODER_DT  2
#define ENCODER_SW  3

int currentVolume = 15;
int lastCLK = HIGH;

Audio audio;


void setup() {

  pinMode(SCK, OUTPUT); // board prise jack" "PCM5100" GND
  digitalWrite(SCK, LOW);
  delay(100);
  
  Serial.begin(115200);
  connectWifi();
  
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(15); // 0...21

  audio.connecttohost("https://icecast.radiofrance.fr/monpetitfranceinter-midfi.aac");

  Serial.println("\n[1/4] Setting up Rotary Encoder...");
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  lastCLK = digitalRead(ENCODER_CLK);
  Serial.println("Rotary Encoder ready");
  Serial.printf("   Pins: CLK=%d, DT=%d, SW=%d\n", ENCODER_CLK, ENCODER_DT, ENCODER_SW);
}

void loop()
{
  audio.loop();

  readRotaryEncoder();
   
  // Read button every 10ms
  static unsigned long lastButtonRead = 0;
  
  if (millis() - lastButtonRead > 10) {
    readButton();
    lastButtonRead = millis();
  }
}

void readRotaryEncoder() {
  static unsigned long lastRotationTime = 0;
  
  // 10ms debounce - fast enough for any human turn speed,
  // long enough for KY-040 electrical bounce to settle (typically 1-5ms)
  if (millis() - lastRotationTime < 10) {
    return;
  }
  
  // Read CLK and DT at the exact same instant from GPIO register
  uint32_t gpioIn = REG_READ(GPIO_IN_REG);
  int clkState = (gpioIn >> ENCODER_CLK) & 1;
  int dtState  = (gpioIn >> ENCODER_DT) & 1;
  
  if (clkState != lastCLK) {
    if (clkState == LOW) {
      // Make sure button is NOT pressed during rotation
      if (digitalRead(ENCODER_SW) == HIGH) {
        // if (dtState == clkState) 
        if (dtState != clkState) {
          // Counter-clockwise - Volume DOWN
          if (currentVolume > 0) {
            currentVolume--;
            audio.setVolume(currentVolume);
            Serial.printf("Volume DOWN: %d/21\n", currentVolume);
            lastRotationTime = millis();
          }
        } else {
          // Clockwise - Volume UP
          if (currentVolume < 21) {
            currentVolume++;
            audio.setVolume(currentVolume);
            Serial.printf("Volume UP: %d/21\n", currentVolume);
            lastRotationTime = millis();
          }
        }
      }
    }
    lastCLK = clkState;
  }
}


void readButton() {
  static int stableButtonState = HIGH;
  static unsigned long buttonDebounceTime = 0;
  const unsigned long buttonDebounceDelay = 250;
  
  int currentButtonState = digitalRead(ENCODER_SW);
  
  if (currentButtonState == LOW && stableButtonState == HIGH) {
    if (millis() - buttonDebounceTime > buttonDebounceDelay) {
      Serial.println("Button pressed");
      buttonDebounceTime = millis();
      stableButtonState = LOW;
      audio.pauseResume();
    }
  } else if (currentButtonState == HIGH && stableButtonState == LOW) {
      stableButtonState = HIGH;
      buttonDebounceTime = millis();
  }
}

// optional
void audio_info(const char *info) {
  Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info) { //id3 metadata
  Serial.print("id3data     "); Serial.println(info);
}
void audio_eof_mp3(const char *info) { //end of file
  Serial.print("eof_mp3     "); Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     "); Serial.println(info);
}
void audio_showstreaminfo(const char *info) {
  Serial.print("streaminfo  "); Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle "); Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     "); Serial.println(info);
}
void audio_commercial(const char *info) { //duration in sec
  Serial.print("commercial  "); Serial.println(info);
}
void audio_icyurl(const char *info) { //homepage
  Serial.print("icyurl      "); Serial.println(info);
}
void audio_lasthost(const char *info) { //stream URL played
  Serial.print("lasthost    "); Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  "); Serial.println(info);
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void connectWifi() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  wm.setAPCallback(configModeCallback);
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");

          delay(1000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();

  }
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

} 
