bool isConfigured = false;
bool shouldReboot = false;
unsigned long lastDataSend = 0;
const unsigned long DATA_INTERVAL = 5000; // 5 секунд

// Настройки сервера
const char* serverIP = "82.202.142.35"; 
const int serverPort = 8080;

// Структура для хранения данных с датчиков
struct SensorData {
  // AHT21
  float temperature = 0;
  float humidity = 0;
  
  // ENS160
  int aqi = 0;
  float tvoc = 0;
  float eco2 = 0;
  
  // MQ-135
  float co = 0;
  float alcohol = 0;
  float co2 = 0;
  float toluene = 0;
  float nh4 = 0;
  float acetone = 0;
  
  // Dust sensor
  float dust = 0;
  float calcVoltage = 0;
  float voMeasured = 0;
  
  // UV sensor
  float uv = 0;
  
  bool dataValid = false;
  unsigned long lastUpdate = 0;
};

SensorData currentData;

// HTML страница для конфигурации
const char* configPage = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>ESP8266 WiFi Configuration</title>
  <meta charset="utf-8">
  <style>
    body { font-family: Arial; margin: 40px; }
    .container { max-width: 400px; margin: 0 auto; }
    input { width: 100%; padding: 10px; margin: 8px 0; }
    button { width: 100%; padding: 12px; background: #4CAF50; color: white; border: none; }
  </style>
</head>
<body>
  <div class="container">
    <h2>WiFi Configuration</h2>
    <form action="/save" method="POST">
      <input type="text" name="ssid" placeholder="WiFi SSID" required>
      <input type="password" name="password" placeholder="WiFi Password" required>
      <button type="submit">Save & Connect</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  
  // Загрузка конфигурации из EEPROM
  loadConfig();
  
  if (wifiConfig.configured) {
    // Подключение к WiFi
    connectToWiFi();
  } else {
    // Режим точки доступа для конфигурации
    setupAP();
  }
}

void loop() {
  if (shouldReboot) {
    delay(1000);
    ESP.restart();
  }
  
  // Чтение данных из Serial порта (от Arduino)
  readSerialData();
  
  if (isConfigured) {
    // Режим клиента - отправка данных
    server.handleClient();
    
    if (WiFi.status() == WL_CONNECTED) {
      // Отправка данных каждые 5 секунд
      if (millis() - lastDataSend > DATA_INTERVAL) {
        sendDataToServer();
        lastDataSend = millis();
      }
    } else {
      // Переподключение при потере соединения
      connectToWiFi();
    }
  } else {
    // Режим точки доступа
    server.handleClient();
  }
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  // Настройка сервера
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", configPage);
  });
  
  server.on("/save", HTTP_POST, handleSaveConfig);
  
  server.begin();
  Serial.println("AP Mode started");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void handleSaveConfig() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    // Сохранение в EEPROM
    strncpy(wifiConfig.ssid, ssid.c_str(), sizeof(wifiConfig.ssid));
    strncpy(wifiConfig.password, password.c_str(), sizeof(wifiConfig.password));
    wifiConfig.configured = true;
    
    saveConfig();
    
    server.send(200, "text/html", 
      "<html><body><h2>Configuration saved!</h2><p>Device will restart and connect to WiFi.</p></body></html>");
    
    shouldReboot = true;
  } else {
    server.send(400, "text/plain", "Missing SSID or Password");
  }
}

void loadConfig() {
  EEPROM.get(0, wifiConfig);
  if (wifiConfig.configured) {
    isConfigured = true;
  }
}

void saveConfig() {
  EEPROM.put(0, wifiConfig);
  EEPROM.commit();
}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(wifiConfig.ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiConfig.ssid, wifiConfig.password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
    // Возврат в режим AP при неудаче
    setupAP();
    isConfigured = false;
  }
}

void readSerialData() {
  static String serialBuffer = "";
  
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n') {
      // Обработка завершенной строки
      processSerialData(serialBuffer);
      serialBuffer = "";
    } else if (c != '\r') {
      serialBuffer += c;
    }
  }
}

