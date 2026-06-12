#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <Wire.h>

#include "send_arduino.h"
#include "definiciones.h"
#include "modelos.h"

const int PIN_SERVO = 9;

// CANT_ITERACIONES * periodo_millis / 1000 = tiempo que el servo esta en un angulo
const int CANT_ITERACIONES = 100;
const float REFERENCIAS[] = {
  0,  3,  6,  9,  9,  6,  3,  0, 
  0, -3, -6, -9, -9, -6, -3,  0, 
};
const float NUM_REFERENCIAS = sizeof(REFERENCIAS) / sizeof(float);

const float A_d[CANT_VARIABLES][CANT_VARIABLES] = {
  {  1.0000, 0.0200 }, 
  { -3.7760, 1.3596 }, 
};
const float B_d[CANT_VARIABLES] = { 0, 0.1231 };
const float C_d[CANT_MEDICIONES][CANT_VARIABLES] = { 
  { 1, 0 }, 
};

const float L[CANT_VARIABLES][CANT_MEDICIONES] = { 
  { 2.0889 },
  { 71.1652 },
};
// const float K[CANT_VARIABLES] = { 
//   35.8825, 
//   10.1650,
// };
const float K[CANT_VARIABLES] = { 
  20, 
  0,
};

const float F[CANT_REF] = { 598.1 };
const float H[CANT_REF] = { 0 };

variables_estado_t variables_estiamdas = { 0 };
ref_t error_ref = { 0 };
float accion_control = 0;

float theta_complementario = 0;
int contador_iteracion = 0, contador_referencias = 0;

Adafruit_MPU6050 mpu;
Servo servo;

void mover_servo(unsigned int senial_pwm) {
  if (senial_pwm < MIN_MICROS_RANGO) {
    senial_pwm = MIN_MICROS_RANGO;
  } else if (senial_pwm > MAX_MICROS_RANGO) {
    senial_pwm = MAX_MICROS_RANGO;
  }
  servo.writeMicroseconds(senial_pwm);
}

float calcular_angulo_complementario(float theta_anterior, sensors_vec_t* velocidad_angular, sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
  float theta_acelerometro = RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z) - SESGO_THETA;

  // theta_nuevo = theta_previo + omega_x * Delta t
  float theta_giroscopio = theta_anterior + RADIANES_2_GRADOS * (velocidad_angular->x - SESGO_OMEGA) * ((float)(periodo_millis) / 1000.0f);
  
  return ALFA * theta_acelerometro + (1 - ALFA) * theta_giroscopio;
}

float avanzar_control(variables_estado_t x_hat) {
  float control = 0;
  for (int i = 0; i < CANT_VARIABLES; i++) {
    control += K[i] * x_hat.vec[i];
  }
  return control;
}

float avanzar_control_feedforward(variables_estado_t x_hat, ref_t referencia) {
  float control = 0;
  for (int i = 0; i < CANT_VARIABLES; i++) {
    control += K[i] * x_hat.vec[i];
  }
  for (int i = 0; i < CANT_REF; i++) {
    control += F[i] * referencia.vec[i];
  }
  return control;
}

float avanzar_control_accion_integral(variables_estado_t x_hat, ref_t q) {
  float control = 0;
  for (int i = 0; i < CANT_VARIABLES; i++) {
    control += K[i] * x_hat.vec[i];
  }
  for (int i = 0; i < CANT_REF; i++) {
    control += H[i] * q.vec[i];
  }
  return control;
}

variables_estado_t avanzar_observador(variables_estado_t x_hat, mediciones_t y_medido, float control) {
  mediciones_t y_hat = { 0 };
  for (int i = 0; i < CANT_MEDICIONES; i++) {
    for (int j = 0; j < CANT_VARIABLES; j++) {
      y_hat.vec[i] += C_d[i][j] * x_hat.vec[j];
    }
  }

  variables_estado_t x_sig_hat = { 0 };
  for (int i = 0; i < CANT_VARIABLES; i++) {
    for (int j = 0; j < CANT_VARIABLES; j++) {
      x_sig_hat.vec[i] += A_d[i][j] * x_hat.vec[j];  
    }

    x_sig_hat.vec[i] += B_d[i] * control;

    for (int j = 0; j < CANT_MEDICIONES; j++) {
      x_sig_hat.vec[i] += L[i][j] * (y_medido.vec[j] - y_hat.vec[j]);  
    }
  }

  return x_sig_hat;
}

ref_t avanzar_error_referencia(ref_t error_ref, ref_t referencia) {
  return error_ref;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("Probando MPU6050");
  if (!mpu.begin()) { // 0x72 -> para otro mpu6050 que no lo reconozca
    Serial.println("No lo veo al chip MPU6050, siento que es medio trucho");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Encontrado!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  Serial.println("MPU6050 Setteado!");

  // Setteo del servo
  servo.attach(PIN_SERVO);
  delay(100);
}

void loop() {
  if (contador_referencias >= NUM_REFERENCIAS) {
    return;
  }

  unsigned long tiempo_inicio = micros();

  // Lectura de los 8 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);

  // Procesamiento de las mediciones
  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.acceleration);

  mediciones_t mediciones = {{
    .theta = theta_complementario, 
  }};

  ref_t referencia = {{
    .theta = REFERENCIAS[contador_referencias],
  }};

  variables_estiamdas = avanzar_observador(variables_estiamdas, mediciones, accion_control);
  error_ref = avanzar_error_referencia(error_ref, referencia);

  // Control sin referencia o referencia nula 
  accion_control = avanzar_control(variables_estiamdas);

  // Control con referencia con matriz de Feedforward
  // accion_control = avanzar_control_feedforward(variables_estiamdas, referencia);

  // Control con referencia y accion integral
  // accion_control = avanzar_control_accion_integral(variables_estiamdas, error_ref);

  unsigned int pwm_control = ACCION_EQUILIBRIO - (unsigned int)accion_control;
  mover_servo(pwm_control);
 
  // Enviar datos
  enviar_datos({
    .accion_control = accion_control,
    .referencia = referencia.theta,

    .theta_medido = mediciones.theta,
    .theta_estimado = variables_estiamdas.theta,

    .omega_medida = (float)((giroscopio.gyro.x - SESGO_OMEGA) * RADIANES_2_GRADOS),
    .omega_estimada = variables_estiamdas.omega,
  });

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }

  if (contador_iteracion >= CANT_ITERACIONES) {
    contador_iteracion = 0;
    contador_referencias++;
  }
  contador_iteracion++;
}
