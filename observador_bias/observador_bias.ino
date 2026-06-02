#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <Wire.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 544;
const unsigned int MID_MICROS = 1472 - 10;  
const unsigned int DIFF_MICROS = MID_MICROS - MIN_MICROS;

const int MIN_ANGULO = -90; 
const int MID_ANGULO = 0; 
const int DIFF_ANGULO = MID_ANGULO - MIN_ANGULO;
const float SESGO_ANGULO = -1.17;

const int MIN_ANGULO_RANGO = -42; 
const int MAX_ANGULO_RANGO = 66; 

// Se esta teniendo en cuenta que la funcion micros descarta los primeros 2 bits
// por lo que esta division entre enteros no causa ningun efecto sobre el resultado
const unsigned int MIN_MICROS_RANGO = MIN_MICROS + (unsigned int) (((MIN_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
// const unsigned int MAX_MICROS_RANGO = MIN_MICROS + (unsigned int) (((MAX_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
const unsigned int MAX_MICROS_RANGO = 2152;

const float RADIANES_2_GRADOS = 57.2958f;
const float ALFA = 0.07f;

const unsigned int CONTROL_EQUILIBRIO = MID_MICROS;

const int PIN_SERVO = 9;

typedef struct {
  float theta;
  float velocidad;
  float bias;
} info_estimacion_t;

typedef struct {
  float pwm;
  float theta_medido; 
  float theta_estimado;
  float velocidad_medida;
  float velocidad_estimada;
  float bias_medido;
  float bias_estimado;
} info_enviar_t;

const int CANT_PRUEBAS = 7;
const int MAX_CANT_PWMS = 13; 
const float SEQ_PWMS[CANT_PRUEBAS][MAX_CANT_PWMS] = {
  { 0.0f, 100.0f, 200.0f, 300.0f, 200.0f, 100.0f, 0.0f, -100.0f, -200.0f, -300.0f, -200.0f, -100.0f, 0.0f },
  { 0.0f, 50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f, 250.0f, 200.0f, 150.0f, 100.0f, 50.0f, 0.0f },
  { 0.0f, 100.0f, 200.0f, 100.0f, 0.0f, -100.0f, -200.0f, -100.0, 0.0f },
  { 0.0f, 100.0f, 200.0f, 300.0f, 300.0f, 200.0f, 100.0f, 0.0f },
  { 0.0f, 100.0f, 0.0f, 100.0f, 0.0f, 100.0f, 0.0f },
  { 0.0f, 200.0f, 0.0f, 200.0f, 0.0f, 200.0f, 0.0f },
  { 0.0f, 300.0f, 0.0f, 300.0f, 0.0f, 300.0f, 0.0f },
}; 
const int SEQ_CANT[CANT_PRUEBAS] = {13, 13, 9, 8, 7, 7, 7};
const int NUM_PRUEBA = 5;

#define CANT_PWMS SEQ_CANT[NUM_PRUEBA]
#define PWMS SEQ_PWMS[NUM_PRUEBA]

const float A_d[3][3] = {
  { 1.0f, 0.02f, 0 }, 
  { -2.118, 1.1606, 0 },
  { 0, 0, 1},
};
const float B_d[3] = { 0.0f, 0.0641f, 0 };
const float C_d[2][3] = { 
  {1.0f, 0.0f, 0.0f},
  {0.0f, 1.0f, 1.0f},
};

const float L[3][2] = { 
  {0.8671, 0.0199},
  {-1.9911, 1.0222},
  {-0.1071, 0.0026},
};

// CANT_ITERACIONES * periodo_millis / 1000 = tiempo que el servo esta en un angulo
const int CANT_ITERACIONES = 100; 

Adafruit_MPU6050 mpu;
Servo servo;
float theta_complementario = 0.0f;

info_estimacion_t datos_estimados = {
  .theta = 0.0f,
  .velocidad = 0.0f,
};

int contador_iteracion = 0, contador_pwm = 0;

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
  float theta_acelerometro = RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z) - SESGO_ANGULO;

  // theta_nuevo = theta_previo + omega_x * Delta t
  float theta_giroscopio = theta_anterior + RADIANES_2_GRADOS * velocidad_angular->x * ((float)(periodo_millis) / 1000.0f);
  
  return ALFA * theta_acelerometro + (1 - ALFA) * theta_giroscopio;
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
  if (contador_pwm >= CANT_PWMS) {
    return;
  }
  
  unsigned long tiempo_inicio = micros();

  // Lectura de los 7 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);

  // Filtro complementario
  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.acceleration);

  // Lograr generar una señal del servo
  float pwm_control = PWMS[contador_pwm];
  mover_servo((unsigned int)(pwm_control + CONTROL_EQUILIBRIO));

  float bias_medido = 20;

  float x_hat[3] = {datos_estimados.theta, datos_estimados.velocidad, datos_estimados.bias};
  float y_medido[2] = { theta_complementario, giroscopio.gyro.x * RADIANES_2_GRADOS + bias_medido };

  float y_hat[2];
  for (int i = 0; i < 2; i++) {
    y_hat[i] = 0.0f;
    for (int j = 0; j < 3; j++) {
      y_hat[i] += C_d[i][j] * x_hat[j];
    }
  }

  float x_sig_hat[3];
  for (int i = 0; i < 3; i++) {
    x_sig_hat[i] = 0.0f;
    for (int j = 0; j < 3; j++) {
      x_sig_hat[i] += A_d[i][j] * x_hat[j];  
    }
    x_sig_hat[i] += B_d[i] * pwm_control;
    for (int j = 0; j < 2; j++) {
      x_sig_hat[i] += L[i][j] * (y_medido[j] - y_hat[j]);  
    }
  }

  datos_estimados.theta = x_sig_hat[0];
  datos_estimados.velocidad = x_sig_hat[1];
  datos_estimados.bias = x_sig_hat[2];

  if (contador_iteracion >= CANT_ITERACIONES) {
    contador_iteracion = 0;
    contador_pwm++;
  }

  contador_iteracion++;
  
  // Enviar datos
  enviar_datos({
    .pwm = pwm_control,
    .theta_medido = theta_complementario,
    .theta_estimado = datos_estimados.theta,
    .velocidad_medida = giroscopio.gyro.x * RADIANES_2_GRADOS,
    .velocidad_estimada = datos_estimados.velocidad,
    .bias_medido = bias_medido,
    .bias_estimado = datos_estimados.bias,
  });

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos(info_enviar_t info){  
  // Enviar header
  Serial.write("abcd");

  const int cant_mediciones = 7;
  float mediciones[cant_mediciones] = { 
    info.pwm,
    info.theta_medido,
    info.theta_estimado,
    info.velocidad_medida,
    info.velocidad_estimada,
    info.bias_medido,
    info.bias_estimado,
  };

  // Enviar los floats como bytes
  Serial.write((byte*) mediciones, sizeof(float) * cant_mediciones);
}
