// esp32_master_hub.cpp

#include <Arduino.h>
#include <Wire.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth não está habilitado no menuconfig do ESP32!
#endif

BluetoothSerial SerialBT;

#define I2C_ADDR   0x08
#define SDA_PIN    21
#define SCL_PIN    22

// Buffer de até 10 amostras
#define BUFFER_SIZE 10

float bufferT[BUFFER_SIZE];
float bufferU[BUFFER_SIZE];
float bufferD[BUFFER_SIZE];
int   bufIndex     = 0;   // próxima posição para escrever
int   sampleCount  = 0;   // quantas amostras válidas existem (até 10)

// Lê um float do Mega usando o comando ('a', 'b', 'c')
bool i2cReadFloat(char cmd, float &outValue) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write((uint8_t)cmd);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(5); // pequeno delay só para sincronizar

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

// Lê opção do usuário no Serial (a, b, c em minúsculo)
char readUserOption() {
  if (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    if (c == 'a' || c == 'b' || c == 'c') return c;
  }
  return 0;
}

void printMenu() {
  Serial.println();
  Serial.println(F("===== HUB DE SENSORES - MASTER ====="));
  Serial.println(F("(a) Temperatura (media ultimos 10)"));
  Serial.println(F("(b) Umidade (media ultimos 10)"));
  Serial.println(F("(c) Distancia (media ultimos 10)"));
  Serial.print(F("Escolha: "));
}

// Calcula média de um buffer de tamanho N (N <= BUFFER_SIZE)
float calcMedia(const float *buf, int n) {
  if (n <= 0) return NAN;
  float soma = 0.0f;
  for (int i = 0; i < n; ++i) {
    soma += buf[i];
  }
  return soma / (float)n;
}

// Adiciona uma nova amostra média T/U/D nos buffers circulares
void addSample(float Tmed, float Umed, float Dmed) {
  bufferT[bufIndex] = Tmed;
  bufferU[bufIndex] = Umed;
  bufferD[bufIndex] = Dmed;

  bufIndex = (bufIndex + 1) % BUFFER_SIZE;
  if (sampleCount < BUFFER_SIZE) {
    sampleCount++;
  }
}

// Faz o parse da string vinda do Slave: "T=xx.xxx;U=yy.yyy;D=zz.zzz"
bool parseSlaveMessage(const String &msg, float &T, float &U, float &D) {
  // Uso de sscanf para facilitar
  const char *cstr = msg.c_str();
  float t, u, d;
  int matched = sscanf(cstr, "T=%f;U=%f;D=%f", &t, &u, &d);
  if (matched == 3) {
    T = t; U = u; D = d;
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println(F("ESP32 Master - Hub de Sensores"));

  Serial.println(F("Iniciando I2C..."));
  Wire.begin(SDA_PIN, SCL_PIN, 400000);

  Serial.println(F("Iniciando Bluetooth como servidor..."));
  SerialBT.begin("HUB_MASTER"); // nome visivel p/ o Slave

  Serial.println(F("Aguardando conexao do Slave via Bluetooth..."));
  printMenu();
}

void loop() {
  // 1) Processar dados recebidos do Slave via Bluetooth
  if (SerialBT.available()) {
    // Le ate uma linha completa (finalizada com '\n')
    String msg = SerialBT.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      float Ts, Us, Ds;
      if (parseSlaveMessage(msg, Ts, Us, Ds)) {
        // Le dados locais do Mega (Master)
        float Tm, Um, Dm;
        bool okT = i2cReadFloat('a', Tm);
        bool okU = i2cReadFloat('b', Um);
        bool okD = i2cReadFloat('c', Dm);

        if (okT && okU && okD && !isnan(Ts) && !isnan(Us) && !isnan(Ds)) {
          float Tmed = (Tm + Ts) / 2.0f;
          float Umed = (Um + Us) / 2.0f;
          float Dmed = (Dm + Ds) / 2.0f;

          addSample(Tmed, Umed, Dmed);

          // Debug opcional: mostra amostras brutas e medias
          Serial.println();
          Serial.println(F("=== Nova amostra recebida do Slave ==="));
          Serial.printf("Master: T=%.3f  U=%.3f  D=%.3f\n", Tm, Um, Dm);
          Serial.printf("Slave : T=%.3f  U=%.3f  D=%.3f\n", Ts, Us, Ds);
          Serial.printf("Media : T=%.3f  U=%.3f  D=%.3f\n", Tmed, Umed, Dmed);
          Serial.printf("Amostras armazenadas: %d (max %d)\n", sampleCount, BUFFER_SIZE);
          printMenu();
        } else {
          Serial.println(F("Falha ao ler do Mega ou dados do Slave invalidos (NaN)."));
        }
      } else {
        Serial.print(F("Mensagem do Slave invalida: "));
        Serial.println(msg);
      }
    }
  }

  // 2) Ler escolha do usuario no Serial (menu)
  char opt = readUserOption();
  if (opt) {
    Serial.println(); // pular linha
    int n = sampleCount;
    if (n == 0) {
      Serial.println(F("Ainda nao ha dados suficientes para calcular media."));
    } else {
      float media = NAN;
      switch (opt) {
        case 'a':
          media = calcMedia(bufferT, n);
          Serial.print(F("[Temperatura] Media dos ultimos "));
          Serial.print(n);
          Serial.print(F(" valores: "));
          break;
        case 'b':
          media = calcMedia(bufferU, n);
          Serial.print(F("[Umidade] Media dos ultimos "));
          Serial.print(n);
          Serial.print(F(" valores: "));
          break;
        case 'c':
          media = calcMedia(bufferD, n);
          Serial.print(F("[Distancia] Media dos ultimos "));
          Serial.print(n);
          Serial.print(F(" valores: "));
          break;
      }

      if (isnan(media)) {
        Serial.println(F("NaN (dados invalidos)."));
      } else {
        Serial.printf("%.3f\n", media);  // 3 casas decimais
      }
    }

    printMenu();
  }

  // Pequena folga no loop
  delay(5);
}
