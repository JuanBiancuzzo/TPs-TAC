#ifndef SEND_ARDUINO_H
#define SEND_ARDUINO_H

#define HEADER "abcd"

typedef struct {
  float accion_control;

  float theta_medido;
  float omega_medida;

  float tiempo_transcurrido;
} info_enviar_t;

void enviar_datos(info_enviar_t info);

#endif
