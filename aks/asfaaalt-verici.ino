#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <LiquidCrystal_I2C.h>

//bağlantı pinleri ayarlayın beyler
#define M0_PIN 7
#define M1_PIN 6
#define AUX_PIN 5
#define TX_PIN 3
#define RX_PIN 2


SoftwareSerial loraSerial(RX_PIN, TX_PIN);  // RX, TX

LoRa_E220 e220ttl(&loraSerial, AUX_PIN, M0_PIN, M1_PIN);

LiquidCrystal_I2C lcd(0x27,16,2);

const int hallPin = 2; // Hall sensör giriş pini
volatile unsigned long lastTime = 0;
volatile unsigned long period = 0;

unsigned long lastChangeTime = 0; // Son RPM değişim zamanı
double lastRPM = 0;

const int sensorPin = A0;
const float VCC = 5.0;
const float sensitivity = 40.0; // mV/A (ACS758LCB-050B için)
const float zeroCurrentVoltage = VCC / 2; // 2.5V

void setup() {
  Serial.begin(9600);
  delay(500);

  e220ttl.begin();

  lcd.init();
  lcd.backlight();

  pinMode(hallPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(hallPin), measurePeriod, FALLING);

  Serial.println("Gönderici başlatıldı LOG");
}

void loop() {
  double hiz = getSpeed();
  double zaman = millis();
  double T_bat_C = getCurrent();
  double V_bat_C = random(36, 54) / 1.0;
  double kalan_enerji_Wh = random(0, 500);

  String message = String(zaman) + ";" +
                   String(hiz) + ";" +
                   String(T_bat_C) + ";" +
                   String(V_bat_C, 1) + ";" +
                   String(kalan_enerji_Wh);

  ResponseStatus rs = e220ttl.sendFixedMessage(0, 5, 18, message);
  
  Serial.print("Mesaj gönderildi: ");
  Serial.println(message);
  Serial.print("Durum: ");
  Serial.println(rs.getResponseDescription());
  
  delay(2000);  
}

double getSpeed() {
  double rpm = 0;

  if (period > 0) {
    double saniye = period / 1000000.0;
    rpm = 30.0 / saniye;
  }

  // RPM değiştiyse zamanı güncelle
  if (rpm != lastRPM) {
    lastChangeTime = millis();
    lastRPM = rpm;
  }

  // Son değişim 3 saniyeden fazla ise RPM=0
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

