//
#include <TimerOne.h>

const int pinBot = 3;

int Sminutos = 10;
int Ssegundos = 10; //levar para eepron

int minutos;
int segundos;

byte flagContando = 0; // 0 aguardando,1 aviso de final de contagem, 2 contando
byte botState;

void setup()
{
  // Initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards
  pinMode(13, OUTPUT);

  Timer1.initialize(1000000); // set a timer of length 1000000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.attachInterrupt( timerIsr ); // attach the service routine here
}

void loop()
{


  if (flagContando == 1) {
    Tocabuzer();
    flagContando = 0;
    }

botState=digitalRead( pinBot );
    if (botState == 1) {
    Timer1.start();
    flagContando = 2;
    }
}

/// --------------------------
/// Interrupção do timer
/// --------------------------
void timerIsr()
{
  segundos--;
  if ((segundos == 0) && (minutos == 0)) {
    segundos = Ssegundos;
    minutos = Sminutos;
    flagContando = 1;
    Timer1.stop();
  } else if (segundos < 0) {
    segundos = 59;
    minutos--;
  }
  // Toggle LED
  digitalWrite( 13, digitalRead( 13 ) ^ 1 );
}

void Tocabuzer()
{

}
