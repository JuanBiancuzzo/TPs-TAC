#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <math.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 500;
const unsigned int MAX_MICROS = 1475;  

const float MIN_ANGULO_RANGO = -42.0f; 
const float MAX_ANGULO_RANGO = 66.0f; 
 
const float MIN_ANGULO = -90.0f; 
const float MAX_ANGULO = 0.0f; 

const float AMPLITUD_PLATAFORMA = 0.3347f;
const float OFFSET_PLATAFORMA = 1.0843f;

const int PIN_SERVO = 9;

const float RADIANES_2_GRADOS = 57.2958f;
const float ALFA = 0.1f;

// Calibracion
const int PUNTOS_INTERMEDIOS = 7;
const int PUNTOS_ESPERA = 50;

Servo servo;  
Adafruit_MPU6050 mpu;

float theta_complementario = 0.0f;

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

float calcular_angulo_complementario(float theta_anterior, float delta_t, sensors_vec_t* velocidad_angular, sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
  float theta_acelerometro = RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z);

  // theta_nuevo = theta_previo + omega_x * Delta t
  float theta_giroscopio = theta_anterior + RADIANES_2_GRADOS * velocidad_angular->x * delta_t;
  
  return ALFA * theta_acelerometro + (1 - ALFA) * theta_giroscopio;
}

void rutina_calibracion_servo() {
  float delta_t_us = 0.0f;
  unsigned long tiempo_inicio = micros();
  
  sensors_event_t acelerometro, giroscopio, temp; // Probar si es necesario crear la referencia o puede ser NULL
  mpu.getEvent(&acelerometro, &giroscopio, &temp);
  theta_complementario = calcular_angulo_complementario(theta_complementario, 1000000.0f * delta_t_us, &giroscopio.gyro, &acelerometro.acceleration);

  unsigned long tiempo_actual = micros();
  delta_t_us = tiempo_actual - tiempo_inicio;
  tiempo_inicio = tiempo_actual;
  
  for (int i = 0; i < PUNTOS_INTERMEDIOS; i++) {
    float angulo = mapFloat(i, -1, PUNTOS_INTERMEDIOS, MIN_ANGULO_RANGO, MAX_ANGULO_RANGO); 
  
    mover_servo_general(angulo);
    for (int j = 0; j < PUNTOS_ESPERA; j++) {
      mpu.getEvent(&acelerometro, &giroscopio, &temp);
      theta_complementario = calcular_angulo_complementario(theta_complementario, 1000000.0f * delta_t_us, &giroscopio.gyro, &acelerometro.acceleration);
      delay(10); 

      tiempo_actual = micros();
      delta_t_us = tiempo_actual - tiempo_inicio;
      tiempo_inicio = tiempo_actual;
    }
    
    Serial.print("Angulo original: "); Serial.print(angulo);
    Serial.print(", Angulo medido: "); Serial.println(theta_complementario);
    
    tiempo_actual = micros();
    delta_t_us = tiempo_actual - tiempo_inicio;
    tiempo_inicio = tiempo_actual;
  }

  Serial.println("Terminado la calibracion");
  delay(100);
}

void setup() {  
  Serial.begin(115200);
  
  while (!Serial){
    delay(500);
  }

  // Setteo del servo
  servo.attach(PIN_SERVO);

  // Setteo de la imu
  
  if (!mpu.begin()) {
    Serial.println("No se pudo encontrar la IMU");
    while (1) 
      delay(10);  
  } 

  Serial.println("IMU encontrada!");
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
  
  delay(100);

  mover_servo_general(0.0f);
  delay(1000);

  // Punto 3
  // rutina_calibracion_servo();
}

/*
 1. Settear el minimo y el maximo de milisegundos para cada direccion (de 90 a -90)
    * Probar con distintos milisegundos, para los angulos y ver cuando no cambia mas 
 2. Settear el angulo minimo y maximo del servo (min_angulo_rango y max_angulo_rango)
    * Progresivamente aumentar los grados para encontrar los extremos  
 3. Settear la amplitud y el desfase 
    * Habiendo obtenido el rango de angulos posibles para el servo que define la plataforma 
      se generan N puntos que recorran los angulos intermedios y guarden el valor del angulo
      con la plataforma, e ir imprimiendolos
 4. Calcular el tiempo necesario para hacer un diferencial de 30° 
    * Setteando un angulo inicial, y haciendolo mover mas de 30° grados, y usando la medicion
      de la IMU obtener el tiempo necesario para superar los 30° grados.
    * Probar lo mismo con un tiempo de paso de 1Hz 
*/

bool ya_llego = false;
bool inicio = false;
unsigned long tiempo_start;

void loop() {
  unsigned long tiempo_inicio = micros();
  if (!inicio){
    inicio = true;
    tiempo_start = tiempo_inicio;
  }
   sensors_event_t acelerometro, giroscopio, temp; // Probar si es necesario crear la referencia o puede ser NULL
   mpu.getEvent(&acelerometro, &giroscopio, &temp);

   float delta_t = (float)(periodo_millis) / 1000.0f;
   theta_complementario = calcular_angulo_complementario(theta_complementario, delta_t, &giroscopio.gyro, &acelerometro.acceleration);
  
  // Punto 1
  // float angulo_servo = MIN_ANGULO; // probamos el minimo de angulo y vamos modificando el MIN_MICROS
  // float angulo_servo = MAX_ANGULO; // probamos el maximo de angulo y vamos modificando el MAX_MICROS
  // mover_servo_general(angulo_servo);

  // Punto 2
  // float angulo_servo = -42.0f; // probamos el angulo y vamos modificando MIN_ANGULO_RANGO y MAX_ANGULO_RANGO
  // mover_servo_general(angulo_servo);

  float angulo_servo = 1.0f;
  mover_servo_general(angulo_servo);

  if (!ya_llego && theta_complementario > 9.66f){
    ya_llego = true;
    unsigned long tiempo_total = micros() - tiempo_start;
    Serial.println(tiempo_total);
  }

  if (!ya_llego){
    Serial.println(theta_complementario);
  }
  
  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
