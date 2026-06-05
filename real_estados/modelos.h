#ifndef MODELOS_H
#define MODELOS_H

#define CANT_VARIABLES 2
typedef union {
  struct {
    float theta;
    float omega;
  };
  float vec[CANT_VARIABLES];
} variables_estado_t;

#define CANT_MEDICIONES 1
typedef union {
  struct {
    float theta;
  };
  float vec[CANT_MEDICIONES];
} mediciones_t;

#endif
