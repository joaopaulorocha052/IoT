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
 * 
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>

#include "config.h"
#include "sensors.h"
#include "motor_control.h"
#include "mqtt_handler.h"

float Kp = PID_KP_INICIAL;
float Ki = PID_KI_INICIAL;
float Kd = PID_KD_INICIAL;

// Sensores
SensorUltrassonico sensorDistancia(PIN_TRIG, PIN_ECHO, NUM_AMOSTRAS);
SensorIMU sensorIMU(MPU_ADDR);
SensorBateria sensorBateria(PIN_BATERIA, BATERIA_NUM_AMOSTRAS);

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

// VARIÁVEIS DE CONTROLE DE TEMPO
unsigned long ultimoTempoSensor = 0;
unsigned long ultimoTempoPID = 0;
unsigned long ultimoTempoTelemetria = 0;

// VARIÁVEIS DE ESTADO
bool sistemaEmErro = false;
bool conexaoPerdida = false;

// CALLBACKS MQTT
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

// FUNÇÕES DE LED DE STATUS
void configurarLedStatus() {
    pinMode(PIN_LED_STATUS, OUTPUT);
    digitalWrite(PIN_LED_STATUS, LOW);
}

void setLedErro(bool erro) {
    digitalWrite(PIN_LED_STATUS, erro ? HIGH : LOW);
}

// VERIFICAÇÃO DE CONEXÃO
void verificarConexao() {
    bool conectado = mqtt.isConectado();
    
    if (!conectado && !conexaoPerdida) {
        // Acabou de perder conexão
        conexaoPerdida = true;
        sistemaEmErro = true;
        sistema.pararEmergencia();
        // setLedErro(true);
        Serial.println("[MAIN] CONEXÃO PERDIDA - Carrinho parado!");
    }
    else if (conectado && conexaoPerdida) {
        // Reconectou
        conexaoPerdida = false;
        sistemaEmErro = false;
        // setLedErro(false);
        Serial.println("[MAIN] Conexão restabelecida");
    }
}

void atualiza_leds_distancia() {
    float distancia = sensorDistancia.getDistanciaAtual();
    int threshold = sistema.getSetpoint();
    if (abs(distancia - threshold) < INSIDE_THRESHOLD) {
        // Muito próximo - LED vermelho
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(PIN_LED_INSIDE_THRESHOLD, LOW);
        mqtt.publicarDentro("0");
        mqtt.publicarFora("1");

    } else {
        // Fora do limite - LED apagado
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(PIN_LED_INSIDE_THRESHOLD, HIGH);
        mqtt.publicarDentro("1");
        mqtt.publicarFora("0");
    }

    
}

// SETUP
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
    
    // Inicializa I2C para MPU-6050
    Wire.begin(PIN_SDA, PIN_SCL);
    
    // Inicializa sensores
    Serial.println("[SETUP] Inicializando sensores...");
    sensorDistancia.inicializar();
    
    if (!sensorIMU.inicializar()) {
        Serial.println("[SETUP] AVISO: MPU-6050 não detectado!");
    }
    
    sensorBateria.inicializar();
    
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

    /* 34 é o pino analógico para leitura da bateria; o cálculo da porcentagem foi realizado diretamente no broker MQTT */
    pinMode(34, INPUT);
    pinMode(PIN_LED_INSIDE_THRESHOLD, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_TOUCH_SENSOR, INPUT);
}

// LOOP PRINCIPAL
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
        sensorBateria.atualizar();

        /*
        * Essa parte foi removida pois o status do sistema estava sobrescrevendo o controle via MQTT.
        * Comentário mantido para referência futura.
        */
        int touchValue = touchRead(PIN_TOUCH_SENSOR);
        if (touchValue > 40) {
            // sistema.ativar();
            // digitalWrite(LED_BUILTIN, HIGH);
            mqtt.publicarTouch("1");
        } else {
            // sistema.desativar();
            // digitalWrite(LED_BUILTIN, LOW);
            mqtt.publicarTouch("0");
        }
        Serial.println(touchValue);

        atualiza_leds_distancia();
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

        /* IMPORTANTE
            * A leitura da bateria foi alterada para o pino 34 diretamente aqui no main,
            * para simplificar a implementação. Idealmente, deveria ser feita via a classe
            * SensorBateria, mas isso exigiria mudanças adicionais.
        */
        int tensao = analogRead(34);
        
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

            mqtt.publicarBateria(tensao);
            
            mqtt.publicarAceleracao(sensorIMU.getDadoTile());
            
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
        Serial.print(sistema.isAtivo() ? "ATIVO" : "INATIVO");
        Serial.print(" | Bat: ");
        Serial.print("(");
        Serial.print(tensao);
        Serial.print("V)");
        
        Serial.println();
    }
}