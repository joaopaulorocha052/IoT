#include "sensors.h"
#include "config.h"

// IMPLEMENTAÇÃO - SENSOR ULTRASSÔNICO
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

// IMPLEMENTAÇÃO - MPU-6050
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

// Adaptação para apenas conseguir pegar um valor do acelerômetro
String SensorIMU::getDadoTile(){
    return String(_dados.accelX);
}

// IMPLEMENTAÇÃO - SENSOR DE BATERIA; lógica legada
// Definição da constante estática
const float SensorBateria::ADC_VOLTAGE = 3.3;

SensorBateria::SensorBateria(int pin, int numAmostras) {
    _pin = pin;
    _numAmostras = numAmostras;
    _indiceAmostra = 0;
    _contadorAmostras = 0;
    _tensaoAtual = 0;
    _tensaoMedia = 0;
    _porcentagem = 0;
    _estado = BATERIA_NORMAL;
    _leituraValida = false;
    
    // Configuração padrão para bateria 2S LiPo
    _tensaoMaxima = BATERIA_TENSAO_MAX;
    _tensaoMinima = BATERIA_TENSAO_MIN;
    
    // Configuração do divisor de tensão
    _resistor1 = BATERIA_R1;
    _resistor2 = BATERIA_R2;
    
    // Aloca array para amostras
    _amostras = new float[_numAmostras];
    for (int i = 0; i < _numAmostras; i++) {
        _amostras[i] = 0;
    }
}

void SensorBateria::inicializar() {
    pinMode(_pin, INPUT);
    
    // Configura resolução do ADC para 12 bits
    analogReadResolution(12);
    
    Serial.println("[SENSOR] Bateria inicializada");
    Serial.print("[SENSOR] Pino ADC: ");
    Serial.println(_pin);
    Serial.print("[SENSOR] Tensão máxima: ");
    Serial.print(_tensaoMaxima);
    Serial.println(" V");
    Serial.print("[SENSOR] Tensão mínima: ");
    Serial.print(_tensaoMinima);
    Serial.println(" V");
    Serial.print("[SENSOR] Divisor de tensão: ");
    Serial.print((_resistor1 + _resistor2) / _resistor2, 2);
    Serial.println("x");
}

void SensorBateria::atualizar() {
    _tensaoAtual = lerTensao();
    
    // Verifica se a leitura está dentro dos limites razoáveis
    if (_tensaoAtual >= 0 && _tensaoAtual <= (_tensaoMaxima + 1.0)) {
        _leituraValida = true;
        
        // Adiciona ao buffer circular
        _amostras[_indiceAmostra] = _tensaoAtual;
        _indiceAmostra = (_indiceAmostra + 1) % _numAmostras;
        
        if (_contadorAmostras < _numAmostras) {
            _contadorAmostras++;
        }
        
        // Calcula média e porcentagem
        calcularMedia();
        calcularPorcentagem();
        atualizarEstado();
    } else {
        _leituraValida = false;
    }
}

float SensorBateria::lerTensao() {
    // Lê valor do ADC (0-4095)
    int valorADC = analogRead(_pin);
    
    // Converte para tensão no pino (0-3.3V)
    float tensaoPino = (valorADC * ADC_VOLTAGE) / ADC_RESOLUTION;
    
    // Aplica o divisor de tensão para obter tensão real da bateria
    float tensaoBateria = tensaoPino * ((_resistor1 + _resistor2) / _resistor2);
    
    return tensaoBateria;
}

void SensorBateria::calcularMedia() {
    if (_contadorAmostras == 0) {
        _tensaoMedia = 0;
        return;
    }
    
    float soma = 0;
    int amostrasValidas = min(_contadorAmostras, _numAmostras);
    
    for (int i = 0; i < amostrasValidas; i++) {
        soma += _amostras[i];
    }
    
    _tensaoMedia = soma / amostrasValidas;
}

void SensorBateria::calcularPorcentagem() {
    // Calcula porcentagem baseada na tensão média
    if (_tensaoMedia <= _tensaoMinima) {
        _porcentagem = 0.0;
    } else if (_tensaoMedia >= _tensaoMaxima) {
        _porcentagem = 100.0;
    } else {
        // Interpolação linear
        _porcentagem = (((_tensaoMedia - _tensaoMinima) / (_tensaoMaxima - _tensaoMinima)) * 100.0);
    }
    
    // Garante que está no intervalo 0-100
    _porcentagem = constrain(_porcentagem, 0.0, 100.0);
}

void SensorBateria::atualizarEstado() {
    if (_porcentagem >= BATERIA_LIMIAR_BAIXO) {
        _estado = BATERIA_NORMAL;
    } else if (_porcentagem >= BATERIA_LIMIAR_CRITICO) {
        _estado = BATERIA_BAIXA;
    } else {
        _estado = BATERIA_CRITICA;
    }
}

// Getters
DadosBateria SensorBateria::getDados() {
    DadosBateria dados;
    dados.tensao = _tensaoMedia;
    dados.porcentagem = _porcentagem;
    dados.estado = _estado;
    dados.leituraValida = _leituraValida;
    return dados;
}

float SensorBateria::getTensao() {
    return _tensaoMedia;
}

float SensorBateria::getPorcentagem() {
    return _porcentagem;
}

float SensorBateria::getNivelBateria() {
    return _porcentagem;
}

EstadoBateria SensorBateria::getEstado() {
    return _estado;
}

bool SensorBateria::isLeituraValida() {
    return _leituraValida;
}

// Verificações de estado
bool SensorBateria::isBateriaBaixa() {
    return _estado == BATERIA_BAIXA || _estado == BATERIA_CRITICA;
}

bool SensorBateria::isBateriaCritica() {
    return _estado == BATERIA_CRITICA;
}

// Configuração
void SensorBateria::setTensaoMaxima(float tensaoMax) {
    _tensaoMaxima = tensaoMax;
    calcularPorcentagem();
    atualizarEstado();
}

void SensorBateria::setTensaoMinima(float tensaoMin) {
    _tensaoMinima = tensaoMin;
    calcularPorcentagem();
    atualizarEstado();
}

void SensorBateria::setLimites(float tensaoMin, float tensaoMax) {
    _tensaoMinima = tensaoMin;
    _tensaoMaxima = tensaoMax;
    calcularPorcentagem();
    atualizarEstado();
}