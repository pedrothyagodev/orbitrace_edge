# Orbitrace — Console de Estacao Terrena

Projeto desenvolvido para a Global Solution 2026 da FIAP, disciplina de Edge Computing & Computer Systems.
Curso de Engenharia de Software, 1 ano.
Tema: Industria Espacial — ODS 9 (Industria, Inovacao e Infraestrutura).

---

## Descricao do projeto

O Orbitrace e um sistema embarcado em Arduino que simula o console fisico de uma estacao terrena de monitoramento orbital. A ideia surgiu de um problema real: o Brasil tem satelites estrategicos em orbita — o CBERS-4A, o Amazonia-1 e o SCD-2 — que sao usados pelo INPE para monitorar desmatamento, queimadas e desastres naturais. O problema e que esses satelites — que orbitam entre 628 km e 752 km de altitude — dividem espaco com mais de 27.000 detritos rastreados pela NORAD, se movendo a 7,5 km/s. As ferramentas profissionais para monitorar isso custam mais de US$ 20.000 por ano em licenca, o que coloca universidades e pesquisadores completamente fora do jogo.

O Orbitrace e uma alternativa open source, gratuita e em portugues. O circuito recebe dados de conjuncao (aproximacao entre um detrito e um satelite), calcula o nivel de risco usando modelos matematicos reais e exibe tudo em tempo real num display OLED, com um semaforo de LEDs e alertas sonoros pelo buzzer.

Este projeto e a camada de Edge Computing de uma solucao maior que o grupo desenvolveu ao longo do semestre, integrando Python, Arduino, front-end e modelagem matematica.

---

## Objetivo da solucao

Levar os calculos de risco orbital para o nivel fisico, tornando o sistema operavel sem depender de computador — da mesma forma que funciona em uma estacao terrena real.

O console monitora 3 satelites brasileiros (CBERS-4A a 628 km, AMAZONIA-1 a 752 km e SCD-2 a 750 km), classifica o risco de colisao em 4 niveis (CRITICO, ALTO, MEDIO e BAIXO), calcula a probabilidade de colisao usando a funcao exponencial P(d) = e^(-d/0.5), calcula o periodo orbital pela 3 Lei de Kepler e a velocidade orbital pela lei da gravitacao, e exibe tudo no display com atualizacao a cada 3 segundos.

---

## Componentes utilizados

| Componente | Quantidade | Finalidade |
|---|---|---|
| Arduino Uno | 1 | Microcontrolador principal |
| Display OLED SSD1306 128x64 I2C | 1 | Exibicao dos dados em tempo real |
| LED verde 5mm | 1 | Indicador de risco BAIXO |
| LED amarelo 5mm | 1 | Indicador de risco MEDIO |
| LED laranja 5mm | 1 | Indicador de risco ALTO |
| LED vermelho 5mm | 1 | Indicador de risco CRITICO |
| Buzzer passivo | 1 | Alerta sonoro proporcional ao nivel de risco |
| Push button | 1 | Alterna entre os 3 satelites monitorados |
| Resistor 220 ohms | 4 | Protecao dos LEDs |
| Protoboard | 1 | Montagem do circuito |
| Jumpers | 20 aproximadamente | Conexoes entre os componentes |

---

## Explicacao do funcionamento

Quando o sistema liga, aparece uma tela de boot no OLED com os tres satelites monitorados. Depois de 2,5 segundos o monitoramento comeca automaticamente.

A cada 3 segundos o Arduino sorteia uma distancia de conjuncao entre 0,05 e 5,0 km, que representa o quao perto um detrito espacial esta do satelite selecionado. Com essa distancia o sistema calcula a probabilidade de colisao usando a formula P(d) = e^(-d/0.5), classifica o nivel de risco e atualiza o display, os LEDs e o buzzer.

O botao serve para trocar o satelite monitorado. Cada clique avanca para o proximo na sequencia CBERS-4A, AMAZONIA-1 e SCD-2, voltando para o inicio depois.

Os modelos matematicos implementados sao os seguintes:

