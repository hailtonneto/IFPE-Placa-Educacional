#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SevSeg.h>
#include <RTClib.h>
#include "Adafruit_ILI9341.h"
#include "Adafruit_GFX.h"

const uint8_t ROWS = 4;
const uint8_t COLS = 3;
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

uint8_t colPins[COLS] = { 4, 3, 2 };
uint8_t rowPins[ROWS] = { 8, 7, 6, 5 };

#define SENSOR_PRESENCA 9
#define TRIG_PIN 10
#define ECHO_PIN 11
#define TFT_DC 12
#define TFT_CS 13
#define LED 14
#define CIMA 15
#define ESQUERDA 16
#define BAIXO 17
#define DIREITA 18
#define ENABLE 44
#define STEP 46
#define DIR 47
#define BOTAO_POWER 48
#define DIR_BOTAO 49

const int VERMELHOS[] = {22, 24, 26, 28, 30};
const int VERDES[] = {23, 25, 27, 29, 31};
const int COLON_PIN = 43;

const int tempoVerde = 400;
const int tempoVermelho = 750;
int incremento = 1;

int letraAtual = 65;
int coluna = 0;
int linha = 0;

int steps_per_rev = 200;

bool sensorAtivo = false;
bool ultrassonicoAtivo = false;
bool semaforoAtivo = false;
bool mensageiro = false;
bool relogio = false;
bool motor = false;
bool motorEnabled = false;
bool motorDirection = true;

bool limparLCD = true;

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);
SevSeg sevseg;
RTC_DS1307 rtc;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SENSOR_PRESENCA, INPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(CIMA, INPUT_PULLUP);
  pinMode(BAIXO, INPUT_PULLUP);
  pinMode(ESQUERDA, INPUT_PULLUP);
  pinMode(DIREITA, INPUT_PULLUP);
  pinMode(COLON_PIN, OUTPUT);
  pinMode(ENABLE, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(BOTAO_POWER, INPUT_PULLUP);
  pinMode(DIR_BOTAO, INPUT_PULLUP);

  digitalWrite(COLON_PIN, HIGH);
  digitalWrite(ENABLE, HIGH);
  digitalWrite(STEP, LOW);
  digitalWrite(DIR, LOW);

  for (int pin : VERMELHOS) pinMode(pin, OUTPUT);
  for (int pin : VERDES) pinMode(pin, OUTPUT);

  byte numDigits = 4;
  byte digitPins[] = {32, 33, 34, 35};
  byte segmentPins[] = {36, 37, 38, 39, 40, 41, 42};
  bool resistorsOnSegments = true;
  bool updateWithDelays = false;
  bool leadingZeros = true;
  sevseg.begin(COMMON_ANODE, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros);
  sevseg.setBrightness(90);

  LCD.init();
  LCD.backlight();
  mostrarLCD("Placa", "Educacional");
  tft.begin();

  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
}

void limparDisplay() {
  if(limparLCD){
    LCD.clear();
    limparLCD = !limparLCD;
  }
}

void mostrarLCD(const char* linha1, const char* linha2) {
  LCD.setCursor(0, 0);
  LCD.print(linha1);
  LCD.setCursor(0, 1);
  LCD.print(linha2);
}

float distanciaCM() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  return pulseIn(ECHO_PIN, HIGH) / 58.0;
}

void verificarParada() {
  char key = keypad.getKey();
  if (key != NO_KEY && key == '#') {
    LCD.clear();
    tft.fillScreen(ILI9341_BLACK);
    LCD.noBlink();
    sevseg.blank();
    sensorAtivo = false;
    ultrassonicoAtivo = false;
    semaforoAtivo = false;
    mensageiro = false;
    relogio = false;
    motor = false;
    mostrarLCD("Modo", "Inativo");
  }
}

void verdeLoop() {
  if (!semaforoAtivo) return;

  for (int i = 0; i < 5; i++) {
    digitalWrite(VERDES[i], HIGH);
    delay(tempoVerde);
    digitalWrite(VERDES[i], LOW);
    delay(tempoVerde);
    verificarParada();
    if (!semaforoAtivo) return;
  }
}

void vermelhoLoop() {
  if (!semaforoAtivo) return;

  for (int i = 0; i < 5; i++) {
    digitalWrite(VERMELHOS[i], HIGH);
  }
  delay(2000);

  verificarParada();
  if (!semaforoAtivo) return;

  for (int i = 4; i >= 0; i--) {
    digitalWrite(VERMELHOS[i], LOW);
    delay(tempoVermelho);
    verificarParada();
    if (!semaforoAtivo) return;
  }
}

