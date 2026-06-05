#ifndef MODELOS_H
#define MODELOS_H

#define CANT_VARIABLES 6
typedef union {
  struct {
    float theta;
    float omega;
    float corriente;
    float posicion;
    float velocidad;
    float x_3;
  };
  float vec[CANT_VARIABLES];
} variables_estado_t;

#define CANT_MEDICIONES 2
typedef union {
  struct {
    float posicion;
    float theta;
  };
  float vec[CANT_MEDICIONES];
} mediciones_t;

#endif
