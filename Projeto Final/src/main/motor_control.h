/*
 * ============================================================================
 * MÓDULO DE CONTROLE DE MOTORES
 * ============================================================================
 * Gerencia o driver L298N e implementa o controlador PID para manter
 * a distância desejada.
 * ============================================================================
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

// ============================================================================
// ENUMERAÇÕES
// ============================================================================

enum DirecaoMotor {
    PARADO,
    FRENTE,
    TRAS
};

enum EstadoControle {
    CONTROLE_ATIVO,
    CONTROLE_INATIVO,
    CONTROLE_ERRO
};

// ============================================================================
// ESTRUTURA DE DADOS DO PID
// ============================================================================

struct DadosPID {
    float setpoint;         // Distância desejada
    float entrada;          // Distância atual
    float saida;            // Valor de saída do PID
    float erro;             // Erro atual
    float erroAnterior;     // Erro da iteração anterior
    float integral;         // Termo integral acumulado
    float derivativo;       // Termo derivativo
    
    // Ganhos (variáveis globais para fácil calibração)
    float Kp;
    float Ki;
    float Kd;
};

// ============================================================================
// CLASSE DO CONTROLADOR DE MOTORES
// ============================================================================

class ControladorMotores {
public:
    ControladorMotores(int pinIN1, int pinIN2, int pinENA,
                       int pinIN3, int pinIN4, int pinENB,
                       int canalA = 0, int canalB = 1);
    
    void inicializar();
    
    // Controle direto dos motores
    void setVelocidade(int velocidade);  // -255 a 255 (negativo = ré)
    void parar();
    void frente(int velocidade);
    void tras(int velocidade);
    
    // Controle individual (se necessário no futuro)
    void setMotorA(DirecaoMotor direcao, int velocidade);
    void setMotorB(DirecaoMotor direcao, int velocidade);
    
    // Getters
    int getVelocidadeAtual();
    DirecaoMotor getDirecaoAtual();

private:
    // Pinos Motor A
    int _pinIN1;
    int _pinIN2;
    int _pinENA;
    int _canalA;
    
    // Pinos Motor B
    int _pinIN3;
    int _pinIN4;
    int _pinENB;
    int _canalB;
    
    // Estado atual
    int _velocidadeAtual;
    DirecaoMotor _direcaoAtual;
    
    void configurarPWM();
};

// ============================================================================
// CLASSE DO CONTROLADOR PID
// ============================================================================

class ControladorPID {
public:
    ControladorPID(float Kp, float Ki, float Kd);
    
    void inicializar();
    void resetar();
    
    // Calcula a saída do PID dado o erro atual
    float calcular(float setpoint, float entrada);
    
    // Ajuste de ganhos (para calibração)
    void setGanhos(float Kp, float Ki, float Kd);
    void setKp(float Kp);
    void setKi(float Ki);
    void setKd(float Kd);
    
    // Getters
    DadosPID getDados();
    float getKp();
    float getKi();
    float getKd();

private:
    DadosPID _dados;
    unsigned long _tempoAnterior;
    
    float limitarIntegral(float valor);
    float limitarSaida(float valor);
};

// ============================================================================
// CLASSE PRINCIPAL - SISTEMA DE CONTROLE DE DISTÂNCIA
// ============================================================================

class SistemaControleDistancia {
public:
    SistemaControleDistancia(ControladorMotores* motores, ControladorPID* pid);
    
    void inicializar();
    void atualizar(float distanciaAtual);
    
    // Controle do sistema
    void ativar();
    void desativar();
    void setSetpoint(float distancia);
    void pararEmergencia();
    
    // Estado
    bool isAtivo();
    EstadoControle getEstado();
    float getSetpoint();
    int getVelocidadeSaida();

private:
    ControladorMotores* _motores;
    ControladorPID* _pid;
    
    EstadoControle _estado;
    float _setpoint;
    int _velocidadeSaida;
    bool _ativo;
    
    int mapearPIDparaVelocidade(float saidaPID);
};

#endif // MOTOR_CONTROL_H
