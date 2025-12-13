/*
 * ============================================================================
 * MÓDULO DE SENSORES
 * ============================================================================
 * Gerencia a leitura do sensor ultrassônico HC-SR04 e do giroscópio MPU-6050.
 * ============================================================================
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// ESTRUTURAS DE DADOS
// ============================================================================

// Dados do sensor ultrassônico
struct DadosDistancia {
    float distanciaAtual;       // Última leitura (cm)
    float distanciaMedia;       // Média das últimas leituras (cm)
    bool leituraValida;         // Indica se a leitura é válida
};

// Dados do MPU-6050
struct DadosIMU {
    // Acelerômetro (valores raw)
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    
    // Giroscópio (valores raw)
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    
    // Temperatura
    float temperatura;
    
    bool leituraValida;
};

// ============================================================================
// CLASSE DO SENSOR ULTRASSÔNICO
// ============================================================================

class SensorUltrassonico {
public:
    SensorUltrassonico(int pinTrig, int pinEcho, int numAmostras = 10);
    
    void inicializar();
    void atualizar();
    
    DadosDistancia getDados();
    float getDistanciaMedia();
    float getDistanciaAtual();
    bool isLeituraValida();

private:
    int _pinTrig;
    int _pinEcho;
    int _numAmostras;
    int _indiceAmostra;
    int _contadorAmostras;
    
    float* _amostras;
    float _distanciaAtual;
    float _distanciaMedia;
    bool _leituraValida;
    
    float lerDistancia();
    void calcularMedia();
};

// ============================================================================
// CLASSE DO MPU-6050
// ============================================================================

class SensorIMU {
public:
    SensorIMU(uint8_t endereco = 0x68);
    
    bool inicializar();
    void atualizar();
    
    DadosIMU getDados();
    bool isLeituraValida();
    
    // Métodos para obter dados formatados
    String getJsonInclinacao();

private:
    uint8_t _endereco;
    DadosIMU _dados;
    
    void lerDadosRaw();
};

#endif // SENSORS_H
