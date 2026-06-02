#include "TimerOne.h"

typedef union{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

typedef struct {
  float err;
  float control;
  float control_prev;
} info_control_t;

const float c1 = 0.9976;
const float c2 = -2.4263;
const float p1 = 1;
const float p2 = 0.6667;

info_control_t info_prev = {
  .err = 0,
  .control = 0,
  .control_prev = 0,
};

float avanzar_control(float h, float h_ref) {
  float err = h_ref - h;
  float control = (p1 + p2) * info_prev.control \
    - p1 * p2 * info_prev.control_prev \
    + c2 * err \
    - c1 * c2 * info_prev.err;

  info_prev.err = err;
  info_prev.control_prev = info_prev.control;
  info_prev.control = control;

  return control;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  // Ajustar condiciones iniciales de trabajo
  static float u0=0.5, h_ref=0.4, h=0.45, u;
  static float Ts=1;
  FLOATUNION_t aux;
  static float sampling_period_ms = 1000*Ts;
  //=========================
  // Definir parametros y variables del control

  //=========================

  if (Serial.available() >= 4/* 8 */) {
 
    aux.number = getFloat();
    h = aux.number;
    // aux.number = getFloat();
    // h_ref = aux.number;
  }
  //=========================
  //CONTROL
  
  u = avanzar_control(h, h_ref) + u0;
  //=========================
    
  matlab_send(u, h_ref, u0);
  delay(sampling_period_ms);
}

void matlab_send(float u, float h, float u0){
  Serial.write("abcd");
  byte * b = (byte *) &u;
  Serial.write(b,4);
  b = (byte *) &h;
  Serial.write(b,4);
  b = (byte *) &u0;
  Serial.write(b,4);
}

float getFloat(){
    int cont = 0;
    FLOATUNION_t f;
    while (cont < 4 ){
        f.bytes[cont] = Serial.read() ;
        cont = cont +1;
    }
    return f.number;
}
