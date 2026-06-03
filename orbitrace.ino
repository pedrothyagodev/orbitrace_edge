// ============================================================
// Projeto  : Orbitrace — Console de Estacao Terrena
// Disciplina: Edge Computing & Computer Systems
// Global Solution 2026 — 1 Semestre — FIAP
// ODS 9 — Industria, Inovacao e Infraestrutura
//
// Integrantes:
//   Pedro Passos Corsini           — RM: 573493
//   Pedro Thyago Araujo dos Santos — RM: 570939
//   Daniel Gomes Torres           — RM: 573436
//   Henrique Lira                  — RM: 571009
//
// Descricao:
//   Console fisico de monitoramento orbital que simula o
//   recebimento de dados de conjuncao (aproximacao entre
//   um detrito espacial e um satelite brasileiro) e exibe
//   o nivel de risco em tempo real via display OLED,
//   semaforo de LEDs e alertas sonoros com buzzer.
//
// Satelites monitorados:
//   CBERS-4A   — 628 km de altitude
//   AMAZONIA-1 — 752 km de altitude
//   SCD-2      — 750 km de altitude
//
// Niveis de risco (modelo exponencial P(d) = e^(-d/0.5)):
//   CRITICO  — d < 0.2 km  (P > 67%) — LED vermelho  + buzzer frenetico
//   ALTO     — d < 0.5 km  (P > 37%) — LED laranja   + buzzer rapido
//   MEDIO    — d < 1.0 km  (P > 13%) — LED amarelo   + buzzer lento
//   BAIXO    — d >= 1.0 km           — LED verde     + sem buzzer
// ============================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ============================================================
// CONFIGURACOES DO DISPLAY OLED
// ============================================================
#define OLED_LARGURA  128
#define OLED_ALTURA    64
#define OLED_RESET     -1
Adafruit_SSD1306 display(OLED_LARGURA, OLED_ALTURA, &Wire, OLED_RESET);

// ============================================================
// PINOS
// ============================================================
#define PINO_LED_VERDE    6
#define PINO_LED_AMARELO  7
#define PINO_LED_LARANJA  8
#define PINO_LED_VERMELHO 9
#define PINO_BUZZER      10
#define PINO_BOTAO        2
#define PINO_LED_ONBOARD 13

// ============================================================
// CONSTANTES FISICAS
// ============================================================
const float RAIO_TERRA = 6371.0f;      // km
const float MU         = 398600.4418f; // km^3/s^2
const float LAMBDA     = 0.5f;         // parametro lambda do modelo exponencial (km)
const float PI_VAL     = 3.14159265f;

// ============================================================
// DADOS DOS SATELITES BRASILEIROS
// ============================================================
struct Satelite {
  const char* nome;   // nome completo para Serial
  const char* sigla;  // nome curto para o OLED (<=9 chars em fonte 1)
  float       altitude; // km
  int         norad;
};

const int NUM_SATELITES = 3;
const Satelite satelites[NUM_SATELITES] = {
  { "CBERS-4A",   "CBERS-4A",  628.0f, 44883 },
  { "AMAZONIA-1", "AMAZ-1",    752.0f, 47699 },
  { "SCD-2",      "SCD-2",     750.0f, 25504 }
};

// ============================================================
// ESTADO GLOBAL — variaveis de dados (nao volateis)
// ============================================================
float distanciaAtual    = 0.0f;
float probColisao       = 0.0f;
int   nivelRisco        = 0;
float periodoOrbital    = 0.0f;
float velocidadeOrbital = 0.0f;

// ============================================================
// CONTROLE DE TEMPO — sem delay() bloqueante
// ============================================================
unsigned long ultimaAtualizacao = 0UL;
const unsigned long INTERVALO_ATUALIZACAO = 3000UL; // ms

// ============================================================
// CONTROLE DO BOTAO — debounce feito no loop(), nao na ISR
// ============================================================
// A ISR apenas seta a flag; o loop le e valida com millis().
volatile bool flagBotao     = false;
unsigned long ultimoBotao   = 0UL;
const unsigned long DEBOUNCE_MS = 250UL;

