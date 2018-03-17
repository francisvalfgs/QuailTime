#include <Arduino.h>
#include <TimerOne.h>
#include <SevSeg.h>
#include <Wire.h>
#include <EEPROM.h>

SevSeg sevseg; //Instantiate a seven segment controller object

//protótipos
void timerIsr();
void trataBotoes();

// pinos de IO
const int pinTeclado = A0;
const int pinBuzer = A1;
const int pinLamp = A2;

// pinos do display
byte digitPins[] = {2, 3, 4, 5};
byte segmentPins[] = {6, 7, 8, 9, 10, 11, 12, 13};

// costantes diversas
const int minutosMax = 20;
const int segundosMax = 59;
const unsigned long intervalButton = 500; //tempo do botão ms
const unsigned long intervalMenu = 3000;  //tempo de retorno do menu ms
const unsigned long intervalBuzer = 2000; //tempo do Buzer ms

//Variaveis
struct ObjectEeprom
{
  int Sminutos;
  int Ssegundos;
};

ObjectEeprom varEeprom;

int eeAddress = 0;

volatile int minutos;
volatile int segundos;

byte prevSegundos;

volatile byte flagContando = 0; // 0 aguardando,1 aviso de final de contagem, 2 contando
byte prevflagContando = 0;

byte lastButtonState = 0;
byte menuState = 0;
unsigned long previousMillisButton;
unsigned long previousMillisMenu;
unsigned long previousMillisBuzer;

void setup()
{

  EEPROM.get(eeAddress, varEeprom);

  if ((varEeprom.Sminutos < 0) || (varEeprom.Sminutos > minutosMax) || (varEeprom.Ssegundos < 0) || (varEeprom.Ssegundos > segundosMax))
  {
    varEeprom.Sminutos = 10;
    varEeprom.Ssegundos = 10;
    EEPROM.put(eeAddress, varEeprom);
  }

  minutos = varEeprom.Sminutos;
  segundos = varEeprom.Ssegundos;
  sevseg.setNumber(minutos * 100 + segundos);

  byte numDigits = 4;
  bool resistorsOnSegments = true;    // resistors are on seguiments pins

// COMMON_CATHODE 0
// COMMON_ANODE 1
// N_TRANSISTORS 2
// P_TRANSISTORS 3
// NP_COMMON_CATHODE 1
// NP_COMMON_ANODE 0
  byte hardwareConfig = NP_COMMON_CATHODE; // See README.md for options COMMON_ANODE

  bool updateWithDelays = false;      // Default. Recommended
  bool leadingZeros = false;          // Use 'true' if you'd like to keep the leading zeros

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros);
  sevseg.setBrightness(90);

  Serial.begin(57600);

  // Initialize the digital pin

  pinMode(pinTeclado, INPUT);
  pinMode(pinBuzer, OUTPUT);
  pinMode(pinLamp, OUTPUT);

  Timer1.initialize(1000000);       // set a timer of length 1000000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt(timerIsr); // attach the service routine here
  Timer1.stop();
 }

void loop()
{
  if (segundos != prevSegundos) //atualiza display a cada segundo
  {
    Serial.print(minutos);
    Serial.print(":");
    Serial.println(segundos);
    prevSegundos = segundos;
    sevseg.setNumber(minutos * 100 + segundos,2);
  }

  sevseg.refreshDisplay(); //refress display

  if (flagContando != prevflagContando) //verifica se houve mudança de estatus do sistema
  {
    prevflagContando = flagContando;

    if (flagContando == 1)  // 1 aviso de final de contagem
    {
      digitalWrite(pinBuzer, HIGH); // Ativa Buzer
      digitalWrite(pinLamp, HIGH);  // Ativa lâmpada
      previousMillisBuzer = millis(); // inicia contagem de tempo para desligar buzer
    }
  }

  unsigned long currentMillisBuzer = millis();
  if ((currentMillisBuzer - previousMillisBuzer >= intervalBuzer) && (flagContando == 1))
  {
    previousMillisBuzer = currentMillisBuzer;
    digitalWrite(pinBuzer, LOW);  //desliga buzer
    flagContando = 0; // 0 aguardando,1 aviso de final de contagem, 2 contando
  }

  trataBotoes();
}

/// --------------------------
/// Interrupção do timer
/// --------------------------
void timerIsr()
{
  segundos--;
  if ((segundos == 0) && (minutos == 0))
  {
    segundos = varEeprom.Ssegundos;
    minutos = varEeprom.Sminutos;
    flagContando = 1; // 0 aguardando,1 aviso de final de contagem, 2 contando
    Timer1.stop();
  }
  else if (segundos < 0)
  {
    segundos = 59;
    minutos--;
  }
}

//*****************************************************************
//menu de controle e botões
void trataBotoes()
{
  //ler entrada AD das teclas
  int ADCteclado = analogRead(pinTeclado);
  byte buttonState;
  //associa o valor da conversão AD a tecla pressionada 
  if (ADCteclado < 60)
    buttonState = 1; // tecla pressionada iniciar contagem
  else if (ADCteclado < 500)
    buttonState = 2; // tecla pressionada programamar
  else if (ADCteclado < 900)
    buttonState = 3; // tecla pressionada ajuste
  else
    buttonState = 0; // nenhuma tecla pressionada

  //sai do menu caso nehuma tecla seja pressionada no tempo intervalMenu
  unsigned long currentMillisMenu = millis();
  if (currentMillisMenu - previousMillisMenu >= intervalMenu)
  {
    previousMillisMenu = currentMillisMenu;
    menuState = 0; // 0 funcionamento normal, 1 ajuste minutos, 2 ajuste segundos
  }
  
  //Reseta o tempo do menu caso as teclas de programação sejam pressionadas
  if ((buttonState == 2) || (buttonState == 3))  
  {
    previousMillisMenu = currentMillisMenu;
  }

//trata botões 
  if (buttonState != lastButtonState) // se houve mudança de estado dos botões
  {
    if (buttonState == 1) //se foi pressionado o botão de iniciar contagem
    {
      segundos = varEeprom.Ssegundos;
      minutos = varEeprom.Sminutos;
      flagContando = 2; // 0 aguardando,1 aviso de final de contagem, 2 contando
      Timer1.start();
      digitalWrite(pinLamp, LOW);
    }
    else if (buttonState == 2) //se foi pressionado o botão de programamar
    {
      segundos = varEeprom.Ssegundos;
      minutos = varEeprom.Sminutos;
      flagContando = 0; // 0 aguardando,1 aviso de final de contagem, 2 contando
      Timer1.stop();
      menuState++;
      if (menuState > 2)
      {
        menuState = 0;
        EEPROM.put(eeAddress, varEeprom); 
      }
    }
  }

//trata botão de ajuste de minutos e segundos
  unsigned long currentMillisButton = millis();
  if ((currentMillisButton - previousMillisButton >= intervalButton) || (buttonState != lastButtonState))
  {
    previousMillisButton = currentMillisButton;
    if ((buttonState == 3) && (menuState == 1)) // 0 funcionamento normal, 1 ajuste minutos, 2 ajuste segundos
    {
      varEeprom.Sminutos++;
      if (varEeprom.Sminutos > minutosMax)
        varEeprom.Sminutos = 0;
    }
    else if ((buttonState == 3) && (menuState == 2)) // 0 funcionamento normal, 1 ajuste minutos, 2 ajuste segundos
    {
      varEeprom.Ssegundos += 10;
      if (varEeprom.Ssegundos > 59)
        varEeprom.Ssegundos = 0;
    }
  }

  lastButtonState = buttonState;
}