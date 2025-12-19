// === ESP32 (Mestre I2C) ===
// Abre um menu no Serial e pede ao Arduino as leituras requisitadas.

#include <Wire.h>

#define I2C_ADDR     1
#define SDA_PIN      4
#define SCL_PIN      5

char readUserOption() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    // normaliza para minúsculo
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    if (c == 'a' || c == 'b' || c == 'c') return c;
  }
  return 0;
}

bool requestFloat(float &outValue) {
  // solicita 4 bytes
  const uint8_t bytes = 4;
  uint8_t buf[bytes];
  int n = Wire.requestFrom(I2C_ADDR, (int)bytes);
  if (n != bytes) return false;
  for (int i = 0; i < bytes; ++i) buf[i] = Wire.read();
  memcpy(&outValue, buf, sizeof(float));
  return true;
}

void printMenu() {
  Serial.println();
  Serial.println(F("===== Central de Sensores (I2C) ====="));
  Serial.println(F("(a) Temperatura (°C)"));
  Serial.println(F("(b) Umidade (%)"));
  Serial.println(F("(c) Distância (cm)"));
  Serial.print(F("Escolha: "));
}

void setup() {
  Serial.begin(115200);
  // Inicia I2C nos pinos padrão do ESP32
  Wire.begin(SDA_PIN, SCL_PIN, 400000); // 400 kHz (pode reduzir p/ 100kHz se houver ruído)
  delay(200);
  printMenu();
}

void loop() {
  char opt = readUserOption();
  if (opt) {
    // Envia o comando ao escravo
    Wire.beginTransmission(I2C_ADDR);
    Wire.write((uint8_t)opt);
    Wire.endTransmission();

    // Aguarda o escravo medir (DHT11 é lento)
    delay(350); // ajuste fino: 300~400 ms

    float value;
    bool ok = requestFloat(value);

    Serial.println();
    switch (opt) {
      case 'a': Serial.print(F("[Temperatura] ")); break;
      case 'b': Serial.print(F("[Umidade] "));     break;
      case 'c': Serial.print(F("[Distância] "));   break;
    }

    if (!ok) {
      Serial.println(F("Falha ao receber dados (I2C)."));
    } else if (isnan(value)) {
      Serial.println(F("Leitura inválida (NaN)."));
    } else {
      // 3 casas decimais
      Serial.printf("%.3f\n", value);
    }

    printMenu();
  }

  // pequeno respiro pro loop
  delay(10);
}
