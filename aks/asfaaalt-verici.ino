#include <SoftwareSerial.h>
#include "LoRa_E220.h"
#include <LiquidCrystal_I2C.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Wire.h>

//bağlantı pinleri ayarlayın beyler
#define M0_PIN 7
#define M1_PIN 6
#define AUX_PIN 5
#define TX_PIN 3
#define RX_PIN 2

#define ONE_WIRE_BUS 2 
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

SoftwareSerial loraSerial(RX_PIN, TX_PIN);  // RX, TX

LoRa_E220 e220ttl(&loraSerial, AUX_PIN, M0_PIN, M1_PIN);

LiquidCrystal_I2C lcd(0x27,20,4);

const int hallPin = 2; // Hall sensör giriş pini
volatile unsigned long lastTime = 0;
volatile unsigned long period = 0;

unsigned long lastChangeTime = 0; // Son RPM değişim zamanı
double lastRPM = 0;

const int sensorPin = A0;
const float VCC = 5.0;
const float sensitivity = 40.0; // mV/A (ACS758LCB-050B için)
const float zeroCurrentVoltage = VCC / 2; // 2.5V

const int analogPin = A0;
const float R1 = 1000000.0; // 1 MΩ
const float R2 = 100000.0;  // 100 kΩ
const float VadcMax = 4.84;
const int ADCmax = 1023;
const int numCells = 12;

double getSpeed();
void measurePeriod();
float getCurrent();
void yaz(int cursor, String yazi);
float getMaxTemperature();
float getBatteryVoltage();

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
  double T_bat_C = getMaxTemperature();
  double V_bat_C = getBatteryVoltage();
  double kalan_enerji_Wh = getRemainingEnergyWh(V_bat_C);

  yaz(1,(String) hiz);
  yaz(2,(String) T_bat_C);
  yaz(3,(String) V_bat_C);
  yaz(4,(String) kalan_enerji_Wh);


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
  
  delay(500);  
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

float getMaxTemperature() {
  sensors.requestTemperatures();
  int deviceCount = sensors.getDeviceCount();
  float maxTemp = -100.0; // çok düşük başlangıç

  for (int i = 0; i < deviceCount; i++) {
    float tempC = sensors.getTempCByIndex(i);
    if (tempC > maxTemp) {
      maxTemp = tempC;
    }
  }
  return maxTemp;
}

float getBatteryVoltage() {
  int adcValue = analogRead(analogPin);
  float Vtotal = (adcValue * VadcMax / ADCmax) * (R1 + R2) / R2;
  return Vtotal;
}

float getRemainingEnergyWh(float vTotal) {
  const float TOTAL_BATTERY_CAPACITY_WH = 2520.0; 
  const float MAX_BATTERY_VOLTAGE = 50.4;        
  const float MIN_BATTERY_VOLTAGE = 36.4;        


  float chargeRatio = (vTotal - MIN_BATTERY_VOLTAGE) / (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE);

  if (chargeRatio > 1.0) {
    chargeRatio = 1.0;
  } else if (chargeRatio < 0.0) {
    chargeRatio = 0.0;
  }

  float remainingWh = TOTAL_BATTERY_CAPACITY_WH * chargeRatio;

  return remainingWh;
}