# TPs-TAC - Práctica N°4
Se tiene una identificación de la planta, con las mediciones de la plataforma sola con mediciones de 0 a 10, dado por $$
    P_d(s) = \frac{ 0.0243 }{ (s + 0.6505) (s + 0.7869) }
P =
 
  0.0083317 (s-100)^2
  --------------------
  (s^2 + 31.13s + 271)
$$

Vamos a plantear el controlador dado por $$
C = k / s, con k = 7.9433
$$

Discretizado obtenemos el controlador $$ 
Cd = 0.066834 * (z + 1) / (z - 1)
$$

En código, esto se traduce a 
```c
float accion_control(float accion_previo, float error_actual, float error_previo) {
    return accion_previo + GANANCIA_A * (error_actual + error_previo);
}
```