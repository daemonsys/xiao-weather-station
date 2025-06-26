#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTelegram2.h>
#include <time.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_BMP280 bmp;


// WiFi
const char* ssid = "//";
const char* password = "//";


// BOT
#define BOT_TOKEN "//"
const int64_t CHAT_ID = //;
WiFiClientSecure secured_client;
AsyncTelegram2 bot(secured_client);


// GREEN LED
#define GREEN_LED 21
#define BREATH_DELAY 6  // more = slower
int breathValue = 0;
int breathDirection = 1;
unsigned long lastBreathUpdate = 0;


// RED LED
#define RED_LED 4
#define RED_BLINK_INTERVAL 1500
unsigned long lastRedBlink = 0;
int redBlinkCount = 0;
bool redBlinkOn = false;
unsigned long lastRedBlinkStep = 0;
bool redBlinkState = false;


// DISPLAY
unsigned long lastDisplayUpdate = 0;
unsigned long displayInterval = 1000;


// NTP
time_t now;
tm timeinfo;
#define GMT_OFFSET_SEC 5 * 3600
#define NTP_SERVER "pool.ntp.org"

unsigned long uptimeStart = 0;


bool alertMode = false;
String alertText = "";
bool showAlertTitle = true;
unsigned long alertPhaseStart = 0;
const unsigned long ALERT_DURATION = 2000;
const unsigned long SCROLL_DURATION = 5000;



void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("SETTING UP THE INTERNET CONNECTION");
  int retries = 0;
  display.setRotation(0);

  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONNECTING TO WIFI");
    display.println(String(retries) + "/20");
    display.display();
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("CONNECTED!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CONNECTED");
    display.display();
    delay(500);
  } else {
    Serial.println("\nUNABLE TO CONNECT. REBOOT THE MACHINE!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("UNABLE TO CONNECT.\nREBOOT THE MACHINE!");
    display.display();
    delay(500);
    while(true);
  }
  display.setRotation(1);
}


void setup() {
  btStop();
  setCpuFrequencyMhz(80);
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  // EEPROM.writeString(0, "STATION 10");
  // EEPROM.commit();

  // DISPLAY
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 NOT FOUND!\nTURNIN' OFF THE SWITCH, BYE BYE"));
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setRotation(1);

  // BME280
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 NOT FOUND!\nTURNIN' OFF THE SWITCH, BYE BYE");
    while (true);
  }
  
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  connectToWiFi();
  configTime(GMT_OFFSET_SEC, 0, NTP_SERVER);

  secured_client.setInsecure();
  bot.setUpdateTime(1000);
  bot.setTelegramToken(BOT_TOKEN);
  bot.begin();
  TBMessage msg;
  while (bot.getNewMessage(msg)) {;}
  bot.setFormattingStyle(AsyncTelegram2::FormatStyle::HTML);

  String message = "📡   " + EEPROM.readString(0) + " IS ON\n";
  message += "SOME FEATURES ARE UNAVALABLE ON THIS STATION\n\n";
  message += "TYPE /HELP TO SEE LIST OF COMMANDS\n";
  // message += "TYPE /GET TO GET ACTUAL REPORT\n";
  // message += "TYPE /UPTIME TO GET THE CURRENT UPTIME\n";
  // message += "TYPE /SUMMARY TO GET FULL STATUS REPORT\n\n";
  // //message += "TYPE /REBOOT TO RESTART THE STATION\n";
  // message += "USE /ALERT <b>YOUR_TEXT_HERE</b> TO ENABLE ALERT MODE\n";
  // message += "AND /ALERTOFF TO DISABLE IT\n";
  bot.sendTo(CHAT_ID, message);
  uptimeStart = millis();
}


void updateDisplay() {

  if (alertMode) {
    display.clearDisplay();
    display.setRotation(0);
    unsigned long now = millis();

    if (showAlertTitle && now - alertPhaseStart < ALERT_DURATION) {
      display.setTextSize(3);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.print("ALERT");
    } else {
      showAlertTitle = false;

      if (now - alertPhaseStart > ALERT_DURATION + SCROLL_DURATION) {
        showAlertTitle = true;
        alertPhaseStart = now;
      }

      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print(alertText);
    }

    display.display();
    return;
  }

  float temp = bmp.readTemperature();
  //float hum = bme.readHumidity();
  float pres = bmp.readPressure() / 100.0;
  
  time(&now);
  localtime_r(&now, &timeinfo);

  int hour = timeinfo.tm_hour;
  String ampm = (hour >= 12) ? "PM" : "AM";
  if (hour == 0) hour = 12;
  else if (hour > 12) hour -= 12;

  char hh[3], mm[3], ss[3];
  snprintf(hh, sizeof(hh), "%02d", hour);
  snprintf(mm, sizeof(mm), "%02d", timeinfo.tm_min);
  snprintf(ss, sizeof(ss), "%02d", timeinfo.tm_sec);

  display.clearDisplay();
  display.setRotation(1);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("/////");
  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);
  display.setCursor(0, 15);
  display.println(ampm);
  display.setCursor(0, 25);
  display.setTextSize(2);
  display.println(hh);
  display.println(mm);
  display.println(ss);
  display.setTextSize(1);
  display.drawLine(0, 76, SCREEN_WIDTH, 76, SSD1306_WHITE);
  display.setCursor(0, 80);
  display.println(String((int)temp) + "C");
  //display.println(String((int)hum) + "%");
  display.println("N/A");
  display.println(String((int)pres) + "h");
  display.display();
}


