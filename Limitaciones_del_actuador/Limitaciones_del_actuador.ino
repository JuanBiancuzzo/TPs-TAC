#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 1000; // 600
const unsigned int MAX_MICROS = 2000; // 2600

const float MIN_ANGULO = -90.0f; // 43.0f;
const float MAX_ANGULO = 90.0f; // 65.0f;

const float AMPLITUD_PLATAFORMA = 1.0f;
const float OFFSET_PLATAFORMA = 0.0f;

const int PIN_SERVO = 9;

Servo servo;  
Adafruit_MPU6050 mpu;

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
  float angulo_limitado = clamp(angulo_servo, MIN_ANGULO, MAX_ANGULO);
  unsigned int micros_servo = (unsigned int) mapFloat(angulo_limitado, MIN_ANGULO, MAX_ANGULO, MIN_MICROS, MAX_MICROS);
  servo.writeMicroseconds(micros_servo);
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

void loop() {
  unsigned long tiempo_inicio = micros();

  sensors_event_t accelerometer, gyro, temp;
  mpu.getEvent(&accelerometer, &gyro, &temp);

  float angulo_servo = 0.0f;
  mover_servo_general(angulo_servo);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}
