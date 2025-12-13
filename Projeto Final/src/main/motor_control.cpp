/*
 * ============================================================================
 * IMPLEMENTAÇÃO DO MÓDULO DE CONTROLE DE MOTORES
 * ============================================================================
 */

#include "motor_control.h"
#include "config.h"

// ============================================================================
// IMPLEMENTAÇÃO - CONTROLADOR DE MOTORES
// ============================================================================

ControladorMotores::ControladorMotores(int pinIN1, int pinIN2, int pinENA,
                                       int pinIN3, int pinIN4, int pinENB,
                                       int canalA, int canalB) {
    _pinIN1 = pinIN1;
    _pinIN2 = pinIN2;
    _pinENA = pinENA;
    _canalA = canalA;
    
    _pinIN3 = pinIN3;
    _pinIN4 = pinIN4;
    _pinENB = pinENB;
    _canalB = canalB;
    
    _velocidadeAtual = 0;
    _direcaoAtual = PARADO;
}

void ControladorMotores::inicializar() {
    // Configura pinos de direção como saída
    pinMode(_pinIN1, OUTPUT);
    pinMode(_pinIN2, OUTPUT);
    pinMode(_pinIN3, OUTPUT);
    pinMode(_pinIN4, OUTPUT);
    
    // Configura PWM usando ledcAttach (ESP32 Arduino Core 3.x)
    configurarPWM();
    
    // Garante que motores estão parados
    parar();
    
    Serial.println("[MOTOR] Controlador de motores inicializado");
    Serial.print("[MOTOR] Motor A - IN1: ");
    Serial.print(_pinIN1);
    Serial.print(", IN2: ");
    Serial.print(_pinIN2);
    Serial.print(", ENA: ");
    Serial.println(_pinENA);
    Serial.print("[MOTOR] Motor B - IN3: ");
    Serial.print(_pinIN3);
    Serial.print(", IN4: ");
    Serial.print(_pinIN4);
    Serial.print(", ENB: ");
    Serial.println(_pinENB);
}

