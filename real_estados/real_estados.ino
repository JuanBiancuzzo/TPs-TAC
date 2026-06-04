#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <Wire.h>

#include "send_arduino.h"

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
// por lo que esta division entre enteros no causa ningun efecto sobre el resultado
const unsigned int MIN_MICROS_RANGO = MIN_MICROS + (unsigned int) (((MIN_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
// const unsigned int MAX_MICROS_RANGO = MIN_MICROS + (unsigned int) (((MAX_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
const unsigned int MAX_MICROS_RANGO = 2152;

const float RADIANES_2_GRADOS = 57.2958;
const float ALFA = 0.07;

const int PIN_SERVO = 9;

// const int CANT_ITERACIONES = 100;
// const float REFERENCIAS

#define CANT_VARIABLES 2
typedef union {
  struct {
    float theta;
    float omega;
  } nombres;
  float variables[CANT_VARIABLES];
} variables_estado_t;

#define CANT_MEDICIONES 1
typedef union {
  struct {
    float theta;
  } nombres;
  float variables[CANT_MEDICIONES];
} mediciones_t;

#define CANT_CONTROL 1
typedef union {
  struct {
    float pwm;
  } nombres;
  float variables[CANT_CONTROL];
} controles_t;

float theta_complementario = 0;

Adafruit_MPU6050 mpu;
Servo servo;

const float A_d[CANT_VARIABLES][CANT_VARIABLES] = {
  { 1, 0.02 }, 
  { -2.118, 1.1606 }, 
};
const float B_d[CANT_CONTROL][CANT_VARIABLES] = {
  { 0, 0.0641 },
};
const float C_d[CANT_MEDICIONES][CANT_VARIABLES] = { 
  { 1, 0 }, 
};

const float L[CANT_VARIABLES][CANT_MEDICIONES] = { 
  { 1.8899 },
  { 50.4383 },
};

const float K[CANT_VARIABLES][CANT_CONTROL] = { 
  { 10 }, 
  { .5 },
};

const float F = 953;

variables_estado_t variables_estiamdas = { 
  .nombres = {
    .theta = 0,
    .omega = 0,
  },
};

controles_t control = { 
  .nombres = {
    .pwm = 0,
  },
};

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

controles_t avanzar_control(variables_estado_t x_hat, float pwm_ref) {
  controles_t accion = { 0 }; 
  for (int i = 0; i < CANT_CONTROL; i++) {
    for (int j = 0; j < CANT_VARIABLES; j++) {
      accion.variables[i] += K[j][i] * x_hat.variables[j] + F * pwm_ref;
    }
  }
  return accion;
}

variables_estado_t avanzar_observador(variables_estado_t x_hat, mediciones_t y_medido, controles_t control) {
  mediciones_t y_hat = { 0 };
  for (int i = 0; i < CANT_MEDICIONES; i++) {
    for (int j = 0; j < CANT_VARIABLES; j++) {
      y_hat.variables[i] += C_d[i][j] * x_hat.variables[j];
    }
  }

  variables_estado_t x_sig_hat = { 0 };
  for (int i = 0; i < CANT_VARIABLES; i++) {
    for (int j = 0; j < CANT_VARIABLES; j++) {
      x_sig_hat.variables[i] += A_d[i][j] * x_hat.variables[j];  
    }

    for (int j = 0; j < CANT_CONTROL; j++) {
      x_sig_hat.variables[i] += B_d[j][i] * control.variables[j];
    }

    for (int j = 0; j < CANT_MEDICIONES; j++) {
      x_sig_hat.variables[i] += L[i][j] * (y_medido.variables[j] - y_hat.variables[j]);  
    }
  }

  return x_sig_hat;
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
  unsigned long tiempo_inicio = micros();

  // Lectura de los 8 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);

  // Procesamiento de las mediciones
  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.acceleration);

  mediciones_t mediciones = { 
    .nombres = {
      .theta = theta_complementario, 
    },
  };

  variables_estiamdas = avanzar_observador(variables_estiamdas, mediciones, control);
  control = avanzar_control(variables_estiamdas, 200);

  unsigned int pwm_control = ACCION_EQUILIBRIO - (unsigned int)control.nombres.pwm;
  mover_servo(pwm_control);
 
  // Enviar datos
  enviar_datos({
    .accion_control = control.nombres.pwm,

    .theta_medido = mediciones.nombres.theta,
    .theta_estimado = variables_estiamdas.nombres.theta,

    .omega_medida = (giroscopio.gyro.x - SESGO_OMEGA) * RADIANES_2_GRADOS,
    .omega_estimada = variables_estiamdas.nombres.omega,
  });

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
