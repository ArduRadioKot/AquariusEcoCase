#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Конфигурация
const char* AP_SSID = "ESP8266_Config";
const char* AP_PASSWORD = "12345678";

// Сервер для конфигурации
ESP8266WebServer server(80);

// Для хранения данных в EEPROM
struct WiFiConfig {
  char ssid[32];
  char password[64];
  bool configured;
};

WiFiConfig wifiConfig;

// Флаги состояний
bool isConfigured = false;
bool shouldReboot = false;
unsigned long lastDataSend = 0;
const unsigned long DATA_INTERVAL = 5000; // 5 секунд

// Настройки сервера - ЗАМЕНИТЕ НА СВОИ
const char* serverIP = "192.168.0.106"; 
const int serverPort = 5000;

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
  
  if (isConfigured) {
    // Режим клиента - отправка данных
    server.handleClient();
    
    if (WiFi.status() == WL_CONNECTED) {
      // Отправка данных каждые 5 секунд
      if (millis() - lastDataSend > DATA_INTERVAL) {
        sendDataToServer();
        lastDataSend = millis();
      }
      
      // Чтение из Serial и отправка
      if (Serial.available()) {
        String data = Serial.readStringUntil('\n');
        data.trim();
        if (data.length() > 0) {
          Serial.print("Received from Serial: ");
          Serial.println(data);
          sendSerialData(data);
        }
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

void sendDataToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    
    Serial.print("Connecting to server for data: ");
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(serverPort);
    
    if (client.connect(serverIP, serverPort)) {
      // Генерация случайных данных сенсоров
      float temperature = 18.0 + random(0, 150) / 10.0;  // 18-33°C
      float humidity = 30.0 + random(0, 500) / 10.0;      // 30-80%
      int co2 = 400 + random(0, 600);                     // 400-1000 ppm
      int pm25 = random(0, 50);                           // 0-50 µg/m³
      int pm10 = random(0, 100);                           // 0-100 µg/m³
      float pressure = 980.0 + random(0, 600) / 10.0;     // 980-1040 hPa
      int voc = random(0, 500);                            // 0-500 ppb
      float ammonia = random(0, 30) / 10.0;               // 0-3.0 ppm
      int nox = random(0, 100);                            // 0-100 ppb
      float benzene = random(0, 50) / 10.0;                // 0-5.0 ppb
      int uv_index = random(0, 12);                       // 0-12
      
      // Формирование POST данных
      String postData = "device=esp01";
      postData += "&status=online";
      postData += "&free_memory=" + String(ESP.getFreeHeap());
      postData += "&temperature=" + String(temperature, 1);
      postData += "&humidity=" + String(humidity, 1);
      postData += "&co2=" + String(co2);
      postData += "&pm25=" + String(pm25);
      postData += "&pm10=" + String(pm10);
      postData += "&pressure=" + String(pressure, 1);
      postData += "&voc=" + String(voc);
      postData += "&ammonia=" + String(ammonia, 1);
      postData += "&nox=" + String(nox);
      postData += "&benzene=" + String(benzene, 1);
      postData += "&uv_index=" + String(uv_index);
      
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
      Serial.println("Sensor data sent to server successfully");
      Serial.print("Temperature: "); Serial.print(temperature); Serial.println("°C");
      Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
      Serial.print("CO2: "); Serial.print(co2); Serial.println("ppm");
    } else {
      Serial.println("Failed to connect to server for data");
    }
  }
}

void sendSerialData(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    
    Serial.print("Connecting to server for serial data: ");
    Serial.print(serverIP);
    Serial.print(":");
    Serial.println(serverPort);
    
    if (client.connect(serverIP, serverPort)) {
      String postData = "device=esp01&serial_data=" + urlEncode(data);
      
      client.println("POST /serial HTTP/1.1");
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
      Serial.println("Serial data sent successfully: " + data);
    } else {
      Serial.println("Failed to connect to server for serial data");
    }
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