void handleTelegram() {
  TBMessage msg;
  if (bot.getNewMessage(msg)) {
    if (msg.text == "/HELP") {
      String response = "TYPE /GET TO GET ACTUAL REPORT\n";
      response += "TYPE /UPTIME TO GET THE CURRENT UPTIME\n";
      response += "TYPE /SUMMARY TO GET FULL STATUS REPORT\n\n";
      //message += "TYPE /REBOOT TO RESTART THE STATION\n";
      response += "USE /ALERT <b>YOUR_TEXT_HERE</b> TO ENABLE ALERT MODE\n";
      response += "AND /ALERTOFF TO DISABLE IT\n";
      response += "TYPE /SETNAME <b>YOUR_TEXT_HERE</b> TO CHANGE THE STATION'S NAME\n";
      bot.sendMessage(msg, response);
    }
    else if (msg.text == "/GET") {
      float temp = bmp.readTemperature();
      //float hum = bme.readHumidity();
      float pres = bmp.readPressure() / 100.0;

      time(&now);
      localtime_r(&now, &timeinfo);
      char timeStr[30];
      strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);

      String response = "📡   <b>" + EEPROM.readString(0) + "</b>\n";
      response += "<b>TIME:</b>   " + String(timeStr) + "\n";
      response += "<b>TEMP:</b>   " + String(temp, 1) + " °C\n";
      response += "<b>HUM:</b>   UNAVAILABLE\n";
      response += "<b>PRES:</b>   " + String(pres, 1) + " hPa\n\n";
      bot.sendMessage(msg, response);
    }
    else if(msg.text == "/UPTIME") {
      unsigned long uptimeMillis = millis() - uptimeStart;
      unsigned long seconds = uptimeMillis / 1000;
      unsigned long minutes = seconds / 60;
      unsigned long hours = minutes / 60;
      unsigned long days = hours / 24;

      seconds %= 60;
      minutes %= 60;
      hours %= 24;

      String uptimeMsg = "🕒   <b>" + EEPROM.readString(0) + " UPTIME:</b>\n";
      uptimeMsg += String(days) + "d ";
      uptimeMsg += String(hours) + "h ";
      uptimeMsg += String(minutes) + "m ";
      uptimeMsg += String(seconds) + "s\n";
      bot.sendMessage(msg, uptimeMsg);
    }
    else if(msg.text == "/SUMMARY") {
      float temp = bmp.readTemperature();
      float pres = bmp.readPressure() / 100.0;

      time(&now);
      localtime_r(&now, &timeinfo);
      char timeStr[30];
      strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);

      unsigned long uptimeMillis = millis();
      unsigned long seconds = uptimeMillis / 1000;
      unsigned long minutes = seconds / 60;
      unsigned long hours = minutes / 60;
      unsigned long days = hours / 24;

      seconds %= 60;
      minutes %= 60;
      hours %= 24;

      long rssi = WiFi.RSSI();
      String signalQuality;

      if (rssi >= -67) {
        signalQuality = "Excellent";
      } else if (rssi >= -70) {
        signalQuality = "Good";
      } else if (rssi >= -75) {
        signalQuality = "Fair";
      } else {
        signalQuality = "Weak";
      }

      String response = "📊 <b>" + EEPROM.readString(0) + " SUMMARY</b>\n\n";
      response += "<b>Time:</b>   " + String(timeStr) + "\n";
      response += "<b>Uptime:</b>   " + String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s\n";
      response += "<b>Wi-Fi:</b>   " + WiFi.SSID() + "\n";
      response += "<b>Signal:</b>   " + String(rssi) + " dBm (" + signalQuality + ")\n";
      response += "<b>IP:</b>   " + WiFi.localIP().toString()  + "\n";
      response += "<b>Temp:</b>   " + String(temp, 1) + " °C\n";
      response += "<b>HUM:</b>   UNAVAILABLE\n";
      response += "<b>Pres:</b>   " + String(pres, 1) + " hPa\n";

      bot.sendMessage(msg, response);
    }
    // else if (msg.text == "/REBOOT1") {
    //   bot.sendMessage(msg, "Rebooting station...");
    //   Serial.println(F("REBOOTING STATION\nSEE YA IN A MINUTE"));
    //   delay(1000);
    //   ESP.restart();
    // }
    else if (msg.text.startsWith("/ALERT ")) {
      alertText = msg.text.substring(7);
      alertMode = true;
      showAlertTitle = true;
      alertPhaseStart = millis();
      bot.sendMessage(msg, "ALERT MODE ENABLED");
    }
    else if (msg.text == "/ALERTOFF") {
      alertMode = false;
      bot.sendMessage(msg, "ALERT MODE DISABLED");
    }
    else if (msg.text.startsWith("/SETNAME ")) {
      String newName = msg.text.substring(9);
      for (int i = 0; i < EEPROM_SIZE; i++) {
        EEPROM.write(i, 0);
      }
      EEPROM.writeString(0, newName);
      EEPROM.commit();
      bot.sendMessage(msg, "STATION'S NAME CHANGES");
    }
  }
}




