<<<<<<< Updated upstream
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AHTX0.h>
#include <SparkFun_ENS160.h>
#include <Adafruit_SI1145.h>
#include <MQUnifiedsensor.h>

// Определение пинов
#define DUST_LED_PIN 2
#define DUST_ANALOG_PIN A0
#define MQ135_PIN A2
#define MQ135_VCC_PIN 7 // Для управления питанием MQ-135 (опционально)

// Параметры MQ-135
#define BOARD "Arduino"
#define VOLTAGE_RESOLUTION 5
#define ADC_BIT_RESOLUTION 10
#define RATIO_MQ135_CLEAN_AIR 3.6 // Rs/R0 ratio in clean air

// Объекты датчиков
Adafruit_AHTX0 aht;
SparkFun_ENS160 ens160;
Adafruit_SI1145 uv = Adafruit_SI1145();
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, "MQ-135");

// Структура для хранения всех данных
struct SensorData {
  // ENS160 + AHT21
  float temperature;
  float humidity;
  int aqi;
  int tvoc;
  int eco2;
  
  // GP2Y1010AU0F (пыль)
  float dustDensity;
  
  // GUVA-S12SD (УФ)
  float uvIndex;
  float uvVoltage;
  
  // MQ-135
  float co2;         // CO2 в ppm
  float nh3;         // Аммиак в ppm
  float alcohol;     // Алкоголь в ppm
  float benzene;     // Бензол в ppm
  float nox;         // NOx в ppm
  float co;          // CO в ppm
  float lpg;         // Сжиженный газ в ppm
  float rawValue;    // Сырое значение с датчика
  float r0;          // Сопротивление в чистом воздухе
};

SensorData data;

// Калибровочные константы для MQ-135
float R0_MQ135 = 10.0; // Начальное значение, требуется калибровка

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  delay(2000);
  
  // Инициализация датчиков
  initializeSensors();
  
  // Калибровка MQ-135 (нужно провести в чистом воздухе)
  calibrateMQ135();
}

void initializeSensors() {
  Serial.println("Инициализация датчиков...");
  
  // Инициализация AHT21
  if (!aht.begin()) {
    Serial.println("Ошибка AHT21!");

  }
  Serial.println("AHT21 OK");
  
  // Инициализация ENS160
  if (!ens160.begin()) {
    Serial.println("Ошибка ENS160!");

  }
  Serial.println("ENS160 OK");
  
  // Сброс ENS160
  if (ens160.setOperatingMode(SFE_ENS160_RESET)) {
    Serial.println("ENS160 сброшен");
  }
  delay(100);
  
  // Установка режима работы ENS160
  ens160.setOperatingMode(SFE_ENS160_STANDARD);
  
  // Инициализация датчика УФ
  if (!uv.begin()) {
    Serial.println("Ошибка датчика УФ!");
  }
  Serial.println("Датчик УФ OK");
  
  // Настройка пинов для датчика пыли
  pinMode(DUST_LED_PIN, OUTPUT);
  pinMode(DUST_ANALOG_PIN, INPUT);
  
  // Настройка MQ-135
  pinMode(MQ135_VCC_PIN, OUTPUT);
  digitalWrite(MQ135_VCC_PIN, HIGH); // Включить питание MQ-135
  
  // Инициализация MQ-135
  MQ135.setRegressionMethod(1); // _PPM =  a*ratio^b
  MQ135.setA(110.47); 
  MQ135.setB(-2.862); 
  
  MQ135.init();
  Serial.print("Calibrating MQ-135, please wait...");
  
  delay(10000); // Прогрев датчика
  
  Serial.println("MQ-135 OK");
  Serial.println("Все датчики инициализированы");
}

void calibrateMQ135() {
  // Калибровка MQ-135 в чистом воздухе
  Serial.println("Калибровка MQ-135...");
  
  // Дать время на прогрев
  delay(30000); // 30 секунд для прогрева
  
  // Рассчитать R0 в чистом воздухе
  float sensor_volt = MQ135.readSensor();
  float RS_air = (5.0 - sensor_volt) / sensor_volt; // Получаем Rs
  data.r0 = RS_air / RATIO_MQ135_CLEAN_AIR; // Получаем R0
  
  MQ135.setR0(data.r0);
  Serial.print("R0 для MQ-135: ");
  Serial.println(data.r0);
}