// Indice do satelite atual — lido/escrito apenas no loop()
// apos verificar e limpar flagBotao (sem race condition).
int indiceSatelite = 0;

// ============================================================
// ESTADO DO BUZZER — gerenciado exclusivamente em tocarBuzzer()
// ============================================================
unsigned long ultimoPulsoBuzzer = 0UL;
bool          buzzerAtivo       = false;

// ============================================================
// PROTOTIPOS
// ============================================================
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

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  // LEDs de saida
  pinMode(PINO_LED_VERDE,    OUTPUT);
  pinMode(PINO_LED_AMARELO,  OUTPUT);
  pinMode(PINO_LED_LARANJA,  OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_LED_ONBOARD,  OUTPUT);
  pinMode(PINO_BUZZER,       OUTPUT);

  // Botao com pull-up interno — ISR em FALLING (borda de descida)
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PINO_BOTAO), ISR_botao, FALLING);

  // Inicializa OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    erroOLED(); // pisca LED onboard infinitamente — nunca retorna
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Semente aleatoria a partir do ruido do pino analogico flutuante
  randomSeed(analogRead(A0));

  // Tela de boot
  exibirTelaInicial();
  delay(2500);

  // Primeira leitura
  atualizarSimulacao();
  ultimaAtualizacao = millis();
}

// ============================================================
// LOOP PRINCIPAL — sem delay() em nenhum ponto
// ============================================================
void loop() {
  unsigned long agora = millis();

  // --- Trata botao com debounce ---
  // flagBotao e setada pela ISR; validamos aqui com millis()
  // para evitar uso de millis() dentro da ISR (nao confiavel no AVR).
  if (flagBotao && (agora - ultimoBotao >= DEBOUNCE_MS)) {
    flagBotao       = false;
    ultimoBotao     = agora;
    indiceSatelite  = (indiceSatelite + 1) % NUM_SATELITES;
    atualizarSimulacao();
    ultimaAtualizacao = agora;
  }

  // --- Atualiza simulacao periodicamente ---
  if (agora - ultimaAtualizacao >= INTERVALO_ATUALIZACAO) {
    atualizarSimulacao();
    ultimaAtualizacao = agora;
  }

  // --- Gerencia buzzer de forma nao-bloqueante ---
  gerenciarBuzzer(nivelRisco);
}

// ============================================================
// ISR — Interrupcao do botao
// Apenas seta a flag. Nada mais. millis() nao e confiavel aqui.
// ============================================================
void ISR_botao() {
  flagBotao = true;
}

