#include <EEPROM.h>
#include <ESP8266WiFi.h>

#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define PASS_ADDR 100
#define MAX_SSID_LENGTH 32
#define MAX_PASS_LENGTH 64

const char* ssid = "TuSSID";  // SSID de tu red WiFi
const char* password = "TuContraseña";  // Contraseña de tu red WiFi

void setup() {
  Serial.begin(115200);
  delay(500);

  EEPROM.begin(EEPROM_SIZE);

  conectarWiFiDesdeEEPROM();
  mostrarMenuWiFi();  // Mostrar el menú principal en el Monitor Serial
}

void loop() {
  // Aquí no hacemos nada en el loop, todo se maneja a través del menú
}

// ======= FUNCIONES PRINCIPALES =======

// Conectar WiFi usando datos de EEPROM
void conectarWiFiDesdeEEPROM() {
  String ssid = leerEEPROM(SSID_ADDR);
  String password = leerEEPROM(PASS_ADDR);

  if (ssid.length() == 0 || ssid.length() > MAX_SSID_LENGTH) {
    Serial.println("⚠️ SSID no encontrado o inválido. Debes configurarlo.");
    mostrarMenuWiFi();
    return;
  }

  Serial.println("\n📡 Intentando conectar...");
  Serial.print("🔗 Red objetivo: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ ¡Conectado exitosamente!");
    Serial.print("📡 IP asignada: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ No se pudo conectar en 20 segundos.");
    mostrarMenuWiFi();
  }
}

// Mostrar menú principal
void mostrarMenuWiFi() {
  Serial.println("\n======== MENÚ PRINCIPAL ========");
  Serial.print("[  Red actual: ");
  Serial.print(WiFi.SSID());
  Serial.println(" ]");
  Serial.println("1. 📡 Escanear redes WiFi");
  Serial.println("2. 🔧 Cambiar WiFi");
  Serial.println("3. 🔵 Buscar dispositivos Bluetooth (futuro)");
  Serial.println("=================================");
  
  Serial.print("⏳ Ingresa opción: ");

  while (!Serial.available()) {
    delay(10);
  }
  
  char opcion = Serial.read();
  Serial.readStringUntil('\n'); // Limpieza de buffer

  switch (opcion) {
    case '1':
      escanearRedesWiFi();
      break;
    case '2':
      cambiarWiFiYGuardar();
      break;
    default:
      Serial.println("\n⚠️ Opción inválida, regresando al menú.");
      mostrarMenuWiFi();
      break;
  }
}

// Escanear redes disponibles
void escanearRedesWiFi() {
  Serial.println("\n🔍 Escaneando redes WiFi...");
  int cantidad = WiFi.scanNetworks();
  
  if (cantidad == 0) {
    Serial.println("❌ No se encontraron redes disponibles.");
  } else {
    Serial.println("📋 Redes encontradas:");
    for (int i = 0; i < cantidad; ++i) {
      Serial.printf("%d. %s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
  }
  mostrarMenuWiFi();
}

// Cambiar y guardar nuevos datos de WiFi
void cambiarWiFiYGuardar() {
  Serial.println("\n🔧 CAMBIAR CONFIGURACIÓN WIFI 🔧");

  Serial.print("📝 Ingrese nuevo SSID: ");
  String nuevoSSID = leerCadenaSerial();

  Serial.print("🔑 Ingrese nueva contraseña: ");
  String nuevaPassword = leerCadenaSerial();

  escribirEEPROM(SSID_ADDR, nuevoSSID);
  escribirEEPROM(PASS_ADDR, nuevaPassword);

  Serial.println("\n💾 Nuevos datos guardados exitosamente en EEPROM.");
  delay(500);

  Serial.println("\n🔙 Volviendo al menú principal...");
  mostrarMenuWiFi(); // Volver al menú en vez de reconectar directamente
}

// ======= FUNCIONES AUXILIARES =======

// Leer texto desde Serial
String leerCadenaSerial() {
  String cadena = "";
  while (cadena.length() == 0) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') continue;
      cadena += c;
    }
    delay(10);
  }
  return cadena;
}

// Escribir String en EEPROM
void escribirEEPROM(int direccion, String texto) {
  for (int i = 0; i < texto.length(); ++i) {
    EEPROM.write(direccion + i, texto[i]);
  }
  EEPROM.write(direccion + texto.length(), '\0'); // Terminar cadena
  EEPROM.commit();
}

// Leer String de EEPROM
String leerEEPROM(int direccion) {
  String texto = "";
  char c;
  while ((c = EEPROM.read(direccion)) != '\0' && texto.length() < 128) {
    texto += c;
    direccion++;
  }
  return texto;
}
