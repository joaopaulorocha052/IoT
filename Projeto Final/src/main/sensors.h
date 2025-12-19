#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>

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

// Estados da bateria; lógica legada
enum EstadoBateria {
    BATERIA_NORMAL,      // > 30%
    BATERIA_BAIXA,       // 15-30%
    BATERIA_CRITICA      // < 15%
};

// Dados da bateria
struct DadosBateria {
    float tensao;            // Tensão atual em Volts
    float porcentagem;       // Nível de carga em %
    EstadoBateria estado;    // Estado da bateria
    bool leituraValida;      // Indica se a leitura é válida
};

// CLASSE DO SENSOR ULTRASSÔNICO
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

// CLASSE DO MPU-6050
class SensorIMU {
public:
    SensorIMU(uint8_t endereco = 0x68);
    
    bool inicializar();
    void atualizar();
    
    DadosIMU getDados();
    bool isLeituraValida();
    String getDadoTile();
    
    // Métodos para obter dados formatados
    String getJsonInclinacao();

private:
    uint8_t _endereco;
    DadosIMU _dados;
    
    void lerDadosRaw();
};

// CLASSE DO SENSOR DE BATERIA; lógcia legada pois o cálculo foi feito no broker
class SensorBateria {
public:
    SensorBateria(int pin, int numAmostras = 10);
    
    void inicializar();
    void atualizar();
    
    // Getters
    DadosBateria getDados();
    float getTensao();
    float getPorcentagem();
    float getNivelBateria();  // Alias para getPorcentagem
    EstadoBateria getEstado();
    bool isLeituraValida();
    
    // Verificações de estado
    bool isBateriaBaixa();
    bool isBateriaCritica();
    
    // Configuração
    void setTensaoMaxima(float tensaoMax);
    void setTensaoMinima(float tensaoMin);
    void setLimites(float tensaoMin, float tensaoMax);

private:
    int _pin;                // Pino ADC para leitura
    int _numAmostras;        // Número de amostras para média
    int _indiceAmostra;      // Índice no buffer circular
    int _contadorAmostras;   // Contador de amostras válidas
    
    float* _amostras;        // Buffer para média móvel
    float _tensaoAtual;      // Tensão atual
    float _tensaoMedia;      // Média das tensões
    float _porcentagem;      // Porcentagem da bateria
    EstadoBateria _estado;   // Estado atual da bateria
    bool _leituraValida;     // Flag de leitura válida
    
    // Limites de tensão (configuráveis)
    float _tensaoMaxima;     // Tensão para 100% (padrão: 8.4V para 2S LiPo)
    float _tensaoMinima;     // Tensão para 0% (padrão: 6.0V para 2S LiPo)
    
    // Constantes do ADC do ESP32
    static const int ADC_RESOLUTION = 4095;  // 12 bits
    static const float ADC_VOLTAGE;          // Tensão de referência (3.3V)
    
    // Divisor de tensão (ajustar conforme hardware)
    float _resistor1;        // Resistor superior do divisor
    float _resistor2;        // Resistor inferior do divisor
    
    // Métodos privados
    float lerTensao();
    void calcularMedia();
    void calcularPorcentagem();
    void atualizarEstado();
};

#endif // SENSORS_H
