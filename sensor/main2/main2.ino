// ==================== БИБЛИОТЕКИ ====================
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <MQUnifiedsensor.h>
#include "SparkFun_ENS160.h"

// ==================== КОНСТАНТЫ И ПЕРЕМЕННЫЕ ====================

// --- Датчик пыли ---
#define DUST_LED_POWER 30
#define DUST_MEASURE_PIN A5
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured;
float calcVoltage;
float dustDensity;

// --- MQ-135 ---
#define MQ135_PIN A3
#define PLACA "Arduino UNO"
#define VOLTAGE_RESOLUTION 5
#define ADC_BIT_RESOLUTION 10
#define TYPE "MQ-135"
#define RATIO_MQ135_CLEAN_AIR 3.6

MQUnifiedsensor MQ135(PLACA, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, TYPE);
float correctionFactor = 0;

// --- ENS160 + AHT21 ---
SparkFun_ENS160 myENS;
Adafruit_AHTX0 aht;
int ensStatus;

// --- УФ датчик (пример для ML8511) ---
#define UV_PIN A2
float uvIntensity = 0;

// Флаг для отслеживания инициализации датчиков
bool sensorsInitialized = false;

// ==================== НАСТРОЙКА ====================
void setup() {
  Serial.begin(115200);
  
  // Даем время на инициализацию Serial
  delay(2000);
  
  Serial.println(F("=== Система мониторинга воздуха ==="));
  Serial.println(F("Инициализация датчиков..."));
  
  // --- Датчик пыли ---
  pinMode(DUST_LED_POWER, OUTPUT);
  Serial.println(F("Пылевой датчик: OK"));
  
  // --- MQ-135 ---
  MQ135.setRegressionMethod(1);
  MQ135.init();
  MQ135.setRL(10);
  
  Serial.print(F("MQ-135 калибровка..."));
  float calcR0 = 0;
  for(int i = 1; i <= 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RATIO_MQ135_CLEAN_AIR);
    Serial.print(".");
    delay(500);
  }
  MQ135.setR0(calcR0 / 10);
  Serial.println(F(" OK"));
  
  // --- ENS160 ---
  Wire.begin();
  if(!myENS.begin()) {
    Serial.println(F("ENS160: ошибка подключения!"));
  } else {
    Serial.println(F("ENS160: OK"));
    if(myENS.setOperatingMode(SFE_ENS160_RESET))
      Serial.println(F("ENS160 сброшен"));
    delay(100);
    myENS.setOperatingMode(SFE_ENS160_STANDARD);
  }
  
  // --- AHT21 ---
  if(!aht.begin()) {
    Serial.println(F("AHT21: не найден!"));
  } else {
    Serial.println(F("AHT21: OK"));
  }
  
  // --- УФ датчик ---
  pinMode(UV_PIN, INPUT);
  Serial.println(F("УФ датчик: OK"));
  
  sensorsInitialized = true;
  Serial.println(F("\n=== Начинаем измерение ===\n"));
  delay(2000);
}

// ==================== ФУНКЦИИ ДАТЧИКОВ ====================

// Функция чтения датчика пыли
float readDustSensor() {
  digitalWrite(DUST_LED_POWER, LOW);
  delayMicroseconds(samplingTime);
  voMeasured = analogRead(DUST_MEASURE_PIN);
  delayMicroseconds(deltaTime);
  digitalWrite(DUST_LED_POWER, HIGH);
  delayMicroseconds(sleepTime);
  
  calcVoltage = voMeasured * (5.0 / 1024.0);
  dustDensity = 0.17 * calcVoltage - 0.1;
  
  if(dustDensity < 0) dustDensity = 0;
  return dustDensity;
}

// Функция чтения MQ-135
void readMQ135(float &co, float &alcohol, float &co2, float &toluene, float &nh4, float &acetone) {
  MQ135.update();
  
  MQ135.setA(605.18); MQ135.setB(-3.937);
  co = MQ135.readSensor(false, correctionFactor);
  
  MQ135.setA(77.255); MQ135.setB(-3.18);
  alcohol = MQ135.readSensor(false, correctionFactor);
  
  MQ135.setA(110.47); MQ135.setB(-2.862);
  co2 = MQ135.readSensor(false, correctionFactor) + 400;
  
  MQ135.setA(44.947); MQ135.setB(-3.445);
  toluene = MQ135.readSensor(false, correctionFactor);
  
  MQ135.setA(102.2); MQ135.setB(-2.473);
  nh4 = MQ135.readSensor(false, correctionFactor);
  
  MQ135.setA(34.668); MQ135.setB(-3.369);
  acetone = MQ135.readSensor(false, correctionFactor);
}

// Функция чтения ENS160 и AHT21
bool readENS160_AHT21(float &temp, float &hum, int &aqi, float &tvoc, float &eco2) {
  static bool ensOK = true;
  static bool ahtOK = true;
  
  // Чтение AHT21
  if(ahtOK) {
    sensors_event_t humidity, temperature;
    if(aht.getEvent(&humidity, &temperature)) {
      temp = temperature.temperature;
      hum = humidity.relative_humidity;
    } else {
      ahtOK = false;
      temp = hum = -999;
    }
  }
  
  // Чтение ENS160
  if(ensOK) {
    if(myENS.checkDataStatus()) {
      aqi = myENS.getAQI();
      tvoc = myENS.getTVOC();
      eco2 = myENS.getECO2();
      ensStatus = myENS.getFlags();
    } else {
      aqi = tvoc = eco2 = -999;
    }
  }
  
  return (ensOK && ahtOK);
}