float readDustSensor() {
  digitalWrite(DUST_LED_PIN, LOW);
  delayMicroseconds(280);
  
  int voMeasured = analogRead(DUST_ANALOG_PIN);
  delayMicroseconds(40);
  
  digitalWrite(DUST_LED_PIN, HIGH);
  delayMicroseconds(9680);
  
  // Конвертация в напряжение
  float calcVoltage = voMeasured * (5.0 / 1024.0);
  
  // Линейная аппроксимация для GP2Y1010AU0F
  // Формула: density (мг/м³) = 0.17 * voltage - 0.1
  float dustDensity = 0.17 * calcVoltage - 0.1;
  
  if (dustDensity < 0) dustDensity = 0;
  
  return dustDensity;
}

void readENS160Data() {
  // Проверка готовности данных
  if (ens160.checkDataStatus()) {
    data.aqi = ens160.getAQI();
    data.tvoc = ens160.getTVOC();
    data.eco2 = ens160.getECO2();
  }
  
  // Чтение температуры и влажности с AHT21
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  
  data.temperature = temp.temperature;
  data.humidity = humidity.relative_humidity;
  
  // Обновление температуры и влажности в ENS160
  ens160.setTempCompensationCelsius(data.temperature);
  ens160.setRHCompensationFloat(data.humidity);
}

void readUVSensor() {
  data.uvIndex = uv.readUV();
  data.uvIndex /= 100.0; // Конвертация в стандартный UV index
  
  // Чтение напряжения (опционально)
  data.uvVoltage = analogRead(A1) * (5.0 / 1024.0);
}

void readMQ135Data() {
  // Чтение сырого значения
  data.rawValue = MQ135.readSensor();
  
  // Установка разных коэффициентов для разных газов
  // CO2
  MQ135.setA(110.47);
  MQ135.setB(-2.862);
  data.co2 = MQ135.readSensor();
  
  // Алкоголь
  MQ135.setA(77.255);
  MQ135.setB(-3.18);
  data.alcohol = MQ135.readSensor();
  
  // CO
  MQ135.setA(605.18);
  MQ135.setB(-3.937);
  data.co = MQ135.readSensor();
  
  // Толуол/Бензол
  MQ135.setA(44.947);
  MQ135.setB(-3.445);
  data.benzene = MQ135.readSensor();
  
  // NOx
  MQ135.setA(-34.654);
  MQ135.setB(1.346);
  data.nox = MQ135.readSensor();
  
  // Аммиак
  MQ135.setA(102.2);
  MQ135.setB(-2.473);
  data.nh3 = MQ135.readSensor();
  
  // LPG
  MQ135.setA(26.654);
  MQ135.setB(-2.222);
  data.lpg = MQ135.readSensor();
  
  // Возвращаем настройки для CO2 (основной газ)
  MQ135.setA(110.47);
  MQ135.setB(-2.862);
}

void printDataToSerial() {
  Serial.println("========== ДАННЫЕ ДАТЧИКОВ ==========");
  Serial.println("--- ENS160 + AHT21 ---");
  Serial.print("Температура: "); Serial.print(data.temperature); Serial.println(" °C");
  Serial.print("Влажность: "); Serial.print(data.humidity); Serial.println(" %");
  Serial.print("AQI: "); Serial.println(data.aqi);
  Serial.print("TVOC: "); Serial.print(data.tvoc); Serial.println(" ppb");
  Serial.print("eCO2: "); Serial.print(data.eco2); Serial.println(" ppm");
  
  Serial.println("--- Датчик пыли GP2Y1010AU0F ---");
  Serial.print("Концентрация пыли: "); Serial.print(data.dustDensity); Serial.println(" мг/м³");
  
  Serial.println("--- Датчик УФ GUVA-S12SD ---");
  Serial.print("УФ индекс: "); Serial.println(data.uvIndex);
  Serial.print("УФ напряжение: "); Serial.print(data.uvVoltage); Serial.println(" В");
  
  Serial.println("--- MQ-135 Датчик газов ---");
  Serial.print("Сырое значение: "); Serial.println(data.rawValue);
  Serial.print("CO2: "); Serial.print(data.co2); Serial.println(" ppm");
  Serial.print("CO: "); Serial.print(data.co); Serial.println(" ppm");
  Serial.print("Алкоголь: "); Serial.print(data.alcohol); Serial.println(" ppm");
  Serial.print("Бензол: "); Serial.print(data.benzene); Serial.println(" ppm");
  Serial.print("Аммиак (NH3): "); Serial.print(data.nh3); Serial.println(" ppm");
  Serial.print("NOx: "); Serial.print(data.nox); Serial.println(" ppm");
  Serial.print("Сжиженный газ (LPG): "); Serial.print(data.lpg); Serial.println(" ppm");
  Serial.print("R0: "); Serial.println(data.r0);
  
  Serial.println("====================================");
}

