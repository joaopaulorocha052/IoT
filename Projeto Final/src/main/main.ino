/*
 * ============================================================================
 * PROJETO FINAL - CARRINHO COM CONTROLE DE DISTÂNCIA ADAPTATIVO
 * ============================================================================
 * 
 * Disciplina: Internet das Coisas (IoT)
 * Universidade Federal de São Paulo (UNIFESP)
 * 
 * Descrição:
 * Sistema de controle de cruzeiro adaptativo que mantém distância segura
 * de objetos à frente. Telemetria enviada via MQTT para o Flespi.
 * 
 * Componentes:
 * - ESP32
 * - Sensor Ultrassônico HC-SR04
 * - Giroscópio/Acelerômetro MPU-6050
 * - Driver de Motor L298N
 * - 2x Motores DC
 * 
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "sensors.h"
#include "motor_control.h"
#include "mqtt_handler.h"

// ============================================================================
// VARIÁVEIS GLOBAIS - PID (Fácil acesso para calibração)
// ============================================================================
float Kp = PID_KP_INICIAL;
float Ki = PID_KI_INICIAL;
float Kd = PID_KD_INICIAL;

// ============================================================================
// INSTÂNCIAS DOS MÓDULOS
// ============================================================================

// Sensores
SensorUltrassonico sensorDistancia(PIN_TRIG, PIN_ECHO, NUM_AMOSTRAS);
SensorIMU sensorIMU(MPU_ADDR);

// Controle de motores
ControladorMotores motores(PIN_IN1, PIN_IN2, PIN_ENA,
                           PIN_IN3, PIN_IN4, PIN_ENB,
                           PWM_CHANNEL_A, PWM_CHANNEL_B);

// Controlador PID
ControladorPID pid(Kp, Ki, Kd);

// Sistema de controle de distância
SistemaControleDistancia sistema(&motores, &pid);

// Comunicação MQTT
GerenciadorMQTT mqtt;

// ============================================================================
// VARIÁVEIS DE CONTROLE DE TEMPO
// ============================================================================
unsigned long ultimoTempoSensor = 0;
unsigned long ultimoTempoPID = 0;
unsigned long ultimoTempoTelemetria = 0;

// ============================================================================
// VARIÁVEIS DE ESTADO
// ============================================================================
bool sistemaEmErro = false;
bool conexaoPerdida = false;

// ============================================================================
// CALLBACKS MQTT
// ============================================================================

void onSetpointRecebido(float novoSetpoint) {
    Serial.print("[MAIN] Novo setpoint recebido: ");
    Serial.print(novoSetpoint);
    Serial.println(" cm");
    
    sistema.setSetpoint(novoSetpoint);
}

void onControleRecebido(bool ativar) {
    Serial.print("[MAIN] Comando de controle recebido: ");
    Serial.println(ativar ? "ATIVAR" : "DESATIVAR");
    
    if (ativar) {
        sistema.ativar();
    } else {
        sistema.desativar();
    }
}

// ============================================================================
// FUNÇÕES DE LED DE STATUS
// ============================================================================

void configurarLedStatus() {
    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, LOW);
}

void setLedErro(bool erro) {
    digitalWrite(PIN_LED_STATUS, erro ? HIGH : LOW);
}

// ============================================================================
// VERIFICAÇÃO DE CONEXÃO
// ============================================================================

void verificarConexao() {
    bool conectado = mqtt.isConectado();
    
    if (!conectado && !conexaoPerdida) {
        // Acabou de perder conexão
        conexaoPerdida = true;
        sistemaEmErro = true;
        sistema.pararEmergencia();
        setLedErro(true);
        Serial.println("[MAIN] CONEXÃO PERDIDA - Carrinho parado!");
    }
    else if (conectado && conexaoPerdida) {
        // Reconectou
        conexaoPerdida = false;
        sistemaEmErro = false;
        setLedErro(false);
        Serial.println("[MAIN] Conexão restabelecida");
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Inicializa Serial
    Serial.begin(115200);
    delay(1000);
    
    Serial.println();
    Serial.println("============================================");
    Serial.println("  CARRINHO - CONTROLE DE DISTÂNCIA");
    Serial.println("  Projeto Final IoT - UNIFESP");
    Serial.println("============================================");
    Serial.println();
    
    // Configura LED de status
    configurarLedStatus();
    setLedErro(false);
    
    // Inicializa I2C para MPU-6050
    Wire.begin(PIN_SDA, PIN_SCL);
    
    // Inicializa sensores
    Serial.println("[SETUP] Inicializando sensores...");
    sensorDistancia.inicializar();
    
    if (!sensorIMU.inicializar()) {
        Serial.println("[SETUP] AVISO: MPU-6050 não detectado!");
    }
    
    // Inicializa motores
    Serial.println("[SETUP] Inicializando motores...");
    motores.inicializar();
    
    // Inicializa PID
    Serial.println("[SETUP] Inicializando PID...");
    pid.inicializar();
    
    // Inicializa sistema de controle
    Serial.println("[SETUP] Inicializando sistema de controle...");
    sistema.inicializar();
    
    // Inicializa MQTT
    Serial.println("[SETUP] Inicializando comunicação MQTT...");
    mqtt.inicializar();
    
    // Registra callbacks
    mqtt.setCallbackSetpoint(onSetpointRecebido);
    mqtt.setCallbackControle(onControleRecebido);
    
    // Conecta WiFi e MQTT
    Serial.println("[SETUP] Conectando...");
    if (mqtt.conectarWiFi()) {
        mqtt.conectarMQTT();
    }
    
    Serial.println();
    Serial.println("[SETUP] Inicialização completa!");
    Serial.println("============================================");
    Serial.println();
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    unsigned long tempoAtual = millis();
    
    // Processa MQTT
    mqtt.loop();
    
    // Verifica conexão
    verificarConexao();
    
    // Se está em erro, apenas pisca LED e tenta reconectar
    if (sistemaEmErro) {
        return;
    }
    
    // ========== LEITURA DOS SENSORES ==========
    if (tempoAtual - ultimoTempoSensor >= INTERVALO_SENSOR) {
        ultimoTempoSensor = tempoAtual;
        
        // Atualiza leituras
        sensorDistancia.atualizar();
        sensorIMU.atualizar();
    }
    
    // ========== CONTROLE PID ==========
    if (tempoAtual - ultimoTempoPID >= INTERVALO_PID) {
        ultimoTempoPID = tempoAtual;
        
        // Atualiza controle com a distância média
        if (sensorDistancia.isLeituraValida()) {
            float distancia = sensorDistancia.getDistanciaMedia();
            sistema.atualizar(distancia);
        }
    }
    
    // ========== ENVIO DE TELEMETRIA ==========
    if (tempoAtual - ultimoTempoTelemetria >= INTERVALO_TELEMETRIA) {
        ultimoTempoTelemetria = tempoAtual;
        
        // Publica dados
        if (mqtt.isConectado()) {
            // Distância
            mqtt.publicarDistancia(sensorDistancia.getDistanciaMedia());
            
            // Velocidade
            mqtt.publicarVelocidade(sistema.getVelocidadeSaida());
            
            // Inclinação (IMU)
            if (sensorIMU.isLeituraValida()) {
                mqtt.publicarInclinacao(sensorIMU.getJsonInclinacao());
            }
            
            // Status
            String status = sistema.isAtivo() ? "ativo" : "inativo";
            mqtt.publicarStatus(status);
        }
        
        // Debug no Serial
        Serial.print("[TELEMETRIA] Dist: ");
        Serial.print(sensorDistancia.getDistanciaMedia());
        Serial.print(" cm | Setpoint: ");
        Serial.print(sistema.getSetpoint());
        Serial.print(" cm | Vel: ");
        Serial.print(sistema.getVelocidadeSaida());
        Serial.print(" | Estado: ");
        Serial.println(sistema.isAtivo() ? "ATIVO" : "INATIVO");
    }
}

// ============================================================================
// FUNÇÕES AUXILIARES PARA DEBUG/CALIBRAÇÃO (via Serial)
// ============================================================================

/*
 * Para calibrar o PID via Serial Monitor, você pode adicionar comandos
 * no loop para receber valores. Exemplo de uso futuro:
 * 
 * void processarComandoSerial() {
 *     if (Serial.available()) {
 *         String cmd = Serial.readStringUntil('\n');
 *         
 *         if (cmd.startsWith("KP=")) {
 *             Kp = cmd.substring(3).toFloat();
 *             pid.setKp(Kp);
 *         }
 *         else if (cmd.startsWith("KI=")) {
 *             Ki = cmd.substring(3).toFloat();
 *             pid.setKi(Ki);
 *         }
 *         else if (cmd.startsWith("KD=")) {
 *             Kd = cmd.substring(3).toFloat();
 *             pid.setKd(Kd);
 *         }
 *     }
 * }
 */
