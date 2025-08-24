#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <LiquidCrystal_I2C.h>


#define M0_PIN 7
#define M1_PIN 6
#define AUX_PIN 5
#define TX_PIN 3
#define RX_PIN 2

SoftwareSerial loraSerial(RX_PIN, TX_PIN);  // RX, TX

LoRa_E220 e220ttl(&loraSerial, AUX_PIN, M0_PIN, M1_PIN);

LiquidCrystal_I2C lcd(0x27,16,2);

struct Telemetri {
  double zaman;
  double hiz;
  double T_bat_C;
  double V_bat_C;
  double kalan_enerji_Wh;
};

void setup() {
  Serial.begin(9600);
  delay(500);

  // LoRa modülünü başlat
  e220ttl.begin();

  lcd.init();
  lcd.backlight();

  Serial.println("Alıcı başlatıldı - Adres: 5, Kanal: 18, RSSI: Aktif");
  Serial.println("Gelen mesajlar bekleniyor...");
}

void loop() {
  if (e220ttl.available() > 0) {

    ResponseContainer rc = e220ttl.receiveMessageRSSI();

    Telemetri t = parseMessage(rc.data);

    if (rc.status.code == 1) {
      Serial.println(rc.data);
      yaz(0, (String) t.hiz);
    } else {
      Serial.print("Hata: ");
      Serial.println(rc.status.getResponseDescription());
    }
  }
  
  delay(100);
}

void yaz(int cursor, String yazi) {
  lcd.setCursor(0,cursor);
  lcd.print(yazi);
}



Telemetri parseMessage(String message) {
  Telemetri veri;

  int index = 0;
  int start = 0;

  index = message.indexOf(';', start);
  veri.zaman = message.substring(start, index).toDouble();
  start = index + 1;

  index = message.indexOf(';', start);
  veri.hiz = message.substring(start, index).toDouble();
  start = index + 1;

  index = message.indexOf(';', start);
  veri.T_bat_C = message.substring(start, index).toDouble();
  start = index + 1;

  index = message.indexOf(';', start);
  veri.V_bat_C = message.substring(start, index).toDouble();
  start = index + 1;

  veri.kalan_enerji_Wh = message.substring(start).toDouble();

  return veri;
}