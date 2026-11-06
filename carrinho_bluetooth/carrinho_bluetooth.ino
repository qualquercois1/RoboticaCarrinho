#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SoftwareSerial BTSerial(12, 13); // RX, TX

// Pinos do Motor (Ponte H L298N)
#define ENA1 11 // Controle de velocidade Motor A
#define IN1 10  // Direção 1 Motor A
#define IN2 9   // Direção 2 Motor A
#define IN3 8   // Direção 1 Motor B
#define IN4 7   // Direção 2 Motor B
#define ENA2 6  // Controle de velocidade Motor B

// Variáveis de controle
int time = 0; 
bool pressed_power = false, cooldown_power = false, power = false;
unsigned long tempoAnterior = 0;

char direcaoAtual = 'S'; // Guarda a direção atual (S = Stop)

void setup() {
  Serial.begin(9600);
  
  // Configura os pinos dos motores como SAÍDA
  pinMode(ENA1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA2, OUTPUT);
  parar(); // Garante que os motores comecem parados
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao encontrar o display SSD1306"));
    for(;;); 
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Sistema Pronto");
  display.display();

  BTSerial.begin(9600);
  Serial.println("Pronto para receber dados via Bluetooth...");
}

void loop() {
  // 1. Lógica do Bluetooth
  if (BTSerial.available()) {
    char data = BTSerial.read();
    Serial.print("Recebido: ");
    Serial.println(data);
    
    if (data == '1') {
      if (!power && !cooldown_power) {
        pressed_power = true;
      } else {
        Serial.println("Ignorado: Habilidade ativa ou em cooldown.");
      }
    } 
    // Atualiza a direção com base no comando Bluetooth (F, B, L, R, S)
    else if (data == 'F' || data == 'B' || data == 'L' || data == 'R' || data == 'S') {
      direcaoAtual = data;
    }

    delay(10); 
    while (BTSerial.available() > 0) {
      char lixo = BTSerial.read(); 
    }
  }

  // 2. Lógica de controle do Power Up
  if (pressed_power && !cooldown_power && !power) {
    power = true;
    time = 10;
    cooldown_power = false;
    pressed_power = false;    
    tempoAnterior = millis(); 
  }

  // Controle de tempo do Power Up ativo
  if (power) {
      if (millis() - tempoAnterior >= 1000) {
        time--;
        tempoAnterior = millis();
      }
      if (time == -1) {
        power = false;
        cooldown_power = true;
        time = 5; 
      }
  }

  // Controle de tempo do Cooldown
  if (cooldown_power) {
      if (millis() - tempoAnterior >= 1000) {
        time--;
        tempoAnterior = millis();
      }
      if (time == -1) {
        power = false;
        cooldown_power = false;
        time = 0;
      }
  }

  // 3. Atualiza os Motores com base no Power Up
  int velocidade = power ? 255 : 150; // Se power up ativo, vel=255. Senão, vel=150.
  
  if (direcaoAtual == 'F') frente(velocidade);
  else if (direcaoAtual == 'B') tras(velocidade);
  else if (direcaoAtual == 'L') esquerda(velocidade);
  else if (direcaoAtual == 'R') direita(velocidade);
  else parar();

  // 4. Atualização do Display OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  
  if (power) display.print("POWER UP: MODO TURBO!");
  else if (cooldown_power) display.print("RESFRIANDO MOTOR...");
  else display.print("Motores Normais");

  display.setCursor(0, 20); 
  display.print("Tempo: ");
  display.println(time);
  
  display.setCursor(0, 40); 
  display.print("Direcao: ");
  display.println(direcaoAtual);

  display.display();
}

// ==========================================
// FUNÇÕES DE CONTROLE DOS MOTORES
// ==========================================

void frente(int velocidade) {
  analogWrite(ENA1, velocidade);
  analogWrite(ENA2, velocidade);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void tras(int velocidade) {
  analogWrite(ENA1, velocidade);
  analogWrite(ENA2, velocidade);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void esquerda(int velocidade) {
  analogWrite(ENA1, velocidade);
  analogWrite(ENA2, velocidade);
  // Motor A para trás, Motor B para frente
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void direita(int velocidade) {
  analogWrite(ENA1, velocidade);
  analogWrite(ENA2, velocidade);
  // Motor A para frente, Motor B para trás
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