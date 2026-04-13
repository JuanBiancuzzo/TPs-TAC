#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <math.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 1000; // 600
const unsigned int MAX_MICROS = 2000; // 2600

const float MIN_ANGULO_RANGO = -90.0f; // 43.0f;
const float MAX_ANGULO_RANGO = 90.0f; // 65.0f;
 
const float MIN_ANGULO = -90.0f; 
const float MAX_ANGULO = 90.0f; 

const float AMPLITUD_PLATAFORMA = 1.0f;
const float OFFSET_PLATAFORMA = 0.0f;

const int PIN_SERVO = 9;

const float RADIANES_2_GRADOS = 57.2958f;
const float ALFA = 0.1f;

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

float calcular_angulo_complementario(float theta_anterior, sensors_vec_t* velocidad_angular, sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
  float theta_acelerometro = atan2(aceleracion->y, aceleracion->z);
  theta_acelerometro *= RADIANES_2_GRADOS;

  // theta_nuevo = theta_previo + omega_x * Delta t
  float theta_giroscopio = theta_anterior + (velocidad_angular->x - offset_giroscopio) * ((float)(periodo_millis) / 1000.0f);
  theta_giroscopio *= RADIANES_2_GRADOS;
  
  return ALFA * theta_acelerometro + (1 - ALFA) * theta_giroscopio;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);  

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
const int PUNTOS_INTERMEDIOS = 10;


void loop() {
  unsigned long tiempo_inicio = micros();

  sensors_event_t acelerometro, giroscopio, temp; // Probar si es necesario crear la referencia o puede ser NULL
  mpu.getEvent(&acelerometro, &giroscopio, &temp);

  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.aceleration);

  // Punto 1
  float angulo_servo = -90.0f; // probamos el minimo de angulo y vamos modificando el MIN_MICROS
  // float angulo_servo = 90.0f; // probamos el maximo de angulo y vamos modificando el MAX_MICROS
  // mover_servo_general(angulo_servo);

  // Punto 2
  // float angulo_servo = 50.0f; // probamos el angulo y vamos modificando MIN_ANGULO_RANGO y MAX_ANGULO_RANGO
  // mover_servo_general(angulo_servo);

  // Punto 3

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