// Функция чтения УФ датчика (ML8511)
float readUVSensor() {
  int uvLevel = analogRead(UV_PIN);
  float outputVoltage = 3.3 * uvLevel / 1024.0;
  uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);
  if(uvIntensity < 0) uvIntensity = 0;
  return uvIntensity;
}

// Вспомогательная функция для маппинга float
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Функция для отправки данных в простом формате
void sendSensorData(float temp, float hum, int aqi, float tvoc, float eco2,
                    float co, float alcohol, float co2_mq, float toluene, 
                    float nh4, float acetone, float dust, float calcVolt, 
                    float rawValue, float uv) {
  
  // Отправляем стартовый маркер
  Serial.println("DATA_START");
  
  // AHT21 данные
  Serial.print("TEMP:");
  Serial.println(temp, 2);
  
  Serial.print("HUM:");
  Serial.println(hum, 2);
  
  // ENS160 данные (только если есть данные)
  if (aqi != -999) {
    Serial.print("AQI:");
    Serial.println(aqi);
    
    Serial.print("TVOC:");
    Serial.println(tvoc, 0);
    
    Serial.print("ECO2:");
    Serial.println(eco2, 0);
  } else {
    Serial.println("ENS160:NO_DATA");
  }
  
  // MQ-135 данные
  Serial.print("CO:");
  Serial.println(co, 2);
  
  Serial.print("ALC:");
  Serial.println(alcohol, 2);
  
  Serial.print("CO2_MQ:");
  Serial.println(co2_mq, 0);
  
  Serial.print("TOL:");
  Serial.println(toluene, 2);
  
  Serial.print("NH4:");
  Serial.println(nh4, 2);
  
  Serial.print("ACE:");
  Serial.println(acetone, 2);
  
  // Датчик пыли
  Serial.print("DUST:");
  Serial.println(dust, 2);
  
  Serial.print("VOLT:");
  Serial.println(calcVolt, 2);
  
  Serial.print("RAW:");
  Serial.println(rawValue, 0);
  
  // УФ датчик
  Serial.print("UV:");
  Serial.println(uv, 2);
  
  // Отправляем завершающий маркер
  Serial.println("DATA_END");
  Serial.println(); // Пустая строка для разделения пакетов
}

// ==================== ОСНОВНОЙ ЦИКЛ ====================
void loop() {
  static unsigned long lastPrint = 0;
  unsigned long currentMillis = millis();
  
  // Вывод данных каждые 2 секунды
  if(currentMillis - lastPrint >= 2000) {
    lastPrint = currentMillis;
    
    if (!sensorsInitialized) {
      Serial.println("SENSORS_NOT_INITIALIZED");
      return;
    }
    
    // Чтение всех датчиков
    float dust = readDustSensor();
    
    float co, alcohol, co2_mq, toluene, nh4, acetone;
    readMQ135(co, alcohol, co2_mq, toluene, nh4, acetone);
    
    float temp = 0, hum = 0;
    int aqi = 0;
    float tvoc = 0, eco2 = 0;
    bool ensAhtOK = readENS160_AHT21(temp, hum, aqi, tvoc, eco2);
    
    float uv = readUVSensor();
    
    // Отправка данных в упрощенном формате для ESP8266
    sendSensorData(temp, hum, aqi, tvoc, eco2,
                   co, alcohol, co2_mq, toluene, nh4, acetone,
                   dust, calcVoltage, voMeasured, uv);
    
    // Также можно оставить отладочный вывод для монитора порта (опционально)
    #ifdef DEBUG_SERIAL
    Serial.println(F("\n=========================================="));
    Serial.println(F("ДАННЫЕ С ДАТЧИКОВ ВОЗДУХА (DEBUG)"));
    Serial.println(F("=========================================="));
    
    if(temp != -999) {
      Serial.print(F("Температура: ")); Serial.print(temp); Serial.println(F(" °C"));
      Serial.print(F("Влажность: ")); Serial.print(hum); Serial.println(F(" %"));
    }
    
    if(aqi != -999) {
      Serial.print(F("Индекс AQI: ")); Serial.println(aqi);
      Serial.print(F("TVOC: ")); Serial.print(tvoc); Serial.println(F(" ppb"));
      Serial.print(F("eCO2: ")); Serial.print(eco2); Serial.println(F(" ppm"));
    }
    
    Serial.print(F("CO: ")); Serial.print(co); Serial.println(F(" ppm"));
    Serial.print(F("Аммиак: ")); Serial.print(nh4); Serial.println(F(" ppm"));
    Serial.print(F("Пыль: ")); Serial.print(dust); Serial.println(F(" мкг/м³"));
    Serial.print(F("УФ: ")); Serial.print(uv); Serial.println(F(" mW/cm²"));
    
    Serial.println(F("==========================================\n"));
    #endif
  }
  
  delay(100); // Небольшая задержка для стабильности
}