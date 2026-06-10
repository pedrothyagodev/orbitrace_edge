// Projeto: Orbitrace - Console de Estacao Terrena
// Global Solution 2026 - 1 Semestre - FIAP
// Disciplina: Edge Computing & Computer Systems
//
// Integrantes:
//   Pedro Passos Corsini           - RM: 573493
//   Pedro Thyago Araujo dos Santos - RM: 570939
//   Daniel Gomes Torres           - RM: 573436
//   Henrique Lira                  - RM: 571009

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// configuracoes do display
#define OLED_LARGURA  128
#define OLED_ALTURA    64
#define OLED_RESET     -1
Adafruit_SSD1306 display(OLED_LARGURA, OLED_ALTURA, &Wire, OLED_RESET);

// pinos
#define PINO_LED_VERDE    6
#define PINO_LED_AMARELO  7
#define PINO_LED_LARANJA  8
#define PINO_LED_VERMELHO 9
#define PINO_BUZZER      10
#define PINO_BOTAO        2
#define PINO_LED_ONBOARD 13

// constantes fisicas
const float RAIO_TERRA = 6371.0f;
const float MU         = 398600.4418f;
const float LAMBDA     = 0.5f;
const float PI_VAL     = 3.14159265f;

// estrutura dos satelites
struct Satelite {
  const char* nome;
  const char* sigla;
  float       altitude;
  int         norad;
};

const int NUM_SATELITES = 3;
const Satelite satelites[NUM_SATELITES] = {
  { "CBERS-4A",   "CBERS-4A", 628.0f, 44883 },
  { "AMAZONIA-1", "AMAZ-1",   752.0f, 47699 },
  { "SCD-2",      "SCD-2",    750.0f, 25504 }
};

// variaveis globais da simulacao
float distanciaAtual    = 0.0f;
float probColisao       = 0.0f;
int   nivelRisco        = 0;
float periodoOrbital    = 0.0f;
float velocidadeOrbital = 0.0f;

// controle de tempo
unsigned long ultimaAtualizacao = 0UL;
const unsigned long INTERVALO_ATUALIZACAO = 3000UL;

// botao - debounce feito no loop
volatile bool flagBotao   = false;
unsigned long ultimoBotao = 0UL;
const unsigned long DEBOUNCE_MS = 250UL;

int indiceSatelite = 0;

// buzzer
unsigned long ultimoPulsoBuzzer = 0UL;
bool          buzzerAtivo       = false;

// prototipos
void  atualizarSimulacao();
float simularDistancia();
float calcularProb(float d);
int   classificarRisco(float d);
float calcularPeriodo(float h);
float calcularVelocidade(float h);
void  atualizarLEDs(int nivel);
void  gerenciarBuzzer(int nivel);
void  atualizarDisplay();
void  exibirTelaInicial();
void  erroOLED();
void  ISR_botao();

void setup() {
  Serial.begin(9600);

  pinMode(PINO_LED_VERDE,    OUTPUT);
  pinMode(PINO_LED_AMARELO,  OUTPUT);
  pinMode(PINO_LED_LARANJA,  OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_ONBOARD,  OUTPUT);
  pinMode(PINO_BUZZER,       OUTPUT);

  // pull-up interno no botao, ISR na borda de descida
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PINO_BOTAO), ISR_botao, FALLING);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    erroOLED();
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  randomSeed(analogRead(A0));

  exibirTelaInicial();
  delay(2500);

  atualizarSimulacao();
  ultimaAtualizacao = millis();
}

void loop() {
  unsigned long agora = millis();

  // verifica se o botao foi pressionado
  if (flagBotao && (agora - ultimoBotao >= DEBOUNCE_MS)) {
    flagBotao      = false;
    ultimoBotao    = agora;
    indiceSatelite = (indiceSatelite + 1) % NUM_SATELITES;
    atualizarSimulacao();
    ultimaAtualizacao = agora;
  }

  // atualiza a cada 3 segundos
  if (agora - ultimaAtualizacao >= INTERVALO_ATUALIZACAO) {
    atualizarSimulacao();
    ultimaAtualizacao = agora;
  }

  gerenciarBuzzer(nivelRisco);
}

// ISR - so seta a flag, nada mais
void ISR_botao() {
  flagBotao = true;
}

void atualizarSimulacao() {
  float h = satelites[indiceSatelite].altitude;

  distanciaAtual    = simularDistancia();
  probColisao       = calcularProb(distanciaAtual);
  nivelRisco        = classificarRisco(distanciaAtual);
  periodoOrbital    = calcularPeriodo(h);
  velocidadeOrbital = calcularVelocidade(h);

  noTone(PINO_BUZZER);
  buzzerAtivo = false;

  atualizarLEDs(nivelRisco);
  atualizarDisplay();

  const char* nomes_nivel[] = { "BAIXO", "MEDIO", "ALTO", "CRITICO" };
  Serial.print(F("SAT: "));
  Serial.print(satelites[indiceSatelite].nome);
  Serial.print(F(" | d="));
  Serial.print(distanciaAtual, 3);
  Serial.print(F(" km | P="));
  Serial.print(probColisao * 100.0f, 1);
  Serial.print(F("% | risco="));
  Serial.print(nomes_nivel[nivelRisco]);
  Serial.print(F(" | T="));
  Serial.print(periodoOrbital, 1);
  Serial.print(F("min | v="));
  Serial.print(velocidadeOrbital, 2);
  Serial.println(F(" km/s"));
}