void loop() {
  // CHECK WIFI
  static bool wasConnected = true;
  if (WiFi.status() != WL_CONNECTED && wasConnected) {
    Serial.println("WIFI LOST. TRYING TO RECONNECT...");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setRotation(0);
    display.println("WIFI LOST");
    display.display();
    delay(1000);
    wasConnected = false;
    connectToWiFi();
  }
  else if (WiFi.status() == WL_CONNECTED && !wasConnected) {
    Serial.println("WIFI RESTORED");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WIFI RESTORED");
    display.display();
    delay(1000);
    wasConnected = true;
    display.setRotation(1);
  }


  handleTelegram();

  // DISPLAY UPDATE
  if (millis() - lastDisplayUpdate > displayInterval) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }

  // AUTO ALERT ON HIGH TEMP
  float curTemp = bmp.readTemperature();
  if (curTemp >= 30.0 && !alertMode) {
    alertText = "HIGH TEMP";
    alertMode = true;
    showAlertTitle = true;
    alertPhaseStart = millis();
    Serial.println("ALERT MODE ACTIVATED: HIGH TEMP");

    // if(WiFi.status() == WL_CONNECTED) {
    //   String message = "⚠️  <b>" + EEPROM.readString(0) + "</b>\n";
    //   message += "ALERT: HIGH TEMPERATURE\n";
    //   message += "CURRENT: " + String(curTemp, 1) + " °C";
    //   bot.sendMessage(TBMessage msg, message);
    // }
  }
  // AUTO OFF
  if (curTemp < 30.0 && alertMode && alertText == "HIGH TEMP") {
    alertMode = false;
    Serial.println("ALERT MODE DEACTIVATED: TEMP IS NORMAL");

    // if(WiFi.status() == WL_CONNECTED) {
    //   String message = "<b>" + EEPROM.readString(0) + "</b>\n";
    //   message += "TEMPERATURE NORMALIZED\n";
    //   message += "CURRENT: " + String(curTemp, 1) + " °C";
    //   bot.sendMessage(TBMessage msg, message);
    // }
  }



  // GREEN BREATHING
  if (millis() - lastBreathUpdate >= BREATH_DELAY) {
    lastBreathUpdate = millis();
    breathValue += breathDirection;
    if (breathValue >= 255) {
      breathValue = 255;
      breathDirection = -1;
    } else if (breathValue <= 0) {
      breathValue = 0;
      breathDirection = 1;
    }
    analogWrite(GREEN_LED, breathValue);
  }

  // RED BLINKING
  if (!redBlinkOn && millis() - lastRedBlink >= RED_BLINK_INTERVAL) {
    redBlinkOn = true;
    redBlinkCount = 0;
    lastRedBlinkStep = millis();
    redBlinkState = false;
    lastRedBlink = millis();
  }
  if (redBlinkOn && redBlinkCount < 5) {
    if (millis() - lastRedBlinkStep >= (redBlinkState ? 10 : 50)) {
      lastRedBlinkStep = millis();
      redBlinkState = !redBlinkState;
      digitalWrite(RED_LED, redBlinkState ? HIGH : LOW);

      if (!redBlinkState) {
        redBlinkCount++;
      }
    }
  } else {
    redBlinkOn = false;
    digitalWrite(RED_LED, LOW);
  }
}







