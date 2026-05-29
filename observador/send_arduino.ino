#include "send_arduino.h"

void enviar_datos(info_enviar_t info){  
  Serial.write("abcd");

  float mediciones[] = { 
    info.accion_control,
    
    info.theta_medido,
    info.theta_estimado,

    info.omega_medida,
    info.omega_estimada,

    info.posicion_medido,
    info.posicion_estimado,

    info.velocidad_estimada,
  };

  Serial.write((byte*) mediciones, sizeof(mediciones));
}
