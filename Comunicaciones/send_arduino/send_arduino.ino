#include "send_arduino.h"

#define TAM_MEDICIONES sizeof(info_enviar_t)
typedef union {
  info_enviar_t info;
  byte bytes[TAM_MEDICIONES];
} datos_t;

void enviar_datos(info_enviar_t info){  
  Serial.write(HEADER);

  datos_t mediciones;
  mediciones.info = info;

  Serial.write(mediciones.bytes, TAM_MEDICIONES);
}
