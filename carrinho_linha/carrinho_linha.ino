#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pinos do Motor (Ponte H L298N)
#define ENA1 11 // Controle de velocidade Motor A (ESQUERDO)
#define IN1 10  // Direção 1 Motor A
#define IN2 9   // Direção 2 Motor A
#define IN3 8   // Direção 1 Motor B
#define IN4 7   // Direção 2 Motor B
#define ENA2 6  // Controle de velocidade Motor B (DIREITO)

// Pinos dos Sensores de Linha IR
#define SENSOR_DIR 2 // Sensor da Direita
#define SENSOR_ESQ 3 // Sensor da Esquerda

// ==========================================
// CONFIGURAÇÕES DE VELOCIDADE E TEMPO
// ==========================================
int velFrenteEsq = 100; 
int velFrenteDir = 130; 
int velCurvaEsq = 150;
int velCurvaDir = 180; 

// Tempos do Macro PWM (Step-and-Wait)
const int TEMPO_ANDANDO = 250; // Tempo aplicando força (ms)
const int TEMPO_PARADO = 30;  // Tempo freando/deslizando (ms)

// Variáveis de controle de tempo (Substituem o delay)
unsigned long tempoAnteriorOLED = 0;
unsigned long lastTimePasso = 0;
bool estadoAndando = true; // Define se está na fase de força ou de freio

String acaoDesejada = "Parado";
String statusMotor = "Parado";

void setup() {
  Serial.begin(9600);
  
  pinMode(ENA1, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENA2, OUTPUT);
  parar(); 
  
  pinMode(SENSOR_DIR, INPUT); pinMode(SENSOR_ESQ, INPUT);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha display SSD1306"));
    for(;;); 
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Modo: Passo millis()");
  display.display();
  delay(1000); 
}

void loop() {
  unsigned long tempoAtual = millis();

  // 1. Leitura Constante dos Sensores
  bool linhaEsq = digitalRead(SENSOR_ESQ);
  bool linhaDir = digitalRead(SENSOR_DIR);
  
  // 2. Decide a Direção Alvo (Sempre atualizado!)
  if (linhaEsq == LOW && linhaDir == LOW) {
    acaoDesejada = "Frente";
  } 
  else if (linhaEsq == HIGH && linhaDir == LOW) {
    acaoDesejada = "Esquerda";
  } 
  else if (linhaEsq == LOW && linhaDir == HIGH) {
    acaoDesejada = "Direita";
  } 
  else if (linhaEsq == HIGH && linhaDir == HIGH) {
    acaoDesejada = "Parado (Preto)";
  }

  // 3. Execução dos Motores (Macro PWM com millis)
  if (acaoDesejada == "Parado (Preto)") {
    // Fim de pista ou cruzamento: Interrompe o passo e para imediatamente
    parar();
    estadoAndando = false; 
    statusMotor = "Pista Fim";
  } 
  else {
    // Lógica Alternante (Andar <-> Frear)
    if (estadoAndando) {
      // FASE 1: Aplicando Força
      if (tempoAtual - lastTimePasso >= TEMPO_ANDANDO) {
        // Acabou o tempo de andar, muda para a fase de freio
        estadoAndando = false;
        lastTimePasso = tempoAtual;
        parar();
      } else {
        // Continua andando (Se o sensor mudar no meio, ele adapta a direção na hora!)
        aplicarMovimento(acaoDesejada);
        statusMotor = "Puxando";
      }
    } 
    else {
      // FASE 2: Freando (Aguardando Inércia)
      if (tempoAtual - lastTimePasso >= TEMPO_PARADO) {
        // Acabou o freio, volta a aplicar força
        estadoAndando = true;
        lastTimePasso = tempoAtual;
      } else {
        parar();
        statusMotor = "Deslizando";
      }
    }
  }

  // 4. Atualização do Display OLED (A cada 200ms)
  if (tempoAtual - tempoAnteriorOLED >= 200) {
    tempoAnteriorOLED = tempoAtual;
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Macro PWM Sem Delay");

    display.setCursor(0, 20); 
    display.print("E: "); display.print(linhaEsq ? "PRETO" : "BRANCO");
    display.print(" | D: "); display.print(linhaDir ? "PRETO" : "BRANCO");

    display.setCursor(0, 35); 
    display.print("Direcao: "); display.print(acaoDesejada);

    display.setCursor(0, 50); 
    display.print("Motor:   "); display.print(statusMotor);

    display.display();
  }
}

// ==========================================
// FUNÇÕES DE CONTROLE DOS MOTORES
// ==========================================

// Função auxiliar para manter o loop limpo
void aplicarMovimento(String acao) {
  if (acao == "Frente") frente(velFrenteEsq, velFrenteDir);
  else if (acao == "Esquerda") esquerda(velCurvaEsq, velCurvaDir);
  else if (acao == "Direita") direita(velCurvaEsq, velCurvaDir);
}

void frente(int velEsq, int velDir) {
  analogWrite(ENA1, velEsq);
  analogWrite(ENA2, velDir);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void tras(int velEsq, int velDir) {
  analogWrite(ENA1, velEsq);
  analogWrite(ENA2, velDir);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void direita(int velEsq, int velDir) {
  analogWrite(ENA1, velEsq);
  analogWrite(ENA2, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void esquerda(int velEsq, int velDir) {
  analogWrite(ENA1, 0);
  analogWrite(ENA2, velDir);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void parar() {
  analogWrite(ENA1, 0);
  analogWrite(ENA2, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}