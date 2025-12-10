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
}