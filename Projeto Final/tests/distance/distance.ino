const int trigPin = 5;
const int echoPin = 3;

#define SOUND_SPEED 0.034
#define SAMPLE_INTERVAL 100 // Intervalo entre leituras em milissegundos
#define NUM_SAMPLES 10      // Número de amostras para a média

long duration;
float distanceCm;
unsigned long previousMillis = 0;

float samples[NUM_SAMPLES];  // Array para armazenar as amostras
int sampleIndex = 0;         // Índice atual no array
int sampleCount = 0;         // Contador de amostras coletadas

void setup() {
  Serial.begin(115200); 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Verifica se é hora de fazer uma nova leitura
  if (currentMillis - previousMillis >= SAMPLE_INTERVAL) {
    previousMillis = currentMillis;
    
    // Gera pulso ultrassônico
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    // Calcula a distância
    duration = pulseIn(echoPin, HIGH);
    distanceCm = duration * SOUND_SPEED / 2;
    
    // Adiciona a nova amostra ao array
    samples[sampleIndex] = distanceCm;
    sampleIndex = (sampleIndex + 1) % NUM_SAMPLES; // Circular buffer
    
    if (sampleCount < NUM_SAMPLES) {
      sampleCount++;
    }
    
    // Calcula e exibe a média
    if (sampleCount == NUM_SAMPLES) {
      float sum = 0;
      for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += samples[i];
      }
      float average = sum / NUM_SAMPLES;
      
      Serial.print("Média (cm): ");
      Serial.println(average);
    }
  }
  
  // Aqui você pode adicionar outras tarefas que precisam rodar continuamente
}