// distribui a distancia em faixas de risco:
// 10% critico, 15% alto, 25% medio, 50% baixo
float simularDistancia() {
  long sorteio = random(0L, 100L);
  float d;

  if (sorteio < 10L) {
    d = 0.050f + (float)random(0L, 150L) / 1000.0f;
  } else if (sorteio < 25L) {
    d = 0.200f + (float)random(0L, 300L) / 1000.0f;
  } else if (sorteio < 50L) {
    d = 0.500f + (float)random(0L, 500L) / 1000.0f;
  } else {
    d = 1.000f + (float)random(0L, 4001L) / 1000.0f;
  }
  return d;
}

// P(d) = e^(-d / lambda)
float calcularProb(float d) {
  return expf(-d / LAMBDA);
}

int classificarRisco(float d) {
  if (d < 0.2f) return 3; // CRITICO
  if (d < 0.5f) return 2; // ALTO
  if (d < 1.0f) return 1; // MEDIO
  return 0;               // BAIXO
}

// 3a lei de Kepler: T = 2*pi * sqrt(a^3 / mu)
float calcularPeriodo(float h) {
  float a     = RAIO_TERRA + h;
  float t_seg = 2.0f * PI_VAL * sqrtf((a * a * a) / MU);
  return t_seg / 60.0f;
}

// v = sqrt(mu / a)
float calcularVelocidade(float h) {
  float a = RAIO_TERRA + h;
  return sqrtf(MU / a);
}

void atualizarLEDs(int nivel) {
  digitalWrite(PINO_LED_VERDE,    (nivel == 0) ? HIGH : LOW);
  digitalWrite(PINO_LED_AMARELO,  (nivel == 1) ? HIGH : LOW);
  digitalWrite(PINO_LED_LARANJA,  (nivel == 2) ? HIGH : LOW);
  digitalWrite(PINO_LED_VERMELHO, (nivel == 3) ? HIGH : LOW);
}

void gerenciarBuzzer(int nivel) {
  if (nivel == 0) {
    if (buzzerAtivo) {
      noTone(PINO_BUZZER);
      buzzerAtivo = false;
    }
    return;
  }

  unsigned long intervaloLigado;
  unsigned long intervaloDesligado;
  unsigned int  frequencia;

  switch (nivel) {
    case 1: // medio - alarme lento
      frequencia         = 880;
      intervaloLigado    = 200UL;
      intervaloDesligado = 1000UL;
      break;
    case 2: // alto - alarme rapido
      frequencia         = 1200;
      intervaloLigado    = 150UL;
      intervaloDesligado = 350UL;
      break;
    case 3: // critico - alarme o tempo todo
      frequencia         = 1800;
      intervaloLigado    = 100UL;
      intervaloDesligado = 100UL;
      break;
    default:
      return;
  }

  unsigned long agora  = millis();
  unsigned long espera = buzzerAtivo ? intervaloLigado : intervaloDesligado;

  if (agora - ultimoPulsoBuzzer >= espera) {
    ultimoPulsoBuzzer = agora;
    if (buzzerAtivo) {
      noTone(PINO_BUZZER);
      buzzerAtivo = false;
    } else {
      tone(PINO_BUZZER, frequencia);
      buzzerAtivo = true;
    }
  }
}

void exibirTelaInicial() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(18, 0);
  display.print(F("ORBITRACE"));

  display.setCursor(4, 12);
  display.print(F("Estacao Terrena v1.0"));

  display.drawLine(0, 22, 127, 22, SSD1306_WHITE);

  display.setCursor(0, 26);
  display.print(F("Satelites:"));

  display.setCursor(0, 36);
  display.print(F(" CBERS-4A  | 628 km"));

  display.setCursor(0, 46);
  display.print(F(" AMAZONIA-1| 752 km"));

  display.setCursor(0, 56);
  display.print(F(" SCD-2     | 750 km"));

  display.display();
}

void erroOLED() {
  Serial.println(F("ERRO: display OLED nao encontrado."));
  while (true) {
    digitalWrite(PINO_LED_ONBOARD, HIGH);
    delay(200);
    digitalWrite(PINO_LED_ONBOARD, LOW);
    delay(200);
  }
}

void atualizarDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // cabecalho
  display.setCursor(24, 0);
  display.print(F("ORBITRACE"));
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // satelite atual
  display.setCursor(0, 12);
  display.print(satelites[indiceSatelite].sigla);
  display.print(F("  "));
  display.print((int)satelites[indiceSatelite].altitude);
  display.print(F(" km"));

  // distancia
  display.setCursor(0, 22);
  display.print(F("Dist: "));
  display.print(distanciaAtual, 3);
  display.print(F(" km"));

  // probabilidade
  display.setCursor(0, 32);
  display.print(F("P(d): "));
  display.print(probColisao * 100.0f, 1);
  display.print(F("%"));

  display.drawLine(0, 41, 127, 41, SSD1306_WHITE);

  // bloco de risco invertido
  display.fillRect(0, 43, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(12, 46);
  display.print(F("RISCO: "));
  const char* labels[] = { "BAIXO  ", "MEDIO  ", "ALTO   ", "CRITICO" };
  display.print(labels[nivelRisco]);
  display.setTextColor(SSD1306_WHITE);

  // dados orbitais - y=56 (ultimo pixel em y=63, dentro do limite de 64px)
  display.setCursor(0, 56);
  display.print(F("T:"));
  display.print(periodoOrbital, 1);
  display.print(F("m v:"));
  display.print(velocidadeOrbital, 2);
  display.print(F("km/s"));

  display.display();
}
