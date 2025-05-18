#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266Ping.h>
#include <ArduinoJson.h>

// WiFi y Telegram
const char* ssid = "Wifi Name";
const char* password = "Wifi Pass";
const String botToken = "7031568892:AAGefuyKv7VeHuZjIA_LeSggZ3McO0xtL6Y";
const String chatId = "6012153212";

WiFiClientSecure client;
unsigned long lastCheck = 0;
const unsigned long checkInterval = 15000; // 15 segundos
int lastUpdateId = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi conectado!");
  client.setInsecure(); // Desactiva verificación SSL
  Serial.println("🤖 Bot listo y conectado a WiFi 📡");
  sendMessage("🤖 Bot listo y conectado a WiFi 📡");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastCheck >= checkInterval) {
      checkTelegramMessages();
      lastCheck = currentMillis;
    }
  }
}

void sendMessage(String message) {
  if (client.connect("api.telegram.org", 443)) {
    String url = "/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + urlencode(message);
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "Connection: close\r\n\r\n");

    while (client.connected()) {
      client.read();
    }

    client.stop();
    Serial.println("✅ Mensaje enviado correctamente.");
  } else {
    Serial.println("❌ Error al enviar mensaje.");
  }
}

void checkTelegramMessages() {
  String url = "https://api.telegram.org/bot" + botToken + "/getUpdates?offset=" + String(lastUpdateId + 1);

  if (client.connect("api.telegram.org", 443)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.connected() && !client.available()) {
      if (millis() - timeout > 2000) {
        Serial.println("❌ Timeout esperando respuesta del servidor.");
        client.stop();
        return;
      }
    }

    String payload = "";
    bool headersEnded = false;

    while (client.available()) {
      String line = client.readStringUntil('\n');
      if (!headersEnded) {
        if (line == "\r") headersEnded = true;
      } else {
        payload += line;
      }
    }

    client.stop();

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray result = doc["result"].as<JsonArray>();
      if (result.size() > 0) {
        JsonObject msg = result[result.size() - 1];
        int updateId = msg["update_id"];
        String text = msg["message"]["text"];

        lastUpdateId = updateId + 1;
        Serial.println("📩 Mensaje recibido: " + text);
        handleCommand(text);
      }
    } else {
      Serial.println("❌ Error parseando JSON: " + String(error.c_str()));
      Serial.println("Payload recibido:");
      Serial.println(payload);
    }
  } else {
    Serial.println("❌ Fallo en la conexión HTTPS con Telegram.");
  }
}

void handleCommand(String text) {
  if (text == "/ping") {
    handlePing();
  } else if (text == "/speedtest") {
    handleSpeedTest();
  } else if (text == "/ip") {
    sendMessage("📡 IP actual: " + WiFi.localIP().toString());
  } else if (text == "/stats") {
    sendStats();
  } else if (text == "/uptime") {
    sendMessage("⏳ Tiempo de actividad: " + String(millis() / 1000) + " segundos");
  } else if (text == "/restart") {
    sendMessage("🔄 Reiniciando el bot...");
    delay(1000);
    ESP.restart();
  } else if (text == "/stop") {
    sendMessage("❌ Deteniendo el bot...");
    delay(1000);
    ESP.deepSleep(0);
  } else {
    sendMessage("❌ Comando no reconocido.");
  }
}

void handlePing() {
  sendMessage("📡 Realizando prueba de latencia a 8.8.8.8...");
  if (Ping.ping("8.8.8.8", 3)) {
    sendMessage("✅ Ping a 8.8.8.8 exitoso. Tiempo medio: " + String(Ping.averageTime()) + " ms");
  } else {
    sendMessage("❌ Falló el ping a 8.8.8.8");
  }

  sendMessage("📡 Probando ping a las.leagueoflegends.com...");
  if (Ping.ping("las.leagueoflegends.com", 3)) {
    sendMessage("✅ Ping a League of Legends exitoso. Tiempo medio: " + String(Ping.averageTime()) + " ms");
  } else {
    sendMessage("❌ Falló el ping a League of Legends");
  }
}

void handleSpeedTest() {
  sendMessage("📡 Iniciando test de latencia...");

  WiFiClient testClient;
  HTTPClient http;
  const char* url = "http://speedtest.tele2.net/100KB.zip";

  unsigned long startTime = millis();
  http.begin(testClient, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    unsigned long endTime = millis();
    int len = http.getSize();

    if (httpCode == HTTP_CODE_OK) {
      sendMessage("✅ Test completado.\n📦 Tamaño: " + String(len / 1024) + " KB\n⏱️ Latencia estimada: " + String(endTime - startTime) + " ms");
    } else {
      sendMessage("⚠️ Código HTTP recibido: " + String(httpCode));
    }
  } else {
    sendMessage("❌ Error al realizar el test: " + String(http.errorToString(httpCode).c_str()));
  }

  http.end();
}

void sendStats() {
  String stats = "📊 Estadísticas de uso de memoria:\n";
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t estimatedTotalHeap = 50000;
  uint32_t usedHeap = estimatedTotalHeap - freeHeap;
  float usagePercent = (float)usedHeap / estimatedTotalHeap * 100;

  stats += "Memoria libre: " + String(freeHeap) + " bytes\n";
  stats += "Memoria utilizada: " + String(usedHeap) + " bytes (" + String(usagePercent, 1) + "%)";

  sendMessage(stats);
}

String urlencode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
      code0 = ((c >> 4) & 0xf) + '0';
      if (((c >> 4) & 0xf) > 9) code0 = ((c >> 4) & 0xf) - 10 + 'A';
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}
