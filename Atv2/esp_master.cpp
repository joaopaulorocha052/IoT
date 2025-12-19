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

    return (ok_t && ok_u && ok_d);
}

const int TAMANHO_BUFFER = 10;
float buffer_temperatura[TAMANHO_BUFFER];
float buffer_umidade[TAMANHO_BUFFER];
float buffer_distancia[TAMANHO_BUFFER];
int indice_temp = 0, indice_umid = 0, indice_dist = 0;
int contagem_temp = 0, contagem_umid = 0, contagem_dist = 0;


BluetoothSerial SerialBT;
String slaveName = "ESP32_Slave";
bool btConectado = false;

float local_temp, local_umid, local_dist;
float remote_temp, remote_umid, remote_dist;
bool new_local_data = false;
bool new_remote_data = false;

unsigned long lastLocalRead = 0;
const long localReadInterval = 100; // Sincroniza com o Slave


void conectarBluetooth() {
    Serial.print("Conectando ao "); Serial.println(slaveName);
    btConectado = SerialBT.connect(slaveName);
    if(btConectado) {
        Serial.println("Conectado!");
    } else {
        Serial.println("Falha ao conectar.");
    }
}

void printMenu() {
  Serial.println();
  Serial.println(F("===== Central de Sensores (I2C) ====="));
  Serial.println(F("(a) Temperatura (°C)"));
  Serial.println(F("(b) Umidade (%)"));
  Serial.println(F("(c) Distância (cm)"));
  Serial.print(F("Escolha: "));
}

float calcularMediaDoBuffer(float* buff, int contagem_elemento){
    if (contagem_elemento == 0) return NAN;
    float soma = 0.0;
    for (int i = 0; i < contagem_elemento; ++i) {
        soma += buff[i];
    }
    return soma / contagem_elemento;
}

void adicionarDado(float valor, float* buff, int &indice, int &contagem_elemento) {
    buff[indice] = valor;
    indice = (indice + 1) % TAMANHO_BUFFER;
    if (contagem_elemento < TAMANHO_BUFFER) {
        contagem_elemento++;
    }
}


void handleSerialMenu() {
    if (Serial.available()) {
        char opt = (char)Serial.read();

        /* opt = (opt)d - 65d (A) + 97d (a)
        (A) sendo o primeiro caractere do alfabeto ASCII e contendo todos os demais caracteres
        faz com que a soma ao 97(seu minúsculo), resultando no caractere minúsculo correspondente (option).
        */
        if (opt >= 'A' && opt <= 'Z') opt = opt - 'A' + 'a';

        float media_calculada = 0.0;
        switch (opt) {
            case 'a':
                media_calculada = calcularMediaDoBuffer(buffer_temperatura, contagem_temp);
                Serial.printf("[Média Temp] %.3f\n", media_calculada);
                break;
            case 'b':
                media_calculada = calcularMediaDoBuffer(buffer_umidade, contagem_umid);
                Serial.printf("[Média Umid] %.3f\n", media_calculada);
                break;
            case 'c':
                media_calculada = calcularMediaDoBuffer(buffer_distancia, contagem_dist);
                Serial.printf("[Média Dist] %.3f\n", media_calculada);
                break;
        }
        printMenu();
    }
}

void handleBluetooth() {
    if (!btConectado) return;
    
    if (SerialBT.available()) {
        String dataIn = SerialBT.readStringUntil('\n');
        dataIn.trim();

        if (sscanf(dataIn.c_str(), "%f,%f,%f", &remote_temp, &remote_umid, &remote_dist) == 3) {
            new_remote_data = true;
        } else {
            Serial.print("BT Erro: "); Serial.println(dataIn);
        }
    }
}

void handleLocalData() {
    if (millis() - lastLocalRead >= localReadInterval) {
        lastLocalRead = millis();
        
        if (getAllLocalData(local_temp, local_umid, local_dist)) {
            new_local_data = true;
        } else {
            Serial.println("Falha ao ler dados I2C locais.");
        }
    }
}

void processData() {
    if (new_local_data && new_remote_data) {
        float T_media = (local_temp + remote_temp) / 2.0;
        float U_media = (local_umid + remote_umid) / 2.0;
        float D_media = (local_dist + remote_dist) / 2.0;

        adicionarDado(T_media, buffer_temperatura, indice_temp, contagem_temp);
        adicionarDado(U_media, buffer_umidade, indice_umid, contagem_umid);
        adicionarDado(D_media, buffer_distancia, indice_dist, contagem_dist);

        new_local_data = false;
        new_remote_data = false;
        
        Serial.printf("Dados processados: T=%.3f, U=%.3f, D=%.3f\n", T_media, U_media, D_media);
    }
}


void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN, 400000);
    SerialBT.begin("ESP32_Master");
    
    conectarBluetooth();
    delay(200);
    printMenu();
}

void loop() {
    if (!btConectado) {
        conectarBluetooth();
        delay(1000);
    }

    handleSerialMenu();
    handleBluetooth();
    handleLocalData();
    processData();
    
}