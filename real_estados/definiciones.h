#ifndef DEFINICIONES_H
#define DEFINICIONES_H

const unsigned long periodo_millis = 20;
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 544;
const unsigned int MID_MICROS = 1472 - 10;
const unsigned int DIFF_MICROS = MID_MICROS - MIN_MICROS;
const unsigned int ACCION_EQUILIBRIO = MID_MICROS;

const int MIN_ANGULO = -90;
const int MID_ANGULO = 0;
const int DIFF_ANGULO = MID_ANGULO - MIN_ANGULO;
const float SESGO_THETA = -1.17;
const float SESGO_OMEGA = -0.0698;

const int MIN_ANGULO_RANGO = -42;
const int MAX_ANGULO_RANGO = 66;

// Se esta teniendo en cuenta que la funcion micros descarta los primeros 2 bits
// por lo que esta division entre enteros no causa ningun efecto sobre el
// resultado
const unsigned int MIN_MICROS_RANGO =
    MIN_MICROS +
    (unsigned int)(((MIN_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) /
                   DIFF_ANGULO);
// const unsigned int MAX_MICROS_RANGO = MIN_MICROS + (unsigned int)
// (((MAX_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
const unsigned int MAX_MICROS_RANGO = 2152;

const float RADIANES_2_GRADOS = 57.2958;
const float ALFA = 0.07;

#endif
