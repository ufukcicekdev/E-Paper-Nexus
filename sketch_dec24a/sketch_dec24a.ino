#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>

// --- PİNLER (ESP32-C3) ---
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

GxEPD2_BW<GxEPD2_290_T94, GxEPD2_290_T94::HEIGHT> display(GxEPD2_290_T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// --- BİTMAP İKONLAR (64x64) ---
const unsigned char icon_clear[] PROGMEM = { 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x42, 0x42, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00 }; // Güneş
const unsigned char icon_cloudy[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Bulut
const unsigned char icon_rain[] PROGMEM = { 0x00, 0x00, 0x1f, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x00, 0x00, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Yağmur
const unsigned char icon_snow[] PROGMEM = { 0x00, 0x00, 0x12, 0x48, 0x24, 0x24, 0x48, 0x12, 0x00, 0x00, 0x12, 0x48, 0x24, 0x24, 0x48, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Kar
const unsigned char icon_storm[] PROGMEM = { 0x00, 0x00, 0x1f, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0x08, 0x20, 0x04, 0x40, 0x02, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // Fırtına

// --- DEĞİŞKENLER ---
Preferences prefs;
String ssid, pass, token, city;
String havaDurumu = "-- C";
int conditionId = 800;
enum Menu { MENU_SAAT, MENU_HAVA, MENU_ZAMANLAYICI, MENU_COUNT };
Menu currentMenu = MENU_SAAT;
bool inDetail = false;
int lastA = HIGH;
unsigned long pressStart = 0;
bool pressing = false;
unsigned long sonHavaGuncelleme = 0;

String temizle(String metin) {
    metin.replace("İ", "I"); metin.replace("ı", "i");
    metin.replace("ş", "s"); metin.replace("Ş", "S");
    metin.replace("ğ", "g"); metin.replace("Ğ", "G");
    metin.replace("ç", "c"); metin.replace("Ç", "C");
    metin.replace("ü", "u"); metin.replace("Ü", "U");
    metin.replace("ö", "o"); metin.replace("Ö", "O");
    return metin;
}

void bip(int frekans, int sure) { tone(SPEAKER_PIN, frekans, sure); }

void printCentered(const char* text, int y, int textSize) {
    display.setTextSize(textSize);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((display.width() - w) / 2, y);
    display.print(text);
}

void havaDurumuGetir() {
    if (WiFi.status() == WL_CONNECTED && token.length() > 5) {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + token + "&units=metric";
        http.begin(url);
        if (http.GET() == HTTP_CODE_OK) {
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, http.getString());
            havaDurumu = String((int)round(doc["main"]["temp"].as<float>())) + " C";
            conditionId = doc["weather"][0]["id"];
        }
        http.end();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(SPEAKER_PIN, OUTPUT); pinMode(ROT_A, INPUT_PULLUP);
    pinMode(ROT_B, INPUT_PULLUP); pinMode(ROT_SW, INPUT_PULLUP);
    bip(2000, 100);
    SPI.begin(EPD_CLK, -1, EPD_DIN, EPD_CS);
    display.init(115200); display.setRotation(3); display.setTextColor(GxEPD_BLACK);
    
    prefs.begin("bilgi", true);
    ssid = prefs.getString("ssid", ""); pass = prefs.getString("pass", "");
    token = prefs.getString("token", ""); city = prefs.getString("city", "Istanbul,TR");
    prefs.end();

    if (ssid != "" && ssid != "null") {
        WiFi.begin(ssid.c_str(), pass.c_str());
        configTime(10800, 0, "pool.ntp.org"); 
    }
    drawMenu();
}

void loop() {
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
            Serial.println("AYARLAR_KAYDEDILDI");
            bip(2500, 200); delay(1000); ESP.restart();
        }
    }

    if (millis() - sonHavaGuncelleme > 1800000 || sonHavaGuncelleme == 0) {
        if (WiFi.status() == WL_CONNECTED) { havaDurumuGetir(); sonHavaGuncelleme = millis(); if (inDetail && currentMenu == MENU_HAVA) drawDetail(); }
    }

    static int sonDakika = -1;
    struct tm timeinfo;
    if (inDetail && currentMenu == MENU_SAAT && getLocalTime(&timeinfo)) {
        if (timeinfo.tm_min != sonDakika) { sonDakika = timeinfo.tm_min; drawDetail(); }
    }

    handleRotary(); handleButton();
}

void handleRotary() {
    int a = digitalRead(ROT_A);
    if (!inDetail && a != lastA) {
        if (digitalRead(ROT_B) != a) currentMenu = (Menu)((currentMenu + 1) % MENU_COUNT);
        else currentMenu = (Menu)((currentMenu + MENU_COUNT - 1) % MENU_COUNT);
        bip(1500, 10); drawMenu();
    }
    lastA = a;
}

void handleButton() {
    int sw = digitalRead(ROT_SW);
    if (sw == LOW && !pressing) { pressing = true; pressStart = millis(); }
    if (sw == HIGH && pressing) {
        unsigned long dur = millis() - pressStart; pressing = false;
        if (dur < 800) { bip(2000, 50); inDetail = true; drawDetail(); }
        else { bip(1000, 150); inDetail = false; drawMenu(); }
    }
}

void drawMenu() {
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        printCentered("BILGIEKRANI", 15, 1);
        display.drawFastHLine(0, 25, display.width(), GxEPD_BLACK);
        const char* label = (currentMenu == MENU_SAAT) ? "> SAAT <" : (currentMenu == MENU_HAVA) ? "> HAVA DURUMU <" : "> ZAMANLAYICI <";
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
                // 1. ÜST BİLGİ: Şehir İsmi (Küçük ve şık)
                display.setTextSize(1);
                String konum = "- " + temizle(city) + " -";
                // Manuel merkezleme: 296px genişlik / 2 = 148. 
                // Karakter başına ~6px, "ISTANBUL,TR" ~11 karakter = 66px. (148 - 33 = 115)
                printCentered(konum.c_str(), 15, 1);

                // 2. ORTA KISIM: Saat (Hassas Manuel Hizalama)
                // Font Size 6 için manuel X, Y koordinatları
                char sBuffer[6]; 
                strftime(sBuffer, 6, "%H:%M", &timeinfo);
                
                display.setTextSize(6);
                // 2.9" Ekran (296x128) için Size 6 Saat Ortalaması:
                // X: 55 (Sol boşluk), Y: 45 (Üst boşluk)
                display.setCursor(55, 45); 
                display.print(sBuffer);

                // 3. ALT KISIM: Tarih ve Gün Bilgisi
                display.drawFastHLine(40, 105, display.width()-80, GxEPD_BLACK); // Estetik ince çizgi

                const char* gunler[] = {"PAZAR", "PAZARTESI", "SALI", "CARSAMBA", "PERSEMBE", "CUMA", "CUMARTESI"};
                char tBuffer[40];
                sprintf(tBuffer, "%02d.%02d.%d | %s", 
                        timeinfo.tm_mday, 
                        timeinfo.tm_mon + 1, 
                        timeinfo.tm_year + 1900, 
                        gunler[timeinfo.tm_wday]);

                // Tarih yazısını en alta ortalayarak bas
                printCentered(tBuffer, 112, 1);

            } else { 
                printCentered("WiFi BEKLENIYOR...", 60, 2); 
            }
        }
        else if (currentMenu == MENU_HAVA) {
            // 1. Üst Kısım: Şehir İsmi
            printCentered(temizle(city).c_str(), 12, 2);
            display.drawFastHLine(20, 30, display.width()-40, GxEPD_BLACK);

            // 2. İkon ve Durum Belirleme
            const unsigned char* icon;
            String text = "";
            if (conditionId >= 200 && conditionId < 300) { icon = icon_storm; text = "FIRTINA"; }
            else if (conditionId >= 300 && conditionId < 600) { icon = icon_rain; text = "YAGMUR"; }
            else if (conditionId >= 600 && conditionId < 700) { icon = icon_snow; text = "KARLI"; }
            else if (conditionId == 800) { icon = icon_clear; text = "ACIK"; }
            else { icon = icon_cloudy; text = "BULUTLU"; }

            // 3. Yan Yana Yerleşim (Sıcaklık Solda - İkon Sağda)
            // Sıcaklık (Sol)
            display.setTextSize(4);
            display.setCursor(30, 75); 
            display.print(havaDurumu);

            // İkon (Sağ) - 32x32 boyutunda
            display.drawBitmap(180, 55, icon, 32, 32, GxEPD_BLACK);
            
            // 4. Alt Kısım: Durum Metni
            display.setTextSize(1);
            display.setCursor(175, 95);
            display.print(text);
            
            // Estetik bir alt bilgi çizgisi
            display.drawFastHLine(0, 115, display.width(), GxEPD_BLACK);
        }
    } while (display.nextPage());
}