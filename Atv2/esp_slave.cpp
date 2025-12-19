#include <Wire.h>
#include "BluetoothSerial.h"


#define I2C_ADDR     0x08
#define SDA_PIN      21
#define SCL_PIN      22

bool requestFloat(float &outValue) {
  const uint8_t bytes = 4;
  uint8_t buf[bytes];
  int n = Wire.requestFrom(I2C_ADDR, (int)bytes);
  if (n != bytes) return false;
  for (int i = 0; i < bytes; ++i) buf[i] = Wire.read();
  memcpy(&outValue, buf, sizeof(float));
  return true;
}


bool getAllLocalData(float &outTemp, float &outUmid, float &outDist) {
    bool ok_t, ok_u, ok_d;

    // a
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)'a');
    Wire.endTransmission();
    delay(350); // Delay para o DHT11
    ok_t = requestFloat(outTemp);

    // b
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)'b');
    Wire.endTransmission();
    delay(350); // Delay para o DHT11
    ok_u = requestFloat(outUmid);

    // c
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)'c');
    Wire.endTransmission();
    delay(100); // Delay menor para o HC-SR04
    ok_d = requestFloat(outDist);

    // Dessa forma, somente quando os três sensores forem lidos com sucesso, o sistema considerará a operação como bem-sucedida.
    return (ok_t && ok_u && ok_d);
}

BluetoothSerial SerialBT;
String slaveName = "ESP32_Slave_João&Fernando";

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN, 400000);
    SerialBT.begin(slaveName);
    Serial.println("ESP32 Slave pronto para parear.");
}

void loop() {
    float temp, umid, dist;

    // 1. Coleta os dados locais
    if (getAllLocalData(temp, umid, dist)) {
        
        // 2. Formata os dados (ex: "25.123,45.456,10.789")
        char buffer[80];
        // Formata com 3 casas decimais
        sprintf(buffer, "%.3f,%.3f,%.3f", temp, umid, dist); 
        
        // 3. Envia via Bluetooth
        SerialBT.println(buffer);
        Serial.print("Enviado: "); Serial.println(buffer);

    } else {
        Serial.println("Falha ao ler dados I2C locais.");
        SerialBT.println("ERRO"); // Informa o master
    }

    // 4. Espera 100ms para o próximo envio 
    delay(100); 
}