#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

// --- PİNLER ---
#define SPEAKER_PIN 1
#define EPD_BUSY    2
#define EPD_RST     3
#define EPD_DC      4
#define EPD_CS      5
#define EPD_CLK     6
#define EPD_DIN     7
#define ROT_A       8
#define ROT_B       9
#define ROT_SW      10

// --- EKRAN ---
GxEPD2_BW<GxEPD2_290_T94, GxEPD2_290_T94::HEIGHT> display(GxEPD2_290_T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// --- GLOBAL DEĞİŞKENLER ---
Preferences prefs;
String ssid, pass, token, city;
String havaDurumu = "-- C";
enum Menu { MENU_SAAT, MENU_HAVA, MENU_ZAMANLAYICI, MENU_COUNT };
Menu currentMenu = MENU_SAAT;
bool inDetail = false;
int lastA = HIGH;
unsigned long pressStart = 0;
bool pressing = false;
unsigned long sonHavaGuncelleme = 0;

// --- SES FONKSİYONU ---
void bip(int frekans, int sure) {
  tone(SPEAKER_PIN, frekans, sure);
}

// --- YARDIMCI ÇİZİM ---
void printCentered(const char* text, int y, int textSize) {
  display.setTextSize(textSize);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((display.width() - w) / 2, y);
  display.print(text);
}

// --- HAVA DURUMU ÇEKME ---
void havaDurumuGetir() {
  if (WiFi.status() == WL_CONNECTED && token.length() > 5) {
    HTTPClient http;
    // URL'yi city ve token değişkenlerine göre oluşturuyoruz
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + token + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, payload);
      float temp = doc["main"]["temp"];
      havaDurumu = String((int)round(temp)) + " C";
    }
    http.end();
  }
}

void ayarlariYukle() {
  prefs.begin("bilgi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  token = prefs.getString("token", "");
  city = prefs.getString("city", "Istanbul,TR");
  prefs.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(ROT_A, INPUT_PULLUP);
  pinMode(ROT_B, INPUT_PULLUP);
  pinMode(ROT_SW, INPUT_PULLUP);

  bip(2000, 100);

  SPI.begin(EPD_CLK, -1, EPD_DIN, EPD_CS);
  display.init(115200);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);

  ayarlariYukle();

  if (ssid != "" && ssid != "null") {
    WiFi.begin(ssid.c_str(), pass.c_str());
    configTime(10800, 0, "pool.ntp.org"); // Türkiye GMT+3
  }

  drawMenu();
}

void loop() {
  // Web Serial Dinleme
  if (Serial.available() > 0) {
    String gelenVeri = Serial.readStringUntil('\n');
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, gelenVeri) == DeserializationError::Ok) {
      prefs.begin("bilgi", false);
      prefs.putString("ssid", doc["s"].as<String>());
      prefs.putString("pass", doc["p"].as<String>());
      prefs.putString("token", doc["t"].as<String>());
      prefs.putString("city", doc["l"].as<String>());
      prefs.end();
      bip(2500, 200);
      delay(500);
      ESP.restart();
    }
  }

  // Hava durumu güncelleme (30 dakikada bir)
  if (millis() - sonHavaGuncelleme > 1800000 || sonHavaGuncelleme == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      havaDurumuGetir();
      sonHavaGuncelleme = millis();
    }
  }

  handleRotary();
  handleButton();
}

void handleRotary() {
  int a = digitalRead(ROT_A);
  if (!inDetail && a != lastA) {
    if (digitalRead(ROT_B) != a) currentMenu = (Menu)((currentMenu + 1) % MENU_COUNT);
    else currentMenu = (Menu)((currentMenu + MENU_COUNT - 1) % MENU_COUNT);
    
    bip(1500, 10);
    drawMenu();
  }
  lastA = a;
}

void handleButton() {
  int sw = digitalRead(ROT_SW);
  if (sw == LOW && !pressing) { pressing = true; pressStart = millis(); }
  if (sw == HIGH && pressing) {
    unsigned long dur = millis() - pressStart;
    pressing = false;
    if (dur < 800) { 
      bip(2000, 50);
      inDetail = true; 
      drawDetail(); 
    }
    else { 
      bip(1000, 150);
      inDetail = false; 
      drawMenu(); 
    }
  }
}

void drawMenu() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    printCentered("BILGIEKRANI", 15, 1);
    display.drawFastHLine(0, 25, display.width(), GxEPD_BLACK);
    const char* label = "";
    if (currentMenu == MENU_SAAT) label = "> SAAT <";
    if (currentMenu == MENU_HAVA) label = "> HAVA DURUMU <";
    if (currentMenu == MENU_ZAMANLAYICI) label = "> ZAMANLAYICI <";
    printCentered(label, (display.height()/2), 2);
  } while (display.nextPage());
}

void drawDetail() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    if (currentMenu == MENU_SAAT) {
      struct tm timeinfo;
      if(getLocalTime(&timeinfo)) {
        char buffer[10]; strftime(buffer, 10, "%H:%M", &timeinfo);
        printCentered("SAAT", 20, 2);
        printCentered(buffer, 70, 5);
      } else { printCentered("WiFi Bekleniyor", 60, 2); }
    }
    else if (currentMenu == MENU_HAVA) {
      printCentered(city.c_str(), 20, 2);
      printCentered(havaDurumu.c_str(), 70, 5);
    }
    else if (currentMenu == MENU_ZAMANLAYICI) {
      printCentered("ZAMANLAYICI", 20, 2);
      printCentered("YAKINDA", 70, 3);
    }
  } while (display.nextPage());
}