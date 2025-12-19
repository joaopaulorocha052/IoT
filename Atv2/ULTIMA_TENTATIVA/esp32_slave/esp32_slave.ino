#include <Arduino.h>
#include <Wire.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth nao esta habilitado no menuconfig do ESP32!
#endif

BluetoothSerial SerialBT;

#define I2C_ADDR    0x09   // Endereco do Arduino local (diferente do Master!)
#define SDA_PIN     21
#define SCL_PIN     22
#define I2C_FREQ    100000 // 100kHz (mais estavel)

// Periodo de envio em ms
#define TELEMETRY_PERIOD_MS 2000UL  // 2 segundos

// Nome Bluetooth do Master
String masterName = "HUB_MASTER";
uint8_t address[6] {0xE0, 0x5A, 0x1B, 0x77, 0x4B, 0x2A};
bool isConnected = false;

bool i2cReadFloat(char cmd, float &outValue) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write((uint8_t)cmd);
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    Serial.printf("[I2C] endTransmission erro: %d\n", error);
    return false;
  }

  delay(10);  // Tempo para o Arduino processar

  const uint8_t numBytes = 4;
  uint8_t buf[numBytes];
  int received = Wire.requestFrom((uint8_t)I2C_ADDR, numBytes);
  
  if (received != numBytes) {
    Serial.printf("[I2C] requestFrom recebeu %d bytes (esperado %d)\n", received, numBytes);
    return false;
  }

  for (int i = 0; i < numBytes; ++i) {
    buf[i] = Wire.read();
  }
  memcpy(&outValue, buf, sizeof(float));
  return true;
}

// Scanner I2C
void scanI2C() {
  Serial.println(F("\n=== Scanner I2C ==="));
  byte devicesFound = 0;
  for (byte address = 8; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print(F("[ENCONTRADO] 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devicesFound++;
    }
  }
  if (devicesFound == 0) {
    Serial.println(F("[ERRO] Nenhum dispositivo I2C!"));
  }
  Serial.println(F("===================\n"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n========================================"));
  Serial.println(F("ESP32 SLAVE - Dispositivo-Padrao 2"));
  Serial.println(F("========================================"));
  
  // Inicializa I2C
  Serial.println(F("[I2C] Iniciando..."));
  Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQ);
  delay(100);
  
  scanI2C();

  // Testa conexao com Arduino local
  Wire.beginTransmission(I2C_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("[I2C] ERRO - Arduino nao encontrado!"));
  } else {
    Serial.println(F("[I2C] OK - Arduino conectado (0x09)"));
  }

  // Inicializa Bluetooth como CLIENTE (Slave)
  Serial.println(F("\n[BT] Iniciando Bluetooth..."));
  if (!SerialBT.begin("HUB_SLAVE", true)) {  // true = modo cliente
    Serial.println(F("[BT] ERRO ao iniciar Bluetooth!"));
    while(1) delay(1000);
  }
  
  Serial.println(F("[BT] OK - Nome: HUB_SLAVE"));
  Serial.print(F("[BT] Conectando ao Master: "));
  Serial.println(masterName);
  
  Serial.println(F("\n========================================\n"));
}

void loop() {
  // 1) Verifica conexao Bluetooth
  if (!SerialBT.connected()) {
    if (isConnected) {
      Serial.println(F("[BT] Desconectado do Master!"));
      isConnected = false;
    }
    
    static unsigned long lastRetry = 0;
    unsigned long now = millis();
    
    if (now - lastRetry > 5000UL) {
      lastRetry = now;
      Serial.println(F("[BT] Tentando conectar ao Master..."));
      
      if (SerialBT.connect(address)) {
        Serial.println(F("[BT] Conectado ao Master!"));
        isConnected = true;
      } else {
        Serial.println(F("[BT] Falha na conexao. Tentando novamente em 5s..."));
      }
    }
    
    delay(100);
    return;
  }

  // Se acabou de conectar
  if (!isConnected) {
    Serial.println(F("[BT] Conexao estabelecida!"));
    isConnected = true;
  }

  // 2) Envio periodico de dados para o Master
  static unsigned long lastSend = 0;
  unsigned long now = millis();

  if (now - lastSend >= TELEMETRY_PERIOD_MS) {
    lastSend = now;

    float t, u, d;
    bool okT = i2cReadFloat('a', t);
    delay(20);
    bool okU = i2cReadFloat('b', u);
    delay(20);
    bool okD = i2cReadFloat('c', d);

    if (okT && okU && okD) {
      char msg[80];
      snprintf(msg, sizeof(msg), "T=%.3f;U=%.3f;D=%.3f\n", t, u, d);

      SerialBT.print(msg);
      Serial.print(F("[BT] Enviado: "));
      Serial.print(msg);
    } else {
      Serial.println(F("[I2C] Falha ao ler sensores!"));
      if (!okT) Serial.println(F("  - Temperatura falhou"));
      if (!okU) Serial.println(F("  - Umidade falhou"));
      if (!okD) Serial.println(F("  - Distancia falhou"));
    }
  }

  delay(10);
}