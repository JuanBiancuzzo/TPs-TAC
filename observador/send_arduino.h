#ifndef MATH_UTILS_H
#define MATH_UTILS_H

typedef struct {
  float accion_control;

  float theta_medido;
  float theta_estimado;

  float omega_medida;
  float omega_estimada;
} info_enviar_t;

void enviar_datos(info_enviar_t info);

#endif