void ControladorMotores::configurarPWM() {
    // ESP32 Arduino Core 3.x usa ledcAttach
    ledcAttach(_pinENA, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttach(_pinENB, PWM_FREQUENCY, PWM_RESOLUTION);
}

void ControladorMotores::setVelocidade(int velocidade) {
    // Limita velocidade aos limites configurados
    velocidade = constrain(velocidade, -PWM_MAX, PWM_MAX);
    
    if (velocidade > 0) {
        frente(velocidade);
    } else if (velocidade < 0) {
        tras(-velocidade);
    } else {
        parar();
    }
}

void ControladorMotores::parar() {
    // Para ambos os motores
    digitalWrite(_pinIN1, LOW);
    digitalWrite(_pinIN2, LOW);
    digitalWrite(_pinIN3, LOW);
    digitalWrite(_pinIN4, LOW);
    
    ledcWrite(_pinENA, 0);
    ledcWrite(_pinENB, 0);
    
    _velocidadeAtual = 0;
    _direcaoAtual = PARADO;
}

void ControladorMotores::frente(int velocidade) {
    velocidade = constrain(velocidade, 0, PWM_MAX);
    
    // Motor A - Frente
    digitalWrite(_pinIN1, HIGH);
    digitalWrite(_pinIN2, LOW);
    
    // Motor B - Frente
    digitalWrite(_pinIN3, HIGH);
    digitalWrite(_pinIN4, LOW);
    
    // Aplica PWM
    ledcWrite(_pinENA, velocidade);
    ledcWrite(_pinENB, velocidade);
    
    _velocidadeAtual = velocidade;
    _direcaoAtual = FRENTE;
}

void ControladorMotores::tras(int velocidade) {
    velocidade = constrain(velocidade, 0, PWM_MAX);
    
    // Motor A - Trás
    digitalWrite(_pinIN1, LOW);
    digitalWrite(_pinIN2, HIGH);
    
    // Motor B - Trás
    digitalWrite(_pinIN3, LOW);
    digitalWrite(_pinIN4, HIGH);
    
    // Aplica PWM
    ledcWrite(_pinENA, velocidade);
    ledcWrite(_pinENB, velocidade);
    
    _velocidadeAtual = velocidade;
    _direcaoAtual = TRAS;
}

void ControladorMotores::setMotorA(DirecaoMotor direcao, int velocidade) {
    velocidade = constrain(velocidade, 0, PWM_MAX);
    
    switch (direcao) {
        case FRENTE:
            digitalWrite(_pinIN1, HIGH);
            digitalWrite(_pinIN2, LOW);
            break;
        case TRAS:
            digitalWrite(_pinIN1, LOW);
            digitalWrite(_pinIN2, HIGH);
            break;
        case PARADO:
        default:
            digitalWrite(_pinIN1, LOW);
            digitalWrite(_pinIN2, LOW);
            velocidade = 0;
            break;
    }
    
    ledcWrite(_pinENA, velocidade);
}

void ControladorMotores::setMotorB(DirecaoMotor direcao, int velocidade) {
    velocidade = constrain(velocidade, 0, PWM_MAX);
    
    switch (direcao) {
        case FRENTE:
            digitalWrite(_pinIN3, HIGH);
            digitalWrite(_pinIN4, LOW);
            break;
        case TRAS:
            digitalWrite(_pinIN3, LOW);
            digitalWrite(_pinIN4, HIGH);
            break;
        case PARADO:
        default:
            digitalWrite(_pinIN3, LOW);
            digitalWrite(_pinIN4, LOW);
            velocidade = 0;
            break;
    }
    
    ledcWrite(_pinENB, velocidade);
}

int ControladorMotores::getVelocidadeAtual() {
    return _velocidadeAtual;
}

DirecaoMotor ControladorMotores::getDirecaoAtual() {
    return _direcaoAtual;
}

// ============================================================================
// IMPLEMENTAÇÃO - CONTROLADOR PID
// ============================================================================

ControladorPID::ControladorPID(float Kp, float Ki, float Kd) {
    _dados.Kp = Kp;
    _dados.Ki = Ki;
    _dados.Kd = Kd;
    resetar();
}

void ControladorPID::inicializar() {
    resetar();
    Serial.println("[PID] Controlador PID inicializado");
    Serial.print("[PID] Ganhos - Kp: ");
    Serial.print(_dados.Kp);
    Serial.print(", Ki: ");
    Serial.print(_dados.Ki);
    Serial.print(", Kd: ");
    Serial.println(_dados.Kd);
}

void ControladorPID::resetar() {
    _dados.setpoint = 0;
    _dados.entrada = 0;
    _dados.saida = 0;
    _dados.erro = 0;
    _dados.erroAnterior = 0;
    _dados.integral = 0;
    _dados.derivativo = 0;
    _tempoAnterior = millis();
}

float ControladorPID::calcular(float setpoint, float entrada) {
    unsigned long tempoAtual = millis();
    float dt = (tempoAtual - _tempoAnterior) / 1000.0; // Converte para segundos
    
    if (dt <= 0) {
        dt = 0.001; // Evita divisão por zero
    }
    
    _dados.setpoint = setpoint;
    _dados.entrada = entrada;
    
    // Calcula erro (positivo = muito longe, negativo = muito perto)
    _dados.erro = setpoint - entrada;
    
    // Termo Proporcional
    float P = _dados.Kp * _dados.erro;
    
    // Termo Integral (com anti-windup)
    _dados.integral += _dados.erro * dt;
    _dados.integral = limitarIntegral(_dados.integral);
    float I = _dados.Ki * _dados.integral;
    
    // Termo Derivativo
    _dados.derivativo = (_dados.erro - _dados.erroAnterior) / dt;
    float D = _dados.Kd * _dados.derivativo;
    
    // Calcula saída
    _dados.saida = P + I + D;
    _dados.saida = limitarSaida(_dados.saida);
    
    // Atualiza valores anteriores
    _dados.erroAnterior = _dados.erro;
    _tempoAnterior = tempoAtual;
    
    return _dados.saida;
}

float ControladorPID::limitarIntegral(float valor) {
    return constrain(valor, PID_INTEGRAL_MIN, PID_INTEGRAL_MAX);
}

float ControladorPID::limitarSaida(float valor) {
    return constrain(valor, -PWM_MAX, PWM_MAX);
}

void ControladorPID::setGanhos(float Kp, float Ki, float Kd) {
    _dados.Kp = Kp;
    _dados.Ki = Ki;
    _dados.Kd = Kd;
}

void ControladorPID::setKp(float Kp) { _dados.Kp = Kp; }
void ControladorPID::setKi(float Ki) { _dados.Ki = Ki; }
void ControladorPID::setKd(float Kd) { _dados.Kd = Kd; }

DadosPID ControladorPID::getDados() { return _dados; }
float ControladorPID::getKp() { return _dados.Kp; }
float ControladorPID::getKi() { return _dados.Ki; }
float ControladorPID::getKd() { return _dados.Kd; }

// ============================================================================
// IMPLEMENTAÇÃO - SISTEMA DE CONTROLE DE DISTÂNCIA
// ============================================================================

SistemaControleDistancia::SistemaControleDistancia(ControladorMotores* motores, 
                                                    ControladorPID* pid) {
    _motores = motores;
    _pid = pid;
    _estado = CONTROLE_INATIVO;
    _setpoint = DISTANCIA_PADRAO;
    _velocidadeSaida = 0;
    _ativo = false;
}

void SistemaControleDistancia::inicializar() {
    _pid->resetar();
    _estado = CONTROLE_INATIVO;
    _ativo = false;
    
    Serial.println("[SISTEMA] Sistema de controle de distância inicializado");
    Serial.print("[SISTEMA] Setpoint inicial: ");
    Serial.print(_setpoint);
    Serial.println(" cm");
}

void SistemaControleDistancia::atualizar(float distanciaAtual) {
    if (!_ativo) {
        _motores->parar();
        _velocidadeSaida = 0;
        return;
    }
    
    // Calcula saída do PID
    float saidaPID = _pid->calcular(_setpoint, distanciaAtual);
    
    // Mapeia saída do PID para velocidade do motor
    _velocidadeSaida = mapearPIDparaVelocidade(saidaPID);
    
    // Aplica velocidade aos motores
    _motores->setVelocidade(_velocidadeSaida);
    
    _estado = CONTROLE_ATIVO;
}

int SistemaControleDistancia::mapearPIDparaVelocidade(float saidaPID) {
    // saidaPID positivo = precisa ir para frente (está longe demais)
    // saidaPID negativo = precisa ir para trás (está perto demais)
    
    int velocidade = (int)saidaPID;
    
    // Aplica velocidade mínima para movimento (zona morta do motor)
    if (abs(velocidade) > 0 && abs(velocidade) < PWM_MIN_MOVIMENTO) {
        velocidade = (velocidade > 0) ? PWM_MIN_MOVIMENTO : -PWM_MIN_MOVIMENTO;
    }
    
    // Se erro muito pequeno, para
    DadosPID dados = _pid->getDados();
    if (abs(dados.erro) < DISTANCIA_TOLERANCIA) {
        velocidade = 0;
    }
    
    return constrain(velocidade, -PWM_MAX, PWM_MAX);
}

void SistemaControleDistancia::ativar() {
    _ativo = true;
    _pid->resetar();
    _estado = CONTROLE_ATIVO;
    Serial.println("[SISTEMA] Controle ATIVADO");
}

void SistemaControleDistancia::desativar() {
    _ativo = false;
    _motores->parar();
    _velocidadeSaida = 0;
    _estado = CONTROLE_INATIVO;
    Serial.println("[SISTEMA] Controle DESATIVADO");
}

void SistemaControleDistancia::setSetpoint(float distancia) {
    _setpoint = constrain(distancia, DISTANCIA_MIN, DISTANCIA_MAX);
    _pid->resetar(); // Reseta integral ao mudar setpoint
    Serial.print("[SISTEMA] Novo setpoint: ");
    Serial.print(_setpoint);
    Serial.println(" cm");
}

void SistemaControleDistancia::pararEmergencia() {
    _ativo = false;
    _motores->parar();
    _velocidadeSaida = 0;
    _estado = CONTROLE_ERRO;
    Serial.println("[SISTEMA] PARADA DE EMERGÊNCIA!");
}

bool SistemaControleDistancia::isAtivo() { return _ativo; }
EstadoControle SistemaControleDistancia::getEstado() { return _estado; }
float SistemaControleDistancia::getSetpoint() { return _setpoint; }
int SistemaControleDistancia::getVelocidadeSaida() { return _velocidadeSaida; }