void paraCima(){
  if(letraAtual < 90){
    letraAtual++;
  }
  LCD.write(letraAtual);
  LCD.setCursor(coluna % 16, linha);
}

void paraBaixo(){
  if(letraAtual > 65){
    letraAtual--;
  }
  LCD.write(letraAtual);
  LCD.setCursor(coluna % 16, linha);
}

void setColon(bool value) {
  digitalWrite(COLON_PIN, value ? LOW : HIGH);
}

void displayTime() {
  DateTime now = rtc.now();
  bool blinkState = now.second() % 2 == 0;
  sevseg.setNumber(now.hour() * 100 + now.minute());
  setColon(blinkState);
}

void motorGirando(){
  digitalWrite(STEP, HIGH);
  delayMicroseconds(1000);
  digitalWrite(STEP, LOW);
  delayMicroseconds(1000);
}

void desligarMotor() {
  if (digitalRead(BOTAO_POWER) == 0) {
    motorEnabled = !motorEnabled;
    LCD.clear();
    delay(500);
  }
}

void mudarDirecao() {
  if (digitalRead(DIR_BOTAO) == 0) {
    motorDirection = !motorDirection;
    LCD.clear();
    delay(100);
  }
}

void mostrarTFT(int linha, int tamanho, const char* texto){
  tft.setCursor(0, linha);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(tamanho);
  tft.println(texto);
}

void letraASCII(int coluna, int linha, int valor){
  tft.setCursor(coluna,linha);
  tft.write(valor);
}

void menuTFT() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,3,"    Placa");
  mostrarTFT(30,3,"  Educativa");
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(80,1,"1.Sensor de presen a (PIR)");
  letraASCII(108,81,135);
  mostrarTFT(100,1,"2.Sensor de dist ncia (Ultrass nico)");
  letraASCII(96,100,131);
  letraASCII(180,100,147);
  mostrarTFT(120,1,"3.Sem foro");
  letraASCII(30,120,160);
  mostrarTFT(140,1,"4.Mensageiro");
  mostrarTFT(160,1,"5.Rel gio");
  letraASCII(30,160,162);
  mostrarTFT(180,1,"6.Motor de Passo");
  mostrarTFT(200,1,"#.Modo inativo (Sair)");
}

void sensorTFT() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,2,"     Sensor de");
  mostrarTFT(20,2,"   Presen a (PIR)");
  letraASCII(108,22,135);
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(60,1,"O sensor PIR (Passive Infrared Sensor)  detecta movimento por meio da radiacao  infravermelha emitida por objetos quen- tes,como seres humanos. Ele e amplamenteutilizado em sistemas de seguranca, ilu-minacao automatica e automacao residen- cial. Os sensores PIR sao eficientes em termos de energia e custo, mas possuem  limitacoes, como alcance e sensibilidadea fatores ambientais. Sua principal fun-cao e ativar dispositivos em resposta aomovimento, oferecendo solucoes praticas e seguras.");
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(240,2," # Menu Principal");
}

void distanciaTFT() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,2,"Sensor de dist ncia");
  letraASCII(168,0,131);
  mostrarTFT(20,2,"   (Ultrass nico)");
  letraASCII(132,20,147);
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(60,1," O sensor ultrassonico utiliza ondas so-noras de alta frequencia para medir dis-tancias, emitindo pulsos que calculam a distancia com base no tempo que levam   para retornar apos refletirem em um ob- jeto. Eh amplamente aplicado em robotica, sistemas de estacionamento, monitora- mento de nivel de liquidos e automacao  residencial. Suas vantagens incluem pre-cisao, ausencia de partes moveis e fun- cionamento em diversas condicoes ambien-tais. No entanto, pode ser sensivel a   interferencias sonoras e a precisao podeser afetada por fatores como temperaturae tipos de superficie. Em resumo, o sen-sor ultrassonico eh uma solucao eficaz eversatil para medicoes de distancia em  diversas aplicacoes tecnologicas.");
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(240,2," # Menu Principal");
}

