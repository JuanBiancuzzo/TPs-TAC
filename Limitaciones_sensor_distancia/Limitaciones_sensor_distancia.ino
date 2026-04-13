#include <NewPing.h>

const unsigned long periodo_millis = 1000; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int VELOCIDAD_CM_MILLIS = 34;
const unsigned int MICRO_EN_MILLIS = 1000;

const int TRIGGER_PIN  = 11;  
const int ECHO_PIN     = 12;  
const int MAX_DISTANCE = 200; 

unsigned long tiempo_acumulado = 0;
unsigned long contador = 0;
unsigned long tiempo_minimo = 1000000;
unsigned long tiempo_maximo = 0;

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

void setup() {
  Serial.begin(115200); 
}

void loop() {
  unsigned long tiempo_inicio = micros();

  // Sensado y procesamiento
  unsigned int tiempo_ida_vuelta_micros = sonar.ping();
  float distancia_cm = ((float)(tiempo_ida_vuelta_micros * VELOCIDAD_CM_MILLIS)) / ((float)(2 * MICRO_EN_MILLIS));
  
  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;   
  contador++;

  tiempo_acumulado += tiempo_transcurrido;
  if (tiempo_transcurrido > 200) // para eliminar mediciones que no tienen sentido fisico
    tiempo_minimo = min(tiempo_minimo, tiempo_transcurrido);
  tiempo_maximo = max(tiempo_maximo, tiempo_transcurrido);

  unsigned long tiempo_promedio = tiempo_acumulado / contador;

  Serial.print("Distancia: "); Serial.println(distancia_cm);
  Serial.print("\tTiempo de muestreo: "); Serial.println(tiempo_transcurrido);
  Serial.print("\tTiempo de promedio: "); Serial.println(tiempo_promedio);
  Serial.print("\tTiempo de minimo:   "); Serial.println(tiempo_minimo);
  Serial.print("\tTiempo de maximo:   "); Serial.println(tiempo_maximo);

  // Espera para llegar que pasen 20ms
  tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
