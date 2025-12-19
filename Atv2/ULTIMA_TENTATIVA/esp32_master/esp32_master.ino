// esp32_master.ino - ESP32 Master (Dispositivo-Padrão 1)
// Comunica via I2C com Arduino (0x08) e via Bluetooth com ESP32 Slave

#include <Arduino.h>
#include <Wire.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth nao esta habilitado no menuconfig do ESP32!
#endif

BluetoothSerial SerialBT;

#define I2C_ADDR   0x08   // Endereco do Arduino local
#define SDA_PIN    21     // ESP32: GPIO21
#define SCL_PIN    22     // ESP32: GPIO22
#define I2C_FREQ   100000 // 100kHz (mais estavel)

// Buffer de ate 10 amostras (media dos dois dispositivos)
#define BUFFER_SIZE 10

float bufferT[BUFFER_SIZE];
float bufferU[BUFFER_SIZE];
float bufferD[BUFFER_SIZE];
int   bufIndex     = 0;
int   sampleCount  = 0;

bool btConnected = false;

// Le um float do Arduino local via I2C
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

// Le opcao do usuario no Serial
char readUserOption() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    if (c == 'a' || c == 'b' || c == 'c' || c == 's') return c;
  }
  return 0;
}

void printMenu() {
  Serial.println();
  Serial.println(F("===== HUB DE SENSORES - MASTER ====="));
  Serial.println(F("(a) Temperatura (media ultimos 10)"));
  Serial.println(F("(b) Umidade (media ultimos 10)"));
  Serial.println(F("(c) Distancia (media ultimos 10)"));
  Serial.println(F("(s) Status da conexao"));
  Serial.print(F("Escolha: "));
}

float calcMedia(const float *buf, int n) {
  if (n <= 0) return NAN;
  float soma = 0.0f;
  int valid = 0;
  for (int i = 0; i < n; ++i) {
    if (!isnan(buf[i])) {
      soma += buf[i];
      valid++;
    }
  }
  return (valid > 0) ? (soma / (float)valid) : NAN;
}

void addSample(float Tmed, float Umed, float Dmed) {
  bufferT[bufIndex] = Tmed;
  bufferU[bufIndex] = Umed;
  bufferD[bufIndex] = Dmed;

  bufIndex = (bufIndex + 1) % BUFFER_SIZE;
  if (sampleCount < BUFFER_SIZE) {
    sampleCount++;
  }
}

// Parse da string do Slave: "T=xx.xxx;U=yy.yyy;D=zz.zzz"
bool parseSlaveMessage(const String &msg, float &T, float &U, float &D) {
  const char *cstr = msg.c_str();
  float t, u, d;
  int matched = sscanf(cstr, "T=%f;U=%f;D=%f", &t, &u, &d);
  if (matched == 3) {
    T = t; U = u; D = d;
    return true;
  }
  return false;
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
  Serial.println(F("ESP32 MASTER - Dispositivo-Padrao 1"));
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
    Serial.println(F("[I2C] OK - Arduino conectado (0x08)"));
  }

  // Inicializa Bluetooth como SERVIDOR (Master)
  Serial.println(F("\n[BT] Iniciando Bluetooth como servidor..."));
  if (!SerialBT.begin("HUB_MASTER")) {
    Serial.println(F("[BT] ERRO ao iniciar Bluetooth!"));
  } else {
    Serial.println(F("[BT] OK - Nome: HUB_MASTER"));
    Serial.println(F("[BT] Aguardando conexao do Slave..."));
  }

  Serial.println(F("\n========================================\n"));
  printMenu();
}