Probabilidade de colisao:
P(d) = e^(-d / 0.5)
- d menor que 0.2 km: probabilidade acima de 67%, nivel CRITICO
- d entre 0.2 e 0.5 km: probabilidade entre 37% e 67%, nivel ALTO
- d entre 0.5 e 1.0 km: probabilidade entre 13% e 37%, nivel MEDIO
- d maior ou igual a 1.0 km: probabilidade abaixo de 13%, nivel BAIXO

Periodo orbital (3 Lei de Kepler):
T(h) = 2pi x raiz((R + h)^3 / mu)
R = 6371 km, mu = 398600.44 km^3/s^2

Velocidade orbital:
v(h) = raiz(mu / (R + h))

O comportamento dos LEDs e do buzzer muda conforme o nivel de risco:
- BAIXO: LED verde aceso, buzzer silencioso
- MEDIO: LED amarelo aceso, buzzer com pulso lento a cada 1,2 segundos
- ALTO: LED laranja aceso, buzzer com pulso rapido a cada 0,5 segundos
- CRITICO: LED vermelho aceso, buzzer com alarme frenetico a cada 0,15 segundos

---

## Estrutura do circuito

Conexoes dos pinos do Arduino:

```
A4  (SDA) → OLED SDA
A5  (SCL) → OLED SCL
3.3V      → OLED VCC
GND       → OLED GND

D6  → Resistor 220 ohms → LED Verde   (BAIXO)   → GND
D7  → Resistor 220 ohms → LED Amarelo (MEDIO)   → GND
D8  → Resistor 220 ohms → LED Laranja (ALTO)    → GND
D9  → Resistor 220 ohms → LED Vermelho (CRITICO) → GND

D10 → Buzzer (+) → GND
D2  → Botao → GND (usa INPUT_PULLUP interno, sem resistor externo)
```

O display SSD1306 usa o endereco I2C 0x3C, que e o padrao de fabrica da maioria dos modulos.

---

## Instrucoes de execucao

Para rodar no simulador Wokwi:

1. Acesse o link da simulacao: https://wokwi.com/projects/465683764519495681
2. Clique no botao de play para iniciar
3. O display OLED vai exibir a tela de boot e em seguida comecar o monitoramento do CBERS-4A
4. Observe os LEDs e o buzzer mudando conforme o nivel de risco a cada 3 segundos
5. Clique no botao azul do circuito para alternar entre os satelites
6. Abra o Serial Monitor no baud rate 9600 para ver o log completo de cada evento calculado

Para rodar em hardware fisico com Arduino real:

1. Instale as bibliotecas Adafruit SSD1306 e Adafruit GFX Library no Arduino IDE em Sketch, Include Library, Manage Libraries
2. Monte o circuito conforme o diagrama de pinos acima
3. Abra o arquivo orbitrace.ino no Arduino IDE
4. Selecione a placa Arduino Uno em Tools, Board
5. Selecione a porta correta em Tools, Port
6. Faca o upload com Ctrl+U

---

## Estrutura do repositorio

```
orbitrace-edge/
├── orbitrace.ino   — codigo-fonte principal em Arduino C++
├── diagram.json       — arquivo do circuito para o simulador Wokwi
└── README.md          — este arquivo
```

---

## Integrantes do grupo

| Nome | RM |
|---|---|
| Pedro Passos Corsini | 573493 |
| Pedro Thyago Araujo dos Santos | 570939 |
| Daniel Gomes Torres | 573436 |
| Henrique Lira | 571009 |

---

## Links

| Recurso | Link |
|---|---|
| Simulador Wokwi | https://wokwi.com/projects/465683764519495681 |
| Repositorio GitHub | *(inserir apos criar o repositorio)* |
| Landing Page | https://landing-page-orbital-watch.vercel.app/ |
| Web Development | https://d4nielt0rres.github.io/WEB-DEVELOPMENTE-ORBITAL-GS/ |

---

Global Solution 2026 — FIAP — Engenharia de Software — 1 Ano