// ============================================================
// ATUALIZA TODA A SIMULACAO PARA O SATELITE ATUAL
// ============================================================
void atualizarSimulacao() {
  float h = satelites[indiceSatelite].altitude;

  distanciaAtual    = simularDistancia();
  probColisao       = calcularProb(distanciaAtual);
  nivelRisco        = classificarRisco(distanciaAtual);
  periodoOrbital    = calcularPeriodo(h);
  velocidadeOrbital = calcularVelocidade(h);

  // Reseta o estado do buzzer ao mudar de simulacao
  // para que gerenciarBuzzer() recomece o ciclo limpo.
  noTone(PINO_BUZZER);
  buzzerAtivo = false;

  atualizarLEDs(nivelRisco);
  atualizarDisplay();

  // Log serial
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

// ============================================================
// SIMULACAO DE DISTANCIA DE CONJUNCAO
// Distribuicao por faixas de risco com probabilidades realistas:
//   10% CRITICO | 15% ALTO | 25% MEDIO | 50% BAIXO
// ============================================================
float simularDistancia() {
  long sorteio = random(0L, 100L);
  float d;

  if (sorteio < 10L) {
    // CRITICO: 0.050 a 0.199 km
    d = 0.050f + (float)random(0L, 150L) / 1000.0f;
  } else if (sorteio < 25L) {
    // ALTO: 0.200 a 0.499 km
    d = 0.200f + (float)random(0L, 300L) / 1000.0f;
  } else if (sorteio < 50L) {
    // MEDIO: 0.500 a 0.999 km
    d = 0.500f + (float)random(0L, 500L) / 1000.0f;
  } else {
    // BAIXO: 1.000 a 5.000 km
    d = 1.000f + (float)random(0L, 4001L) / 1000.0f;
  }
  return d;
}

// ============================================================
// MODELO MATEMATICO — Probabilidade de colisao
// P(d) = e^(-d / lambda)   lambda = 0.5 km
// ============================================================
float calcularProb(float d) {
  return expf(-d / LAMBDA);
}

// ============================================================
// CLASSIFICACAO DO NIVEL DE RISCO
// ============================================================
int classificarRisco(float d) {
  if (d < 0.2f) return 3; // CRITICO
  if (d < 0.5f) return 2; // ALTO
  if (d < 1.0f) return 1; // MEDIO
  return 0;               // BAIXO
}

// ============================================================
// 3a LEI DE KEPLER — Periodo orbital em minutos
// T(h) = 2*pi * sqrt((R+h)^3 / mu)
// ============================================================
float calcularPeriodo(float h) {
  float a      = RAIO_TERRA + h;
  float t_seg  = 2.0f * PI_VAL * sqrtf((a * a * a) / MU);
  return t_seg / 60.0f;
}

// ============================================================
// VELOCIDADE ORBITAL em km/s
// v(h) = sqrt(mu / (R+h))
// ============================================================
float calcularVelocidade(float h) {
  float a = RAIO_TERRA + h;
  return sqrtf(MU / a);
}

// ============================================================
// CONTROLE DOS LEDs
// Apenas o LED correspondente ao nivel aceso; os demais apagados.
// ============================================================
void atualizarLEDs(int nivel) {
  digitalWrite(PINO_LED_VERDE,    (nivel == 0) ? HIGH : LOW);
  digitalWrite(PINO_LED_AMARELO,  (nivel == 1) ? HIGH : LOW);
  digitalWrite(PINO_LED_LARANJA,  (nivel == 2) ? HIGH : LOW);
  digitalWrite(PINO_LED_VERMELHO, (nivel == 3) ? HIGH : LOW);
}

// ============================================================
// GERENCIADOR DO BUZZER — nao-bloqueante com millis()
//
// Usa tone(pin, freq) SEM argumento de duracao para evitar
// conflito com o Timer2 interno da funcao tone(pin,freq,dur).
// O silenciamento e feito manualmente com noTone().
//
// Estados do ciclo para cada nivel:
//   buzzerAtivo=false -> chama tone() e seta buzzerAtivo=true
//   buzzerAtivo=true  -> chama noTone() e seta buzzerAtivo=false
// O intervalo entre transicoes e definido por 'intervalo'.
// ============================================================
void gerenciarBuzzer(int nivel) {
  if (nivel == 0) {
    // BAIXO: sem som. Garante silencio mesmo se nivel mudou.
    if (buzzerAtivo) {
      noTone(PINO_BUZZER);
      buzzerAtivo = false;
    }
    return;
  }

  // Parametros por nivel de risco
  unsigned long intervaloLigado;   // ms que o buzzer fica ligado
  unsigned long intervaloDesligado; // ms que o buzzer fica desligado
  unsigned int  frequencia;

  switch (nivel) {
    case 1:  // MEDIO — pulso lento
      frequencia        = 880;
      intervaloLigado   = 200UL;
      intervaloDesligado= 1000UL;
      break;
    case 2:  // ALTO — pulso rapido
      frequencia        = 1200;
      intervaloLigado   = 150UL;
      intervaloDesligado= 350UL;
      break;
    case 3:  // CRITICO — alarme frenetico
      frequencia        = 1800;
      intervaloLigado   = 100UL;
      intervaloDesligado= 100UL;
      break;
    default:
      return;
  }

  unsigned long agora     = millis();
  unsigned long espera    = buzzerAtivo ? intervaloLigado : intervaloDesligado;

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

// ============================================================
// TELA DE BOOT (exibida uma vez no setup)
// ============================================================
void exibirTelaInicial() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Layout: 6 linhas de texto x 8px = 48px + separador. Tudo dentro de 64px.
  // y= 0: titulo       "ORBITRACE"        (13ch=78px)
  // y=12: subtitulo    "Estacao Terrena v1.0" (20ch=120px)
  // y=22: drawLine separador
  // y=26: cabecalho    "Satelites:"           (10ch=60px)
  // y=36: satelite 1   " CBERS-4A  | 628 km" (19ch=114px)
  // y=46: satelite 2   " AMAZONIA-1| 752 km" (19ch=114px)
  // y=56: satelite 3   " SCD-2     | 750 km" (19ch=114px) -> y+8=64 OK

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

// ============================================================
// ERRO DE OLED — pisca LED onboard e trava (nunca retorna)
// Permite diagnosticar falha de hardware sem monitor serial.
// ============================================================
void erroOLED() {
  Serial.println(F("ERRO FATAL: display OLED nao encontrado."));
  while (true) {
    digitalWrite(PINO_LED_ONBOARD, HIGH);
    delay(200);
    digitalWrite(PINO_LED_ONBOARD, LOW);
    delay(200);
  }
}

// ============================================================
// ATUALIZACAO DO DISPLAY OLED (128x64 pixels, fonte tamanho 1)
//
// Layout das 8 linhas uteis (cada linha ocupa 8px de altura):
//
//   y= 0  [cabecalho] "ORBITRACE"
//   y= 9  [linha separadora]
//   y=12  [satelite]  sigla + altitude
//   y=22  [distancia] "Dist: X.XXX km"
//   y=32  [prob]      "P(d): XX.X%"
//   y=41  [linha separadora]
//   y=43  [bloco invertido] "RISCO: XXXXX"
//   y=56  [dados orbitais]  "T:XX.Xm  v:X.XXkm/s"  (y+8=64, limite exato)
// ============================================================
void atualizarDisplay() {
  display.clearDisplay();

  // Garante cor branca antes de qualquer print
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // --- Cabecalho ---
  display.setCursor(24, 0);
  display.print(F("ORBITRACE"));
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // --- Satelite e altitude ---
  display.setCursor(0, 12);
  display.print(satelites[indiceSatelite].sigla);
  display.print(F("  "));
  display.print((int)satelites[indiceSatelite].altitude);
  display.print(F(" km"));

  // --- Distancia de conjuncao ---
  display.setCursor(0, 22);
  display.print(F("Dist: "));
  display.print(distanciaAtual, 3);
  display.print(F(" km"));

  // --- Probabilidade de colisao ---
  display.setCursor(0, 32);
  display.print(F("P(d): "));
  display.print(probColisao * 100.0f, 1);
  display.print(F("%"));

  // --- Separador ---
  display.drawLine(0, 41, 127, 41, SSD1306_WHITE);

  // --- Bloco de risco invertido (texto preto em fundo branco) ---
  // Preenche o retangulo ANTES de mudar a cor do texto.
  display.fillRect(0, 43, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK); // texto preto sobre fundo branco
  display.setCursor(12, 46);
  display.print(F("RISCO: "));
  // Labels com largura fixa para centralizar visualmente
  const char* labels[] = { "BAIXO  ", "MEDIO  ", "ALTO   ", "CRITICO" };
  display.print(labels[nivelRisco]);
  // Restaura cor branca obrigatoriamente antes do proximo print
  display.setTextColor(SSD1306_WHITE);

  // --- Dados orbitais ---
  // y=56: ultimo pixel de texto em y=63 (56+8-1=63). Dentro do limite.
  display.setCursor(0, 56);
  display.print(F("T:"));
  display.print(periodoOrbital, 1);
  display.print(F("m v:"));
  display.print(velocidadeOrbital, 2);
  display.print(F("km/s"));

  // Envia buffer para o display
  display.display();
}
