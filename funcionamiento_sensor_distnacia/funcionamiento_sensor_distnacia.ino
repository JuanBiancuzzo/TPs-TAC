#include <NewPing.h>
#include <Wire.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 544;
const unsigned int MID_MICROS = 1472;  
const unsigned int DIFF_MICROS = MID_MICROS - MIN_MICROS;

const float VELOCIDAD_CM_MICROS = 337.4f * 1e-4;

const int TRIGGER_PIN  = 11;  
const int ECHO_PIN     = 12;  
const int MAX_DISTANCE = 200; 

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

float calcular_posicion(unsigned int tiempo_ida_vuelta_micros) {
  return ((float) (tiempo_ida_vuelta_micros) * VELOCIDAD_CM_MICROS) / 2.0f;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
}

void loop() {
  unsigned long tiempo_inicio = micros();

  unsigned int tiempo_ida_vuelta_micros = sonar.ping();
  float posicion_carro = calcular_posicion(tiempo_ida_vuelta_micros);

  enviar_datos((float) tiempo_ida_vuelta_micros, posicion_carro);
  Serial.print("Posicion: "); Serial.println(posicion_carro);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos(float tiempo, float posicion){  
  // Enviar header
  Serial.write("abcd");

  const int cant_mediciones = 2;
  float mediciones[cant_mediciones] = { tiempo, posicion };

  // Enviar los floats como bytes
  Serial.write((byte*) mediciones, sizeof(float) * cant_mediciones);
}