void printJSONData() {
  // Альтернативный вывод в формате JSON
  Serial.print("{");
  Serial.print("\"temperature\":"); Serial.print(data.temperature); Serial.print(",");
  Serial.print("\"humidity\":"); Serial.print(data.humidity); Serial.print(",");
  Serial.print("\"aqi\":"); Serial.print(data.aqi); Serial.print(",");
  Serial.print("\"tvoc\":"); Serial.print(data.tvoc); Serial.print(",");
  Serial.print("\"eco2\":"); Serial.print(data.eco2); Serial.print(",");
  Serial.print("\"dust\":"); Serial.print(data.dustDensity); Serial.print(",");
  Serial.print("\"uv_index\":"); Serial.print(data.uvIndex); Serial.print(",");
  Serial.print("\"co2_mq135\":"); Serial.print(data.co2); Serial.print(",");
  Serial.print("\"co\":"); Serial.print(data.co); Serial.print(",");
  Serial.print("\"alcohol\":"); Serial.print(data.alcohol); Serial.print(",");
  Serial.print("\"benzene\":"); Serial.print(data.benzene); Serial.print(",");
  Serial.print("\"nh3\":"); Serial.print(data.nh3); Serial.print(",");
  Serial.print("\"nox\":"); Serial.print(data.nox); Serial.print(",");
  Serial.print("\"lpg\":"); Serial.print(data.lpg);
  Serial.println("}");
}

void checkAirQuality() {
  // Проверка качества воздуха на основе показаний
  Serial.println("--- АНАЛИЗ КАЧЕСТВА ВОЗДУХА ---");
  
  // Анализ CO2
  if (data.co2 < 800) {
    Serial.println("CO2: Отличное качество воздуха");
  } else if (data.co2 < 1200) {
    Serial.println("CO2: Хорошее качество воздуха");
  } else if (data.co2 < 2000) {
    Serial.println("CO2: Умеренное качество воздуха");
  } else {
    Serial.println("CO2: Плохое качество воздуха - требуется проветривание!");
  }
  
  // Анализ TVOC
  if (data.tvoc < 250) {
    Serial.println("TVOC: Низкий уровень");
  } else if (data.tvoc < 2000) {
    Serial.println("TVOC: Средний уровень");
  } else {
    Serial.println("TVOC: Высокий уровень!");
  }
  
  // Анализ пыли
  if (data.dustDensity < 0.05) {
    Serial.println("Пыль: Очень чисто");
  } else if (data.dustDensity < 0.15) {
    Serial.println("Пыль: Нормально");
  } else {
    Serial.println("Пыль: Повышенное содержание!");
  }
  
  Serial.println("--------------------------------");
}

void loop() {
  // Чтение данных со всех датчиков
  readENS160Data();
  data.dustDensity = readDustSensor();
  readUVSensor();
  readMQ135Data();
  
  // Вывод в Serial Monitor
  printDataToSerial();
  
  // Альтернативный вывод в JSON
  // printJSONData();
  
  // Анализ качества воздуха
  checkAirQuality();
  
  // Пауза между измерениями
  // MQ-135 требует минимум 2-3 секунды между измерениями
  delay(5000);
=======
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
>>>>>>> Stashed changes
}