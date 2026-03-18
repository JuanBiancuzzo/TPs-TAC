#include <Servo.h>

const unsigned long periodo_millis = 1000; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 600;
const unsigned int MEN_MICROS = 1600;
const unsigned int MAX_MICROS = 2 * MEN_MICROS - MIN_MICROS;

const int MIN_POTE = 0, MAX_POTE = 1023;
const int MIN_ANGLE = -90, MAX_ANGLE = 90;

const int PIN_POTE = A0, PIN_SERVO = 9;

Servo myservo;  

int medir_pote() {
  int valor_pote = analogRead(PIN_POTE);
  return map(valor_pote, MIN_POTE, MAX_POTE, MIN_ANGLE, MAX_ANGLE);
}

void mover_servo(int angulo) {
  unsigned int micros_servo = map(angulo, MIN_ANGLE, MAX_ANGLE, MIN_MICROS, MAX_MICROS);
  myservo.writeMicroseconds(micros_servo);
}

void setup() {
  Serial.begin(9600);  

  myservo.attach(PIN_SERVOR);
  pinMode(PIN_POTE, INPUT);
}

void loop() {
  unsigned long tiempo_inicio = micros();

  int angulo_pote = medir_pote();
  mover_servo(angulo_pote);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
