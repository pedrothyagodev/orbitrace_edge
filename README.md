# Orbitrace — Console de Estação Terrena

Projeto desenvolvido para a Global Solution 2026 da FIAP, disciplina de Edge Computing & Computer Systems.
Curso de Engenharia de Software, 1º ano.
Tema: Indústria Espacial — ODS 9 (Indústria, Inovação e Infraestrutura).

---

## Descrição do projeto

O Orbitrace é um sistema embarcado em Arduino que simula o console físico de uma estação terrena de monitoramento orbital. A ideia surgiu de um problema real: o Brasil tem satélites estratégicos em órbita, o CBERS-4A, o Amazônia-1 e o SCD-2, que são usados pelo INPE para monitorar desmatamento, queimadas e desastres naturais. O problema é que esses satélites, que orbitam entre 628 km e 752 km de altitude, dividem espaço com mais de 27.000 detritos rastreados pela NORAD, se movendo a 7,5 km/s. As ferramentas profissionais para monitorar isso custam mais de US$ 20.000 por ano em licença, o que coloca universidades e pesquisadores completamente fora do jogo.

O Orbitrace é uma alternativa open source, gratuita e em português. O circuito recebe dados de conjunção (aproximação entre um detrito e um satélite), calcula o nível de risco usando modelos matemáticos reais e exibe tudo em tempo real num display OLED, com um semáforo de LEDs e alertas sonoros pelo buzzer.

Este projeto é a camada de Edge Computing de uma solução maior que o grupo desenvolveu ao longo do semestre, integrando Python, Arduino, front-end e modelagem matemática.

---

## Objetivo da solução

Levar os cálculos de risco orbital para o nível físico, tornando o sistema operável sem depender de computador, da mesma forma que funciona em uma estação terrena real.

O console monitora 3 satélites brasileiros (CBERS-4A a 628 km, AMAZÔNIA-1 a 752 km e SCD-2 a 750 km), classifica o risco de colisão em 4 níveis (CRÍTICO, ALTO, MÉDIO e BAIXO), calcula a probabilidade de colisão usando a função exponencial P(d) = e^(-d/0.5), calcula o período orbital pela 3ª Lei de Kepler e a velocidade orbital pela lei da gravitação, e exibe tudo no display com atualização a cada 3 segundos.

---

## Componentes utilizados

| Componente | Quantidade | Finalidade |
|---|---|---|
| Arduino Uno | 1 | Microcontrolador principal |
| Display OLED SSD1306 128x64 I2C | 1 | Exibição dos dados em tempo real |
| LED verde 5mm | 1 | Indicador de risco BAIXO |
| LED amarelo 5mm | 1 | Indicador de risco MÉDIO |
| LED laranja 5mm | 1 | Indicador de risco ALTO |
| LED vermelho 5mm | 1 | Indicador de risco CRÍTICO |
| Buzzer passivo | 1 | Alerta sonoro proporcional ao nível de risco |
| Push button | 1 | Alterna entre os 3 satélites monitorados |
| Resistor 220 ohms | 4 | Proteção dos LEDs |
| Protoboard | 1 | Montagem do circuito |
| Jumpers | 20 aproximadamente | Conexões entre os componentes |

---

## Explicação do funcionamento

Quando o sistema liga, aparece uma tela de boot no OLED com os três satélites monitorados. Depois de 2,5 segundos o monitoramento começa automaticamente.

A cada 3 segundos o Arduino sorteia uma distância de conjunção entre 0,05 e 5,0 km, que representa o quão perto um detrito espacial está do satélite selecionado. Com essa distância o sistema calcula a probabilidade de colisão usando a fórmula P(d) = e^(-d/0.5), classifica o nível de risco e atualiza o display, os LEDs e o buzzer.

O botão serve para trocar o satélite monitorado. Cada clique avança para o próximo na sequência CBERS-4A, AMAZÔNIA-1 e SCD-2, voltando para o início depois.

Os modelos matemáticos implementados são os seguintes:

Probabilidade de colisão:
P(d) = e^(-d / 0.5)
- d menor que 0.2 km: probabilidade acima de 67%, nível CRÍTICO
- d entre 0.2 e 0.5 km: probabilidade entre 37% e 67%, nível ALTO
- d entre 0.5 e 1.0 km: probabilidade entre 13% e 37%, nível MÉDIO
- d maior ou igual a 1.0 km: probabilidade abaixo de 13%, nível BAIXO

Período orbital (3ª Lei de Kepler):
T(h) = 2pi x raiz((R + h)^3 / mu)
R = 6371 km, mu = 398600.44 km^3/s^2

Velocidade orbital:
v(h) = raiz(mu / (R + h))

O comportamento dos LEDs e do buzzer muda conforme o nível de risco:
- BAIXO: LED verde aceso, buzzer silencioso
- MÉDIO: LED amarelo aceso, buzzer com pulso lento a cada 1,2 segundos
- ALTO: LED laranja aceso, buzzer com pulso rápido a cada 0,5 segundos
- CRÍTICO: LED vermelho aceso, buzzer com alarme frenético a cada 0,15 segundos

---

## Estrutura do circuito

Conexões dos pinos do Arduino:

```
A4  (SDA) → OLED SDA
A5  (SCL) → OLED SCL
3.3V      → OLED VCC
GND       → OLED GND

D6  → Resistor 220 ohms → LED Verde   (BAIXO)   → GND
D7  → Resistor 220 ohms → LED Amarelo (MÉDIO)   → GND
D8  → Resistor 220 ohms → LED Laranja (ALTO)    → GND
D9  → Resistor 220 ohms → LED Vermelho (CRÍTICO) → GND

D10 → Buzzer (+) → GND
D2  → Botão → GND (usa INPUT_PULLUP interno, sem resistor externo)
```

O display SSD1306 usa o endereço I2C 0x3C, que é o padrão de fábrica da maioria dos módulos.

---

## Instruções de execução

Para rodar no simulador Wokwi:

1. Acesse o link da simulação: https://wokwi.com/projects/465683764519495681
2. Clique no botão de play para iniciar
3. O display OLED vai exibir a tela de boot e em seguida começar o monitoramento do CBERS-4A
4. Observe os LEDs e o buzzer mudando conforme o nível de risco a cada 3 segundos
5. Clique no botão azul do circuito para alternar entre os satélites
6. Abra o Serial Monitor no baud rate 9600 para ver o log completo de cada evento calculado

Para rodar em hardware físico com Arduino real:

1. Instale as bibliotecas Adafruit SSD1306 e Adafruit GFX Library no Arduino IDE em Sketch, Include Library, Manage Libraries
2. Monte o circuito conforme o diagrama de pinos acima
3. Abra o arquivo orbitrace.ino no Arduino IDE
4. Selecione a placa Arduino Uno em Tools, Board
5. Selecione a porta correta em Tools, Port
6. Faça o upload com Ctrl+U

---

## Estrutura do repositório

```
orbitrace-edge/
├── orbitrace.ino   — código-fonte principal em Arduino C++
├── diagram.json       — arquivo do circuito para o simulador Wokwi
└── README.md          — este arquivo
```

---

## Integrantes do grupo

| Nome | RM |
|---|---|
| Pedro Passos Corsini | 573493 |
| Pedro Thyago Araújo dos Santos | 570939 |
| Daniel Gomes Torres | 573436 |
| Henrique Lira | 571009 |

---

## Links

| Recurso | Link |
|---|---|
| Simulador Wokwi | https://wokwi.com/projects/465683764519495681 |
| Repositório GitHub | https://github.com/pedrothyagodev/orbitrace_edge |
| Landing Page | https://landing-page-orbitrace.vercel.app/ |
| Web Development | https://d4nielt0rres.github.io/WEB-DEVELOPMENTE-ORBITAL-GS/ |

---

Global Solution 2026 — FIAP — Engenharia de Software — 1º Ano