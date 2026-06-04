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

// CANT_ITERACIONES * periodo_millis / 1000 = tiempo que el servo esta en un angulo
const int CANT_ITERACIONES = 100;
const float REFERENCIAS[] = {
  0,  3,  6,  9,  9,  6,  3,  0, 
  0, -3, -6, -9, -9, -6, -3,  0, 
};
const float NUM_REFERENCIAS = sizeof(REFERENCIAS) / sizeof(float);

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

float theta_complementario = 0;
int contador_iteracion = 0, contador_referencias = 0;

Adafruit_MPU6050 mpu;
Servo servo;

const float A_d[CANT_VARIABLES][CANT_VARIABLES] = {
  { 1, 0.02 }, 
  { -2.118, 1.1606 }, 
};
const float B_d[CANT_VARIABLES] = { 0, 0.0641 };
const float C_d[CANT_MEDICIONES][CANT_VARIABLES] = { 
  { 1, 0 }, 
};

const float L[CANT_VARIABLES][CANT_MEDICIONES] = { 
  { 1.8899 },
  { 50.4383 },
};

const float K[CANT_VARIABLES] = { 10,  0.5 };

const float F = 953;

variables_estado_t variables_estiamdas = { 
  .nombres = {
    .theta = 0,
    .omega = 0,
  },
};

float accion_control = 0;

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

float avanzar_control(variables_estado_t x_hat, float pwm_ref) {
  float control = 0;
  for (int i = 0; i < CANT_VARIABLES; i++) {
    control += K[i] * x_hat.variables[i] + F * pwm_ref;
  }
  return control;
}

variables_estado_t avanzar_observador(variables_estado_t x_hat, mediciones_t y_medido, float control) {
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

    x_sig_hat.variables[i] += B_d[i] * control;

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
  if (contador_referencias >= NUM_REFERENCIAS) {
    return;
  }

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

  float referencia = REFERENCIAS[contador_referencias];
  variables_estiamdas = avanzar_observador(variables_estiamdas, mediciones, accion_control);
  accion_control = avanzar_control(variables_estiamdas, referencia);

  unsigned int pwm_control = ACCION_EQUILIBRIO - (unsigned int)accion_control;
  mover_servo(pwm_control);
 
  // Enviar datos
  enviar_datos({
    .accion_control = accion_control,
    .referencia = referencia,

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

  if (contador_iteracion >= CANT_ITERACIONES) {
    contador_iteracion = 0;
    contador_referencias++;
  }
  contador_iteracion++;
}
