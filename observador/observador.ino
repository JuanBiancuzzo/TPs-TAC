#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <Wire.h>
#include <math.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 544;
const unsigned int MAX_MICROS = 1472;  

const float MIN_ANGULO_RANGO = -42.0f; 
const float MAX_ANGULO_RANGO = 66.0f; 
 
const float MIN_ANGULO = -90.0f; 
const float MAX_ANGULO = 0.0f; 

const float AMPLITUD_PLATAFORMA = 0.3347f;
const float OFFSET_PLATAFORMA = 1.0843f;

const int PIN_SERVO = 9;

const float RADIANES_2_GRADOS = 57.2958f;
const float ALFA = 0.12f;

const int CANT_PRUEBAS = 7;
const int MAX_CANT_ANGULOS = 13; 
const float SEQ_ANGULOS[CANT_PRUEBAS][MAX_CANT_ANGULOS] = {
  { 0.0f, 10.0f, 20.0f, 30.0f, 20.0f, 10.0f, 0.0f, -10.0f, -20.0f, -30.0f, -20.0f, -10.0f, 0.0f },
  { 0.0f, 5.0f, 10.0f, 15.0f, 20.0f, 25.0f, 30.0f, 25.0f, 20.0f, 15.0f, 10.0f, 5.0f, 0.0f },
  { 0.0f, 10.0f, 20.0f, 10.0f, 0.0f, -10.0f, -20.0f, -10.0, 0.0f },
  { 0.0f, 10.0f, 20.0f, 30.0f, 30.0f, 20.0f, 10.0f, 0.0f },
  { 0.0f, 10.0f, 0.0f, 10.0f, 0.0f, 10.0f, 0.0f },
  { 0.0f, 20.0f, 0.0f, 20.0f, 0.0f, 20.0f, 0.0f },
  { 0.0f, 30.0f, 0.0f, 30.0f, 0.0f, 30.0f, 0.0f },
}; 
const int SEQ_CANT[CANT_PRUEBAS] = {13, 13, 9, 8, 7, 7, 7};
const int NUM_PRUEBA = 6;

#define CANT_ANGULOS SEQ_CANT[NUM_PRUEBA]
#define ANGULOS SEQ_ANGULOS[NUM_PRUEBA]

const float A_d[2][2] = {
  { 1.0f, 0.02f }, 
  { -5.4200, 1.6226 },
};

const float B_d[2] = { 0.0f, 0.1666e-3 };
const float C_d[2] = { 1.0f, 0.0f };

const float L[2] = { 2.6227, 126.2258 };

// CANT_ITERACIONES * periodo_millis / 1000 = tiempo que el servo esta en un angulo
const int CANT_ITERACIONES = 100; 

Adafruit_MPU6050 mpu;
Servo servo;
float theta_complementario = 0.0f;

int contador_iteracion = 0, contador_angulo = 0;

const int ESTADO_THETA = 0;
const int ESTADO_VELOCIDAD = 1;
float estado[2] = { 0.0f, 0.0f };

inline float clamp(float valor, float minimo, float maximo) {
  return max(minimo, min(maximo, valor));
}

inline float mapFloat(float valor, float min_in, float max_in, float min_out, float max_out) {
  return (valor - min_in) * (max_out - min_out) / (max_in - min_in) + min_out; 
}

void mover_plataforma(float angulo_plataforma) {
  mover_servo(AMPLITUD_PLATAFORMA * (angulo_plataforma - OFFSET_PLATAFORMA));
}

void mover_servo(float angulo_servo) {
  // Para cuando tengamos las amplitudes definidas, podemos juntar las dos funciones de
  //    la siguiente forma:
  // float angulo_servo = AMPLITUD_PLATAFORMA * (angulo_plataforma - OFFSET_PLATAFORMA); 
  float angulo_limitado = clamp(angulo_servo, MIN_ANGULO_RANGO, MAX_ANGULO_RANGO);
  unsigned int micros_servo = (unsigned int) mapFloat(angulo_limitado, MIN_ANGULO, MAX_ANGULO, MIN_MICROS, MAX_MICROS);
  servo.writeMicroseconds(micros_servo);
}

float calcular_angulo_complementario(float theta_anterior, sensors_vec_t* velocidad_angular, sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
  float theta_acelerometro = RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z);

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

  mover_plataforma(0.0f);
  delay(1000);
}

void loop() {
  if (contador_angulo >= CANT_ANGULOS) {
    return;
  }
  
  unsigned long tiempo_inicio = micros();

  // Lectura de los 7 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);

  // Filtro complementario
  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.acceleration);

  // Lograr generar una señal del servo
  float angulo_control = ANGULOS[contador_angulo];
  mover_servo(angulo_control);

  float theta_estimado = (A_d[0][0] * estado[ESTADO_THETA] + A_d[0][1] * estado[ESTADO_VELOCIDAD]) \
    + B_d[0] * angulo_control \
    + L[0] * (theta_complementario - C_d[0] * estado[ESTADO_THETA]);

  float velocidad_estimado = (A_d[1][0] * estado[ESTADO_THETA] + A_d[1][1] * estado[ESTADO_VELOCIDAD]) \
    + B_d[1] * angulo_control \
    + L[1] * (theta_complementario - C_d[1] * estado[ESTADO_VELOCIDAD]);

  estado[ESTADO_THETA] = theta_estimado;
  estado[ESTADO_VELOCIDAD] = velocidad_estimado;

  if (contador_iteracion >= CANT_ITERACIONES) {
    contador_iteracion = 0;
    contador_angulo++;
  }

  contador_iteracion++;
  
  // Enviar datos
  enviar_datos(angulo_control, theta_complementario, theta_estimado, giroscopio.gyro.x, velocidad_estimado);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos(float control, float theta_complementario, float theta_estimado, float velocidad_angular, float velocidad_angular_estimado){  
  // Enviar header
  Serial.write("abcd");

  const int cant_mediciones = 5;
  float mediciones[cant_mediciones] = { 
    control, theta_complementario, theta_estimado,
    velocidad_angular, velocidad_angular_estimado,
  };

  // Enviar los floats como bytes
  Serial.write((byte*) mediciones, sizeof(float) * cant_mediciones);
}
