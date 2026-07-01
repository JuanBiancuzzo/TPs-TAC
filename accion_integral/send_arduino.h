#ifndef SEND_ARDUINO_H
#define SEND_ARDUINO_H

typedef struct {
  float accion_control;
  float referencia;
  float error;

  float theta_medido;
  float theta_estimado;

  float omega_medida;
  float omega_estimada;

  float posicion_medido;
  float posicion_estimado;

  float velocidad_estimada;

  float tiempo_transcurrido;
} info_enviar_t;

void enviar_datos(info_enviar_t info);

#endif