void processSerialData(String data) {
  data.trim();
  
  if (data.length() == 0) return;
  
  // Отладочный вывод
  Serial.print("Received from Arduino: ");
  Serial.println(data);
  
  if (data == "DATA_START") {
    // Начало нового пакета данных - сбрасываем флаг
    currentData.dataValid = false;
    return;
  }
  
  if (data == "DATA_END") {
    // Конец пакета данных - помечаем данные как валидные
    currentData.dataValid = true;
    currentData.lastUpdate = millis();
    
    Serial.println("\n=== Полный пакет данных получен ===");
    Serial.print("Temperature: "); Serial.print(currentData.temperature); Serial.println("°C");
    Serial.print("Humidity: "); Serial.print(currentData.humidity); Serial.println("%");
    Serial.print("CO: "); Serial.print(currentData.co); Serial.println("ppm");
    Serial.print("NH4: "); Serial.print(currentData.nh4); Serial.println("ppm");
    Serial.print("CO2: "); Serial.print(currentData.co2); Serial.println("ppm");
    Serial.print("AQI: "); Serial.println(currentData.aqi);
    Serial.print("Dust: "); Serial.print(currentData.dust); Serial.println("µg/m³");
    Serial.print("UV: "); Serial.print(currentData.uv); Serial.println("mW/cm²");
    Serial.println("===================================\n");
    return;
  }
  
  if (data == "ENS160:NO_DATA") {
    // Датчик ENS160 не работает
    Serial.println("ENS160: нет данных");
    return;
  }
  
  if (data == "SENSORS_NOT_INITIALIZED") {
    Serial.println("Датчики не инициализированы на Arduino");
    return;
  }
  
  // Парсим данные в формате КЛЮЧ:ЗНАЧЕНИЕ
  int colonIndex = data.indexOf(':');
  if (colonIndex == -1) return;
  
  String key = data.substring(0, colonIndex);
  String valueStr = data.substring(colonIndex + 1);
  valueStr.trim();
  
  float value = valueStr.toFloat();
  
  // Парсим данные по ключам
  if (key == "TEMP") {
    currentData.temperature = value;
  } 
  else if (key == "HUM") {
    currentData.humidity = value;
  }
  else if (key == "AQI") {
    currentData.aqi = (int)value;
  }
  else if (key == "TVOC") {
    currentData.tvoc = value;
  }
  else if (key == "ECO2") {
    currentData.eco2 = value;
  }
  else if (key == "CO") {
    currentData.co = value;
  }
  else if (key == "ALC") {
    currentData.alcohol = value;
  }
  else if (key == "CO2_MQ") {
    currentData.co2 = value;
  }
  else if (key == "TOL") {
    currentData.toluene = value;
  }
  else if (key == "NH4") {
    currentData.nh4 = value;
  }
  else if (key == "ACE") {
    currentData.acetone = value;
  }
  else if (key == "DUST") {
    currentData.dust = value;
  }
  else if (key == "VOLT") {
    currentData.calcVoltage = value;
  }
  else if (key == "RAW") {
    currentData.voMeasured = value;
  }
  else if (key == "UV") {
    currentData.uv = value;
  }
}

void sendDataToServer() {
  // Проверяем, есть ли актуальные данные
  bool hasRecentData = (millis() - currentData.lastUpdate < 10000); // Данные актуальны если получены менее 10 секунд назад
  
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    
    Serial.print("Connecting to server for data: ");
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(serverPort);
    
    if (client.connect(serverIP, serverPort)) {
      // Формирование POST данных
      String postData = "device=esp01";
      postData += "&status=" + String(hasRecentData ? "online" : "no_data");
      postData += "&free_memory=" + String(ESP.getFreeHeap());
      
      if (hasRecentData && currentData.dataValid) {
        // AHT21 данные
        postData += "&temperature=" + String(currentData.temperature, 1);
        postData += "&humidity=" + String(currentData.humidity, 1);
        
        // ENS160 данные
        postData += "&aqi=" + String(currentData.aqi);
        postData += "&tvoc=" + String(currentData.tvoc, 0);
        postData += "&eco2=" + String(currentData.eco2, 0);
        
        // MQ-135 данные
        postData += "&co=" + String(currentData.co, 2);
        postData += "&alcohol=" + String(currentData.alcohol, 2);
        postData += "&co2_real=" + String(currentData.co2, 0);
        postData += "&toluene=" + String(currentData.toluene, 2);
        postData += "&ammonia=" + String(currentData.nh4, 2);
        postData += "&acetone=" + String(currentData.acetone, 2);
        
        // Dust sensor
        postData += "&pm25=" + String(currentData.dust, 1);
        postData += "&pm10=" + String(currentData.dust * 1.5, 1);
        postData += "&dust_density=" + String(currentData.dust, 1);
        
        // UV sensor
        postData += "&uv_index=" + String(currentData.uv, 1);
        
        // Debug info
        postData += "&calc_voltage=" + String(currentData.calcVoltage, 2);
        postData += "&raw_value=" + String(currentData.voMeasured, 0);
        
        Serial.println("\nОтправляем актуальные данные на сервер:");
        Serial.print("Temperature: "); Serial.println(currentData.temperature);
        Serial.print("Humidity: "); Serial.println(currentData.humidity);
        Serial.print("NH4: "); Serial.println(currentData.nh4);
        Serial.print("Dust: "); Serial.println(currentData.dust);
      } else {
        // Если данных нет - отправляем 0
        postData += "&temperature=0";
        postData += "&humidity=0";
        postData += "&aqi=0";
        postData += "&tvoc=0";
        postData += "&eco2=0";
        postData += "&co=0";
        postData += "&alcohol=0";
        postData += "&co2_real=0";
        postData += "&toluene=0";
        postData += "&ammonia=0";
        postData += "&acetone=0";
        postData += "&pm25=0";
        postData += "&pm10=0";
        postData += "&dust_density=0";
        postData += "&uv_index=0";
        
        Serial.println("Нет актуальных данных от датчиков - отправляем 0");
      }
      
      client.println("POST /data HTTP/1.1");
      client.println("Host: " + String(serverIP) + ":" + String(serverPort));
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.println(postData);
      
      // Ждем ответа
      delay(100);
      while (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
      }
      
      client.stop();
      
      if (hasRecentData && currentData.dataValid) {
        Serial.println("Sensor data sent to server successfully");
      } else {
        Serial.println("Zero data sent to server (no sensor data available)");
      }
    } else {
      Serial.println("Failed to connect to server for data");
    }
  } else {
    Serial.println("WiFi not connected - cannot send data");
  }
}

String urlEncode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  
  return encodedString;
}