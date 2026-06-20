#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <NewPing.h>
#include <Wire.h>

#include "send_arduino.h"
#include "definiciones.h"
#include "modelos.h"

const int PIN_SERVO = 9;
const int TRIGGER_PIN  = 11;  
const int ECHO_PIN     = 12;  

// CANT_ITERACIONES * periodo_millis / 1000 = tiempo que el servo esta en un angulo
const int CANT_ITERACIONES = 100; 
const float REFERENCIAS_POSICION[] = { 0 };
const float REFERENCIAS_THETA[] = { 0 };

const int NUM_REFERENCIAS_POSICION = sizeof(REFERENCIAS_POSICION) / sizeof(float);
const int NUM_REFERENCIAS_THETA = sizeof(REFERENCIAS_THETA) / sizeof(float);
const int NUM_REFERENCIAS = min(NUM_REFERENCIAS_POSICION, NUM_REFERENCIAS_THETA);

const float A_d[CANT_VARIABLES][CANT_VARIABLES] = {
  { +9.6659e-01, +1.6589e-02, +0.0000e+00, +0.0000e+00, +0.0000e+00}, 
  { -3.1320e+00, +6.6833e-01, +0.0000e+00, +0.0000e+00, +0.0000e+00}, 
  { +5.9297e-02, +4.8378e-04, +1.0000e+00, +3.3251e-03, -1.2778e-01}, 
  { +3.0592e+00, +5.0598e-02, +0.0000e+00, +2.4788e-03, -7.2137e+00}, 
  { +1.3154e-01, +1.2040e-03, +0.0000e+00, +0.0000e+00, +8.6688e-01}, 
};
const float B_d[CANT_VARIABLES] = { 
  +1.0882e-03, +1.0202e-01, +1.8417e-05, +2.9752e-03, +5.1576e-05 
};
const float C_d[CANT_MEDICIONES][CANT_VARIABLES] = { 
  { 0, 0, 1, 0, 0 }, 
  { 1, 0, 0, 0, 0 }, 
};

const float L_T[CANT_MEDICIONES][CANT_VARIABLES] = { 
  { +4.3161e-02, +9.1522e-02, +8.1068e-01, +3.5751e+00, -4.2696e-01 }, 
  { +5.5681e-01, -2.7933e+00, +1.4711e-01, +4.1089e+00, +5.0831e-03 },
};

const float K[CANT_VARIABLES] = {
  +5.3305e+01, +2.2338e+00, -1.6664e+02, -5.5282e-01, +1.5591e+02
};

const float F[CANT_REF] = { +2.3812e+02 };

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 
Adafruit_MPU6050 mpu;
Servo servo;

variables_estado_t variables_estimadas = { 0 };
float theta_complementario = 0, accion_control = 0;
unsigned long tiempo_transcurrido = 0;

int contador_iteracion = 0, contador_referencias = 0;

void mover_servo(unsigned int senial_pwm) {
  senial_pwm = max(MIN_MICROS_RANGO, min(MAX_MICROS_RANGO, senial_pwm));
  servo.writeMicroseconds(senial_pwm);
}

float calcular_angulo_complementario(float theta_anterior, sensors_vec_t* velocidad_angular, sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
  float theta_acelerometro = RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z) - SESGO_THETA;

  // theta_nuevo = theta_previo + omega_x * Delta t
  float theta_giroscopio = theta_anterior + RADIANES_2_GRADOS * (velocidad_angular->x - SESGO_OMEGA) * ((float)(periodo_millis) / 1000.0f);
  
  return ALFA * theta_acelerometro + (1 - ALFA) * theta_giroscopio;
}

float calcular_posicion(unsigned int tiempo_ida_vuelta_micros) {
  return CENTRO_PLATAFORMA - ((float) (tiempo_ida_vuelta_micros) * VELOCIDAD_CM_MICROS) / 2.0f;
}

variables_estado_t avanzar_observador(variables_estado_t x_hat, mediciones_t y_medido, float pwm_control) {
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

    x_sig_hat.vec[i] += B_d[i] * pwm_control;

    for (int j = 0; j < CANT_MEDICIONES; j++) {
      x_sig_hat.vec[i] += L_T[j][i] * (y_medido.vec[j] - y_hat.vec[j]);  
    }
  }

  return x_sig_hat;
}

float avanzar_control(variables_estado_t x_hat, ref_t referencia) {
  float control = 0;
  for (int i = 0; i < CANT_VARIABLES; i++) {
    control -= K[i] * x_hat.vec[i];
  }
  for (int i = 0; i < CANT_REF; i++) {
    control += F[i] * referencia.vec[i];
  }
  return control;
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
    contador_referencias = 0;
  }
  
  unsigned long tiempo_inicio = micros();

  // Lectura de los 8 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);
  unsigned int tiempo_ida_vuelta_micros = sonar.ping();

  // Procesamiento de las mediciones
  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.acceleration);
  float posicion_carro = calcular_posicion(tiempo_ida_vuelta_micros);

  mediciones_t mediciones = {{
    .posicion = posicion_carro,
    .theta = theta_complementario, 
  }};

  ref_t referencia = {{
    .posicion = REFERENCIAS_POSICION[contador_referencias],
    .theta = REFERENCIAS_THETA[contador_referencias],
  }};

  variables_estimadas = avanzar_observador(variables_estimadas, mediciones, accion_control);
  accion_control = avanzar_control(variables_estimadas, referencia);

  // Lograr generar una señal del servo
  mover_servo(CONTROL_EQUILIBRIO + (unsigned int)accion_control);
 
  // Enviar datos
  enviar_datos({
    .accion_control = accion_control,
    .referencia_posicion = referencia.posicion,
    .referencia_theta = referencia.theta,

    .theta_medido = mediciones.theta,
    .theta_estimado = variables_estimadas.theta,

    .omega_medida = (giroscopio.gyro.x - SESGO_OMEGA) * RADIANES_2_GRADOS,
    .omega_estimada = variables_estimadas.omega,

    .posicion_medido = mediciones.posicion,
    .posicion_estimado = variables_estimadas.posicion,

    .velocidad_estimada = variables_estimadas.velocidad,

    .tiempo_transcurrido = (float)tiempo_transcurrido,
  });

  if (contador_iteracion >= CANT_ITERACIONES) {
    contador_iteracion = 0;
    contador_referencias++;
  }

  contador_iteracion++;

  tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