void loop() {
  // 1) Verifica conexao Bluetooth
  if (SerialBT.hasClient()) {
    if (!btConnected) {
      Serial.println(F("\n[BT] Slave conectado!"));
      btConnected = true;
      printMenu();
    }
  } else {
    if (btConnected) {
      Serial.println(F("\n[BT] Slave desconectado!"));
      btConnected = false;
    }
  }

  // 2) Processa dados recebidos do Slave via Bluetooth
  if (SerialBT.available()) {
    String msg = SerialBT.readStringUntil('\n');
    msg.trim();
    
    if (msg.length() > 0) {
      float Ts, Us, Ds;
      if (parseSlaveMessage(msg, Ts, Us, Ds)) {
        // Le dados locais do Arduino (Master)
        float Tm, Um, Dm;
        bool okT = i2cReadFloat('a', Tm);
        delay(20);
        bool okU = i2cReadFloat('b', Um);
        delay(20);
        bool okD = i2cReadFloat('c', Dm);

        Serial.println();
        Serial.println(F("=== Dados Recebidos ==="));
        Serial.printf("  Local (Master): T=%.2f  U=%.2f  D=%.2f\n", Tm, Um, Dm);
        Serial.printf("  Remoto (Slave): T=%.2f  U=%.2f  D=%.2f\n", Ts, Us, Ds);

        if (okT && okU && okD && !isnan(Ts) && !isnan(Us) && !isnan(Ds)) {
          float Tmed = (Tm + Ts) / 2.0f;
          float Umed = (Um + Us) / 2.0f;
          float Dmed = (Dm + Ds) / 2.0f;

          addSample(Tmed, Umed, Dmed);

          Serial.printf("  Media         : T=%.2f  U=%.2f  D=%.2f\n", Tmed, Umed, Dmed);
          Serial.printf("  Amostras: %d/%d\n", sampleCount, BUFFER_SIZE);
        } else {
          Serial.println(F("  [AVISO] Dados locais invalidos, usando apenas remoto"));
          if (!isnan(Ts) && !isnan(Us) && !isnan(Ds)) {
            addSample(Ts, Us, Ds);
          }
        }
        Serial.println(F("======================="));
      } else {
        Serial.print(F("[BT] Mensagem invalida: "));
        Serial.println(msg);
      }
    }
  }

  // 3) Leitura local periodica (debug)
  static unsigned long lastLocalRead = 0;
  if (millis() - lastLocalRead >= 5000UL) {
    lastLocalRead = millis();
    
    float Tm, Um, Dm;
    bool okT = i2cReadFloat('a', Tm);
    delay(20);
    bool okU = i2cReadFloat('b', Um);
    delay(20);
    bool okD = i2cReadFloat('c', Dm);
    
    if (okT && okU && okD) {
      Serial.println();
      Serial.println(F("--- Leitura Local (I2C) ---"));
      Serial.printf("  T=%.2f C  U=%.2f %%  D=%.2f cm\n", Tm, Um, Dm);
      Serial.printf("  BT: %s\n", btConnected ? "Conectado" : "Aguardando");
    }
  }

  // 4) Menu do usuario
  char opt = readUserOption();
  if (opt) {
    Serial.println();
    
    if (opt == 's') {
      Serial.println(F("=== Status ==="));
      Serial.printf("  I2C: Arduino 0x%02X\n", I2C_ADDR);
      Serial.printf("  Bluetooth: %s\n", btConnected ? "Slave conectado" : "Aguardando");
      Serial.printf("  Amostras: %d/%d\n", sampleCount, BUFFER_SIZE);
      printMenu();
      return;
    }
    
    int n = sampleCount;
    if (n == 0) {
      Serial.println(F("Ainda nao ha dados para calcular media."));
    } else {
      float media = NAN;
      const char* tipo = "";
      switch (opt) {
        case 'a': media = calcMedia(bufferT, n); tipo = "Temperatura"; break;
        case 'b': media = calcMedia(bufferU, n); tipo = "Umidade"; break;
        case 'c': media = calcMedia(bufferD, n); tipo = "Distancia"; break;
      }

      Serial.printf("[%s] Media dos ultimos %d valores: ", tipo, n);
      if (isnan(media)) {
        Serial.println(F("NaN"));
      } else {
        Serial.printf("%.2f\n", media);
      }
    }
    printMenu();
  }

  delay(10);
}
