#include "send_arduino.h"

#define TAM_MEDICIONES sizeof(info_enviar_t)
typedef union {
  info_enviar_t info;
  byte vec[TAM_MEDICIONES];
} datos_t;

void enviar_datos(info_enviar_t info){  
  Serial.write("abcd");

  datos_t mediciones;
  mediciones.info = info;

  Serial.write(mediciones.vec, TAM_MEDICIONES);
}
