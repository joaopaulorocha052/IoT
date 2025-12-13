/*
 * ============================================================================
 * IMPLEMENTAÇÃO DO MÓDULO DE SENSORES
 * ============================================================================
 */

#include "sensors.h"
#include "config.h"

// ============================================================================
// IMPLEMENTAÇÃO - SENSOR ULTRASSÔNICO
// ============================================================================

SensorUltrassonico::SensorUltrassonico(int pinTrig, int pinEcho, int numAmostras) {
    _pinTrig = pinTrig;
    _pinEcho = pinEcho;
    _numAmostras = numAmostras;
    _indiceAmostra = 0;
    _contadorAmostras = 0;
    _distanciaAtual = 0;
    _distanciaMedia = 0;
    _leituraValida = false;
    
    // Aloca array para amostras
    _amostras = new float[_numAmostras];
    for (int i = 0; i < _numAmostras; i++) {
        _amostras[i] = 0;
    }
}

void SensorUltrassonico::inicializar() {
    pinMode(_pinTrig, OUTPUT);
    pinMode(_pinEcho, INPUT);
    digitalWrite(_pinTrig, LOW);
    
    Serial.println("[SENSOR] Ultrassônico inicializado");
    Serial.print("[SENSOR] Pinos - TRIG: ");
    Serial.print(_pinTrig);
    Serial.print(", ECHO: ");
    Serial.println(_pinEcho);
}

void SensorUltrassonico::atualizar() {
    _distanciaAtual = lerDistancia();
    
    // Verifica se a leitura está dentro dos limites válidos
    if (_distanciaAtual >= DISTANCIA_MIN && _distanciaAtual <= DISTANCIA_MAX) {
        _leituraValida = true;
        
        // Adiciona ao buffer circular
        _amostras[_indiceAmostra] = _distanciaAtual;
        _indiceAmostra = (_indiceAmostra + 1) % _numAmostras;
        
        if (_contadorAmostras < _numAmostras) {
            _contadorAmostras++;
        }
        
        // Calcula média quando tiver amostras suficientes
        calcularMedia();
    } else {
        _leituraValida = false;
    }
}

float SensorUltrassonico::lerDistancia() {
    // Gera pulso ultrassônico
    digitalWrite(_pinTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(_pinTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(_pinTrig, LOW);
    
    // Mede duração do eco (timeout de 30ms)
    long duracao = pulseIn(_pinEcho, HIGH, 30000);
    
    // Calcula distância em cm
    if (duracao == 0) {
        return DISTANCIA_MAX + 1; // Retorna valor inválido
    }
    
    return duracao * SOUND_SPEED / 2.0;
}

void SensorUltrassonico::calcularMedia() {
    if (_contadorAmostras == 0) {
        _distanciaMedia = 0;
        return;
    }
    
    float soma = 0;
    int amostrasValidas = min(_contadorAmostras, _numAmostras);
    
    for (int i = 0; i < amostrasValidas; i++) {
        soma += _amostras[i];
    }
    
    _distanciaMedia = soma / amostrasValidas;
}

DadosDistancia SensorUltrassonico::getDados() {
    DadosDistancia dados;
    dados.distanciaAtual = _distanciaAtual;
    dados.distanciaMedia = _distanciaMedia;
    dados.leituraValida = _leituraValida;
    return dados;
}

float SensorUltrassonico::getDistanciaMedia() {
    return _distanciaMedia;
}

float SensorUltrassonico::getDistanciaAtual() {
    return _distanciaAtual;
}

bool SensorUltrassonico::isLeituraValida() {
    return _leituraValida;
}

// ============================================================================
// IMPLEMENTAÇÃO - MPU-6050
// ============================================================================

SensorIMU::SensorIMU(uint8_t endereco) {
    _endereco = endereco;
    _dados.leituraValida = false;
}

bool SensorIMU::inicializar() {
    Wire.begin();
    
    // Acorda o MPU-6050 (sai do modo sleep)
    Wire.beginTransmission(_endereco);
    Wire.write(0x6B);  // Registrador PWR_MGMT_1
    Wire.write(0);     // Seta para zero (acorda o MPU-6050)
    byte erro = Wire.endTransmission(true);
    
    if (erro != 0) {
        Serial.println("[SENSOR] Erro ao inicializar MPU-6050!");
        Serial.print("[SENSOR] Código de erro: ");
        Serial.println(erro);
        return false;
    }
    
    Serial.println("[SENSOR] MPU-6050 inicializado");
    Serial.print("[SENSOR] Endereço I2C: 0x");
    Serial.println(_endereco, HEX);
    
    _dados.leituraValida = true;
    return true;
}

void SensorIMU::atualizar() {
    lerDadosRaw();
}

void SensorIMU::lerDadosRaw() {
    Wire.beginTransmission(_endereco);
    Wire.write(0x3B);  // Começa no registrador ACCEL_XOUT_H
    Wire.endTransmission(false);
    
    // Requisita 14 bytes (7 registradores de 16 bits)
    Wire.requestFrom(_endereco, (uint8_t)14, (uint8_t)true);
    
    if (Wire.available() < 14) {
        _dados.leituraValida = false;
        return;
    }
    
    // Lê acelerômetro
    _dados.accelX = Wire.read() << 8 | Wire.read();
    _dados.accelY = Wire.read() << 8 | Wire.read();
    _dados.accelZ = Wire.read() << 8 | Wire.read();
    
    // Lê temperatura
    int16_t tempRaw = Wire.read() << 8 | Wire.read();
    _dados.temperatura = tempRaw / 340.00 + 36.53;
    
    // Lê giroscópio
    _dados.gyroX = Wire.read() << 8 | Wire.read();
    _dados.gyroY = Wire.read() << 8 | Wire.read();
    _dados.gyroZ = Wire.read() << 8 | Wire.read();
    
    _dados.leituraValida = true;
}

DadosIMU SensorIMU::getDados() {
    return _dados;
}

bool SensorIMU::isLeituraValida() {
    return _dados.leituraValida;
}

String SensorIMU::getJsonInclinacao() {
    String json = "{";
    json += "\"ax\":" + String(_dados.accelX) + ",";
    json += "\"ay\":" + String(_dados.accelY) + ",";
    json += "\"az\":" + String(_dados.accelZ) + ",";
    json += "\"gx\":" + String(_dados.gyroX) + ",";
    json += "\"gy\":" + String(_dados.gyroY) + ",";
    json += "\"gz\":" + String(_dados.gyroZ) + ",";
    json += "\"temp\":" + String(_dados.temperatura, 1);
    json += "}";
    return json;
}
