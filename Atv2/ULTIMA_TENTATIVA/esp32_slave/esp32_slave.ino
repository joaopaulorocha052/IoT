// esp32_slave_hub.cpp

#include <Arduino.h>
#include <Wire.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth não está habilitado no menuconfig do ESP32!
#endif

BluetoothSerial SerialBT;

#define I2C_ADDR    0x08
#define SDA_PIN     21
#define SCL_PIN     22

// Período de envio em ms (atividade pede 100 ms)
#define TELEMETRY_PERIOD_MS 100UL

// Nome Bluetooth do Master ao qual vamos conectar
const char* MASTER_BT_NAME = "HUB_MASTER";

bool i2cReadFloat(char cmd, float &outValue) {
  // Diz ao Mega qual grandeza queremos
  Wire.beginTransmission(I2C_ADDR);
  Wire.write((uint8_t)cmd);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  // Pequeno delay para o Mega atualizar a seleção (não é leitura pesada)
  delay(5);

  const uint8_t numBytes = 4;
  uint8_t buf[numBytes];
  int received = Wire.requestFrom(I2C_ADDR, (int)numBytes);
  if (received != numBytes) {
    return false;
  }

  for (int i = 0; i < numBytes; ++i) {
    buf[i] = Wire.read();
  }
  memcpy(&outValue, buf, sizeof(float));
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println(F("ESP32 Slave - Hub de Sensores"));
  Serial.println(F("Iniciando I2C..."));
  Wire.begin(SDA_PIN, SCL_PIN, 400000);

  Serial.println(F("Iniciando Bluetooth em modo cliente..."));
  SerialBT.begin("HUB_SLAVE", true); // nome local, modo cliente

  Serial.print(F("Conectando ao Master ("));
  Serial.print(MASTER_BT_NAME);
  Serial.println(F(")..."));

  bool connected = SerialBT.connect(MASTER_BT_NAME);
  if (!connected) {
    Serial.println(F("Falha inicial ao conectar. Tentando reconectar em loop..."));
  } else {
    Serial.println(F("Conectado ao Master!"));
  }
}

void loop() {
  // Garante que haja conexão Bluetooth
  if (!SerialBT.connected()) {
    static unsigned long lastRetry = 0;
    unsigned long now = millis();
    if (now - lastRetry > 2000UL) {
      lastRetry = now;
      Serial.println(F("Tentando reconectar ao Master..."));
      SerialBT.connect(MASTER_BT_NAME);
    }
    delay(100);
    return;
  }

  static unsigned long lastSend = 0;
  unsigned long now = millis();

  if (now - lastSend >= TELEMETRY_PERIOD_MS) {
    lastSend = now;

    float t, u, d;
    bool okT = i2cReadFloat('a', t);
    bool okU = i2cReadFloat('b', u);
    bool okD = i2cReadFloat('c', d);

    if (okT && okU && okD) {
      char msg[80];
      // Sempre 3 casas decimais
      snprintf(msg, sizeof(msg), "T=%.3f;U=%.3f;D=%.3f\n", t, u, d);

      SerialBT.print(msg);   // envia ao Master
      Serial.print(F("Enviado ao Master: "));
      Serial.print(msg);     // debug serial local
    } else {
      Serial.println(F("Falha ao ler sensores via I2C (Slave)."));
    }
  }

  // Nada impede de processar alguma coisa de retorno via Bluetooth, se quiser
}