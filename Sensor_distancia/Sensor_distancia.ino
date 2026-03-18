#include <NewPing.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int VELOCIDAD_CM_MILLIS = 34;
const unsigned int MICRO_EN_MILLIS = 1000;

const int TRIGGER_PIN  = 11;  
const int ECHO_PIN     = 12;  
const int MAX_DISTANCE = 200; 

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

void setup() {
  Serial.begin(9600); 
}

void loop() {
  unsigned long tiempo_inicio = micros();

  // Sensado y procesamiento
  unsigned int tiempo_ida_vuelta_micros = sonar.ping();
  float distancia_cm = ((float)(tiempo_ida_vuelta_micros * VELOCIDAD_CM_MILLIS)) / ((float)(2 * MICRO_EN_MILLIS));
  
  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;   

  Serial.print("Distancia: "); Serial.print(distancia_cm);
  Serial.print(", tiempo de muestreo: "); Serial.println(tiempo_transcurrido); 

  // Espera para llegar que pasen 20ms
  tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
