
const int MIN_POTE = 0, MAX_POTE = 1023;
const int MIN_ANGLE = 0, MAX_ANGLE = 270;

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;
 
const int PIN_POTE = A0;


void setup() {
  Serial.begin(9600);  
  pinMode(PIN_POTE, INPUT);
}

void pote_tarea1() {
  unsigned long tiempo_inicio = micros();
  int valor_pote = analogRead(PIN_POTE);
  
  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  Serial.println(tiempo_transcurrido);
}

void pote_tarea2() {
  int valor_pote = analogRead(PIN_POTE);
  
  // int angulo = MIN_ANGLE + ((MAX_ANGLE - MIN_ANGLE) * (valor_pote - MIN_POTE)) / (MAX_POTE - MIN_POTE);
  long angulo = map(valor_pote, MIN_POTE, MAX_POTE, MIN_ANGLE, MAX_ANGLE);
  
  Serial.print(valor_pote);
  Serial.print(", ");
  Serial.println(angulo);
}

void pote_tarea3() {
  unsigned long tiempo_inicio = micros();

  // Sensado y procesamiento
  pote_tarea2();
  
  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;   

  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    // no se puede usar porque tiene un limite de 2^14 bits de resolución
    // delayMicroseconds((unsigned int)tiempo_espera); 
    delay(tiempo_espera / 1000);
  }
}

void loop() {
  pote_tarea3();  
}