void semaforo() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,2, "      Sem foro");
  letraASCII(110,0,160);
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(40,1," Os LEDs RGB (Red, Green, Blue) sao dio-dos emissores de luz que podem produzir uma ampla gama de cores ao combinar luz vermelha, verde e azul em diferentes in-tensidades. Essa versatilidade os torna populares em diversas aplicacoes, como  iluminacao decorativa, telas e displays. ");
  mostrarTFT(110,1, " Uma aplicacao pratica dos LEDs RGB e   nos semaforos. Em vez de usar lampadas  tradicionais, muitos semaforos modernos utilizam LEDs RGB, permitindo que cada  sinal (vermelho, amarelo e verde) seja  facilmente visivel e energeticamente    eficiente. Alem de reduzir o consumo de energia, os LEDs tem uma longa vida utile podem ser programados para alterar as cores, aumentando a flexibilidade e a   seguranca nas vias urbanas.");
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(240,2," # Menu Principal");
}

void mensageiroTFT() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,2, "     Mensageiro");
  mostrarTFT(20,2, "       (LCD)");
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(60,1, "O LCD (Liquid Crystal Display) eh uma   tecnologia de exibicao que utiliza cris-tais liquidos para produzir imagens. Eleeh amplamente utilizado em telas de te- levisores, monitores de computador,     smartphones e outros dispositivos ele-  tronicos. Os cristais liquidos sao ilu- minados por uma fonte de luz, geralmenteuma retroiluminacao LED, que passa atra-ves de filtros de cor para criar imagensnitidas e vibrantes. Os LCDs sao conhe- cidos por sua eficiência energetica,   baixo perfil e capacidade de exibir uma ampla gama de cores. Alem disso, ofere- cem boas caracteristicas de visualizacaoem ambientes iluminados, mas podem ter  limitacoes em angulos de visao e con-   traste, em comparacao com outras tecno- logias de display, como OLED.");
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(240,2," # Menu Principal");
}

void relogioTFT() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,2, "      Rel gio");
  letraASCII(105,0,162);
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(40,1, "O display de 7 segmentos e um dispositi-vo eletronico usado para exibir numeros e algumas letras. Ele eh composto por   sete LEDs dispostos em forma de figura  oito, que podem ser ativados em diferen-tes combinacoes para representar os di- gitos de 0 a 9 e algumas letras, como A, b, C, d, E e F. Esse tipo de display e amplamente utilizado em relogios digita-is, calculadoras, paineis de instrumen- tos e outros dispositivos que requerem  uma apresentacao simples de informacoes numericas. Os displays de 7 segmentos   podem ser de dois tipos: comuns, onde   todos os LEDs sao controlados individu- almente, e multiplexados, que economizamenergia ao ativar os segmentos de forma alternada. Eles sao valorizados pela    sua simplicidade, baixo custo e facili- dade de leitura.");
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(240,2," # Menu Principal");
}

void motorTFT() {
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(0,2, "   Motor de Passo");
  tft.setTextColor(ILI9341_WHITE);
  mostrarTFT(60,1, " O motor de passo eh um tipo de motor   eletrico que divide uma rotacao completaem passos discretos, permitindo controlepreciso de posicao e velocidade. Ele fun-ciona atraves de um conjunto de bobinasque, ao serem energizadas em sequencia, criam campos magneticos que fazem o ro- tor se mover em pequenos incrementos.   Isso o torna ideal para aplicacoes que  exigem precisao, como impressoras 3D,   robotica e CNC. Os motores de passo po- dem ser de dois tipos principais: unipo-lares e bipolares, cada um com caracte- risticas especificas em termos de torquee controle. Alem disso, sua facilidade de controle digital os torna populares emsistemas automatizados.");
  tft.setTextColor(ILI9341_PURPLE);
  mostrarTFT(240,2," # Menu Principal");
}

