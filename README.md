# 📟 E-Paper Nexus: Akıllı Masaüstü Bilgi Ekranı

E-Paper Nexus, ESP32-C3 mikrodenetleyici ve 2.9 inç E-Kağıt (E-Ink) ekran kullanarak hazırlanan, düşük güç tüketimli bir akıllı saat ve hava durumu istasyonudur. Şık bir arayüze sahiptir ve ayarları tamamen web tarayıcısı üzerinden (Web Serial API) kolayca yapılabilir.

## ✨ Özellikler
- **Canlı Saat:** WiFi üzerinden NTP sunucularına bağlanarak saniyeler içinde güncellenen hassas zaman.
- **Hava Durumu:** OpenWeatherMap API entegrasyonu ile anlık sıcaklık ve grafik ikonlar.
- **Web Tabanlı Yapılandırma:** Kod yazmadan, sadece tarayıcı üzerinden WiFi ve API anahtarı ayarı.
- **Düşük Güç Tüketimi:** E-Kağıt teknolojisi sayesinde sadece ekran güncellenirken enerji harcar.
- **Türkçe Karakter Desteği:** Konum isimlerini otomatik temizleyerek hatasız görüntüleme.

## 🛠 Donanım Gereksinimleri
- **Mikrodenetleyici:** ESP32-C3 SuperMini (veya standart ESP32-C3)
- **Ekran:** 2.9" E-Paper Display (BW - Siyah/Beyaz)
- **Giriş:** Rotary Encoder (EC11)
- **Ses:** Pasif Buzzer / Hoparlör (Opsiyonel)

## 📌 Pin Bağlantı Şeması (Wiring)

| E-Paper Ekran | ESP32-C3 GPIO | Açıklama |
| :--- | :--- | :--- |
| **BUSY** | GPIO 2 | Ekran Meşgul Sinyali |
| **RST** | GPIO 3 | Reset |
| **DC** | GPIO 4 | Data / Command |
| **CS** | GPIO 5 | Chip Select |
| **CLK** | GPIO 6 | SPI Clock |
| **DIN** | GPIO 7 | SPI MOSI |
| **VCC** | 3.3V | Güç Girişi |
| **GND** | GND | Toprak |

**Ek Bileşenler:**
- **Rotary A:** GPIO 8
- **Rotary B:** GPIO 9
- **Rotary Switch:** GPIO 10
- **Buzzer (+):** GPIO 1



## ☁️ Hava Durumu API Anahtarı Nasıl Alınır?
Hava durumu verilerini alabilmek için ücretsiz bir API anahtarına ihtiyacınız var:
1. [OpenWeatherMap](https://openweathermap.org/api) adresine gidin.
2. Ücretsiz bir hesap oluşturun.
3. **API Keys** sekmesinden size özel anahtarı kopyalayın.
4. Cihazınızın kontrol panelindeki "OpenWeather API Key" alanına bu anahtarı yapıştırın.

## 🚀 Kurulum ve Kullanım
1. **Flash Modu:** Cihazı **BOOT** tuşuna basılı tutarak bilgisayara takın.
2. **Yükleme:** [Cihaz Kontrol Paneli](https://SITENIZIN_ADRESI.railway.app) adresine gidin.
3. **Yazılım:** "Yazılımı Yükle" butonuna basarak cihazınıza güncel yazılımı kurun.
4. **Yapılandırma:** Yükleme bittikten sonra "Ayarlara Geç" butonuna basın, WiFi bilgilerinizi ve API anahtarınızı girerek "Kaydet" deyin.

## 🤝 Katkıda Bulunun
Bu proje açık kaynaklıdır! Hata bildirimleri, özellik önerileri ve PR (Pull Request) gönderimleri için her zaman açığım.

---
Developed with ❤️ by [Ufuk Çiçek](https://github.com/ufukcicekdev)