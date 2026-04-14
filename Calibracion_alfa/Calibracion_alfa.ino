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
const float offset_giroscopio = -0.07175f;

const float alfa = 0.1f;

const float ANGULO_POSITIVO = 30;
const float ANGULO_NEGATIVO = -30;
// CANT_ITERACIONES * 2 * periodo_millis / 1000 = tiempo que el servo esta en un angulo
const int CANT_ITERACIONES = 125; 

Adafruit_MPU6050 mpu;
Servo servo;
float theta_giroscopio = 0.0f, theta_complementario = 0.0f;

int contador = 0;
int signo = -1;

inline float clamp(float valor, float minimo, float maximo) {
  return max(minimo, min(maximo, valor));
}

inline float mapFloat(float valor, float min_in, float max_in, float min_out, float max_out) {
  return (valor - min_in) * (max_out - min_out) / (max_in - min_in) + min_out; 
}

void mover_servo(float angulo_plataforma) {
  mover_servo_general(AMPLITUD_PLATAFORMA * (angulo_plataforma - OFFSET_PLATAFORMA));
}

void mover_servo_general(float angulo_servo) {
  // Para cuando tengamos las amplitudes definidas, podemos juntar las dos funciones de
  //    la siguiente forma:
  // float angulo_servo = AMPLITUD_PLATAFORMA * (angulo_plataforma - OFFSET_PLATAFORMA); 
  float angulo_limitado = clamp(angulo_servo, MIN_ANGULO_RANGO, MAX_ANGULO_RANGO);
  unsigned int micros_servo = (unsigned int) mapFloat(angulo_limitado, MIN_ANGULO, MAX_ANGULO, MIN_MICROS, MAX_MICROS);
  servo.writeMicroseconds(micros_servo);
}

// Devuelve en grados
float angulo_giroscopio(float theta_anterior, sensors_vec_t* velocidad_angular) {
  // theta_nuevo = theta_previo + omega_x * Delta t
  return theta_anterior + RADIANES_2_GRADOS * (velocidad_angular->x - offset_giroscopio) * ((float)(periodo_millis) / 1000.0f);
}

// Devuelve en grados
float angulo_acelerometro(sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
   return RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z);
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

  mover_servo_general(0.0f);
  delay(1000);
}

void loop() {
  unsigned long tiempo_inicio = micros();

  // Lectura de los 7 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);

  // Calcular angulos
  float theta_acelerometro = angulo_acelerometro(&acelerometro.acceleration);
  theta_giroscopio = angulo_giroscopio(theta_giroscopio, &giroscopio.gyro);

  // Filtro complementario
  float theta_giroscopio_optimo = angulo_giroscopio(theta_complementario, &giroscopio.gyro);
  theta_complementario = alfa * theta_acelerometro + (1 - alfa) * theta_giroscopio_optimo;

  // Lograr generar una señal del servo
  float angulo = (contador >= 0) ? ANGULO_POSITIVO : ANGULO_NEGATIVO;
  mover_servo_general(angulo);

  contador += signo;
  if (abs(contador) >= CANT_ITERACIONES) {
    signo *= -1;
  }
  
  // Enviar datos
  enviar_datos_sensor(&giroscopio.gyro, angulo, theta_giroscopio, theta_acelerometro, theta_complementario);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos_sensor(sensors_vec_t* velocidad_angular, float angulo, float theta_giro, float theta_acc, float theta_complementario){
  const int cant_mediciones = 6;
  
  // Enviar header
  Serial.write("abcd");

  float mediciones[cant_mediciones] = { 
    theta_acc, theta_giro, theta_complementario, 
    velocidad_angular->x, (float) ((float)(periodo_millis) / 1000.0f),
    angulo,
  };

  // Enviar los floats como bytes
  Serial.write((byte*) mediciones, sizeof(float) * cant_mediciones);
}