void loop() {
  char key = keypad.getKey();

  menuTFT();

  if (key != NO_KEY) {
    Serial.println(key);
    if (key == '1') {
      LCD.clear();
      tft.fillScreen(ILI9341_BLACK);
      sensorTFT();
      sensorAtivo = true;
      ultrassonicoAtivo = false;
      semaforoAtivo = false;
      mensageiro = false;
      relogio = false;
      motor = false;
      mostrarLCD("Sensor de", "Presenca");
      delay(2000);
      LCD.clear();
    } else if (key == '2') {
      LCD.clear();
      tft.fillScreen(ILI9341_BLACK);
      distanciaTFT();
      sensorAtivo = false;
      ultrassonicoAtivo = true;
      semaforoAtivo = false;
      mensageiro = false;
      relogio = false;
      motor = false;
      mostrarLCD("Sensor de", "Distancia");
      delay(2000);
      LCD.clear();
    } else if (key == '3') {
      LCD.clear();
      tft.fillScreen(ILI9341_BLACK);
      semaforo();
      sensorAtivo = false;
      ultrassonicoAtivo = false;
      semaforoAtivo = true;
      mensageiro = false;
      relogio = false;
      motor = false;
      mostrarLCD("Semaforo", "");
      delay(2000);
      LCD.clear();
    } else if (key == '4') {
      LCD.clear();
      tft.fillScreen(ILI9341_BLACK);
      mensageiroTFT();
      sensorAtivo = false;
      ultrassonicoAtivo = false;
      semaforoAtivo = false;
      mensageiro = true;
      relogio = false;
      motor = false;
      mostrarLCD("Mensageiro", "");
      delay(2000);
      LCD.clear();
    } else if (key == '5') {
      LCD.clear();
      tft.fillScreen(ILI9341_BLACK);
      relogioTFT();
      sensorAtivo = false;
      ultrassonicoAtivo = false;
      semaforoAtivo = false;
      mensageiro = false;
      relogio = true;
      motor = false;
      mostrarLCD("Relogio", "");
      delay(2000);
    } else if (key == '6') {
      LCD.clear();
      tft.fillScreen(ILI9341_BLACK);
      motorTFT();
      sensorAtivo = false;
      ultrassonicoAtivo = false;
      semaforoAtivo = false;
      mensageiro = false;
      relogio = false;
      motor = true;
      mostrarLCD("Motor de", "Passo");
      delay(2000);
    } else if (key == '#') {
      LCD.clear();
      mostrarLCD("Modo", "Inativo");
    }
  }

  while (sensorAtivo) {
    int valor = digitalRead(SENSOR_PRESENCA);
    digitalWrite(LED, valor);
    if (valor) {
      limparDisplay();
      mostrarLCD("Movimento", "Detectado");
    } else {
      if(!limparLCD){
        LCD.clear();
        limparLCD = !limparLCD;
      }
      mostrarLCD("Sem Movimento", "");
    }
    verificarParada();
    if (!sensorAtivo) return;
  }

  while (ultrassonicoAtivo) {
    mostrarLCD("Distancia em cm:", String(distanciaCM()).c_str());
    verificarParada();
    if (!ultrassonicoAtivo) return;
  }

  while (semaforoAtivo) {
    limparDisplay();
    mostrarLCD("Verde", "");
    verdeLoop();
    if (incremento == 5) {
      if(!limparLCD){
        LCD.clear();
        limparLCD = !limparLCD;
      }
      mostrarLCD("Vermelho", "");
      vermelhoLoop();
      incremento = 0;
    }
    incremento++;
  }

  while (mensageiro) {
    LCD.blink();
    LCD.setCursor(coluna, linha);

    while(digitalRead(CIMA) == 0){
      paraCima();
      delay(500);
    }
    
    while(digitalRead(BAIXO) == 0){
      paraBaixo();
      delay(500);
    }
    
    while(digitalRead(DIREITA) == 0){
      coluna++;
      if(coluna > 15){
        linha = 1;
      }
      LCD.setCursor(coluna % 16, linha);
      delay(500);
    }
    
    while(digitalRead(ESQUERDA) == 0){
      if(coluna > 0){
        coluna--;
      }
      if(coluna < 16){
        linha = 0;
      }
      LCD.setCursor(coluna % 16, linha);
      delay(500);
    }

    verificarParada();
    if (!mensageiro) return;
  }

  while (relogio) {
    sevseg.refreshDisplay();
    displayTime();

    verificarParada();
    if (!relogio) return;
  }

  while (motor) {
    limparDisplay();
    mostrarLCD("Motor", "Parado");
    verificarParada();
    if (!motor) return;

    desligarMotor();

    if (motorEnabled){
      digitalWrite(ENABLE, motorEnabled ? LOW : HIGH);
      
      mudarDirecao();

      if (motorDirection){
        digitalWrite(DIR, HIGH);

        for (int i = 0; i < steps_per_rev; i++){
          motorGirando();
          mostrarLCD("Sentido:", "Horario");
          verificarParada();
          if (!motor) return;
        }
      } else {
        digitalWrite(DIR, LOW);
        
        for (int i = 0; i < steps_per_rev; i++){
          motorGirando();
          mostrarLCD("Sentido:", "Anti-Horario");
          verificarParada();
          if (!motor) return;
        }
      }
    }
  }
}