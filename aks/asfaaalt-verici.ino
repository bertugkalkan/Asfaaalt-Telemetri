#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <LiquidCrystal_I2C.h>

const int chipSelect = 4; // SD kart modülünün CS pini girilmeli.
const char* unsentLogFile = "unsent.txt";
const char* tempLogFile = "temp.txt";
unsigned long outageStartTime = 0; // Sinyal kesintisinin başlangıç zamanı tutulacak

#define M0_PIN 7
#define M1_PIN 6
#define AUX_PIN 5
#define TX_PIN 3
#define RX_PIN 2


SoftwareSerial loraSerial(RX_PIN, TX_PIN);  

LoRa_E220 e220ttl(&loraSerial, AUX_PIN, M0_PIN, M1_PIN);

LiquidCrystal_I2C lcd(0x27,16,2);

const int hallPin = 2; 
volatile unsigned long lastTime = 0;
volatile unsigned long period = 0;

unsigned long lastChangeTime = 0; 
double lastRPM = 0;

const int sensorPin = A0;
const float VCC = 5.0;
const float sensitivity = 40.0; 
const float zeroCurrentVoltage = VCC / 2; 


void logUnsent(const String& data);
String readFirstLine(const char* filename);
void removeFirstLine(const char* filename);


void setup() {
  Serial.begin(9600);
  delay(500);

  Serial.print("SD Kart baslatiliyor...");
  if (!SD.begin(chipSelect)) {
    Serial.println("HATA: SD Kart baslatilamadi! Program durduruluyor.");
    while (1); 
  }
  Serial.println("OK");

  e220ttl.begin();

  lcd.init();
  lcd.backlight();

  pinMode(hallPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(hallPin), measurePeriod, FALLING);

  Serial.println("Gonderici baslatildi LOG");
}

void loop() {
  double hiz = getSpeed();
  double zaman = millis();
  double T_bat_C = getCurrent();
  double V_bat_C = random(36, 54) / 1.0;
  double kalan_enerji_Wh = random(0, 500);

  String liveMessage = String(zaman) + ";" +
                   String(hiz) + ";" +
                   String(T_bat_C) + ";" +
                   String(V_bat_C, 1) + ";" +
                   String(kalan_enerji_Wh);

  String backlogData = readFirstLine(unsentLogFile);
  
  if (backlogData.length() > 0) {
    // Birikmiş veri var. Göndermeyi dene.
    if (e220ttl.sendFixedMessage(0, 5, 18, backlogData).isSuccess()) {
      // BAŞARILIYSA
      outageStartTime = 0; // Bağlantı var demektir, kesinti sayacını sıfırla.
      Serial.println("Birikmis veri gonderildi: " + backlogData);
      removeFirstLine(unsentLogFile); // Gönderilen veriyi dosya başından silme.
      
      // Canlı veri uyumlu olsun diye en sona ekleme.
      logUnsent(liveMessage);
      Serial.println("Anlik veri siraya eklendi: " + liveMessage);

    } else {
      // BAŞARISIZSA Bağlantı Yok.
      // Kesinti sayacının başladığından emin ol.
      if (outageStartTime == 0) {
          outageStartTime = millis();
          Serial.println("Sinyal kesintisi basladi.");
      }
      
      // 60 saniye kuralın kontrol
      if (millis() - outageStartTime < 60000) {
        logUnsent(liveMessage);
        Serial.println("Baglanti yok, anlik veri de siraya eklendi: " + liveMessage);
      } else {
        Serial.println("60sn doldu, yeni veri (" + liveMessage + ") kaydedilmiyor.");
      }
    }
  } else {
    // Birikmiş Yoksa Canlıya Hopla
    if (e220ttl.sendFixedMessage(0, 5, 18, liveMessage).isSuccess()) {
      // BAŞARILI: Canlı veri gitti. Her şey yolunda.
      outageStartTime = 0; // Kesinti sayacını sıfır tut.
      Serial.println("Anlik veri gonderildi: " + liveMessage);
    } else {
      // BAŞARISIZ: Canlı veri gönderilemedi. Kesintiyi başlat ve veriyi logla.
      if (outageStartTime == 0) {
          outageStartTime = millis();
          Serial.println("Sinyal kesintisi basladi.");
      }
      
      // 60 saniye kuralı burada da geçerli.
      if (millis() - outageStartTime < 60000) {
        logUnsent(liveMessage);
        Serial.println("Baglanti yok, anlik veri SD karta kaydedildi: " + liveMessage);
      }
    }
  }
  
  delay(2000);
}

double getSpeed() {
  double rpm = 0;

  if (period > 0) {
    double saniye = period / 1000000.0;
    rpm = 30.0 / saniye;
  }

  if (rpm != lastRPM) {
    lastChangeTime = millis();
    lastRPM = rpm;
  }

  if (millis() - lastChangeTime > 3000) {
    rpm = 0;
  }

  double hiz = rpm * 60.0 * 1.8/ 1000;

  return hiz;
}

void measurePeriod() {
  unsigned long currentTime = micros();
  period = currentTime - lastTime;
  lastTime = currentTime;
}

float getCurrent() {
  int sensorValue = analogRead(sensorPin);
  float voltage = sensorValue * (VCC / 1023.0);
  float current = (voltage - zeroCurrentVoltage) * 1000 / sensitivity;
  return current;
}
void yaz(int cursor, String yazi) {
  lcd.setCursor(0,cursor);
  lcd.print(yazi);
}

// --- YENİ EKLENEN SD KART FONKSİYONLARI ---

// Veriyi log dosyasının sonuna ekler
void logUnsent(const String& data) {
  File file = SD.open(unsentLogFile, FILE_WRITE);
  if (file) {
    file.println(data);
    file.close();
  } else {
    Serial.println("HATA: Log dosyasi (unsent.txt) acilamadi!");
  }
}

// Dosyanın ilk satırını okur
String readFirstLine(const char* filename) {
  File file = SD.open(filename);
  String firstLine = "";
  if (file && file.size() > 0) {
    while (file.available()) {
      char c = file.read();
      if (c == '\n') {
        break;
      }
      firstLine += c;
    }
  }
  file.close();
  return firstLine;
}

// Dosyanın ilk satırını siler
void removeFirstLine(const char* filename) {
  File originalFile = SD.open(filename, FILE_READ);
  File tempFile = SD.open(tempLogFile, FILE_WRITE);

  if (!originalFile || !tempFile) {
    Serial.println("HATA: Dosya silme/yeniden yazma isleminde hata!");
    if(originalFile) originalFile.close();
    if(tempFile) tempFile.close();
    return;
  }

  bool firstLineSkipped = false;
  while (originalFile.available()) {
    char c = originalFile.read();
    if (!firstLineSkipped) {
      if (c == '\n') {
        firstLineSkipped = true;
      }
    } else {
      tempFile.write(c);
    }
  }

  originalFile.close();
  tempFile.close();

  SD.remove(filename);
  SD.rename(tempLogFile, filename);

  File f = SD.open(filename);
  if (f && f.size() == 0) {
    f.close();
    SD.remove(filename);
  } else if(f) {
    f.close();
  }
}
