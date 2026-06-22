#ifndef MODELOS_H
#define MODELOS_H

#define CANT_VARIABLES 5
typedef union {
  struct {
    float theta;
    float omega;
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

#define CANT_REF 1
typedef union {
  struct {
    float posicion;
  };
  float vec[CANT_REF];
} ref_t;

#endif
