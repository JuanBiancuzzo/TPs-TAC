#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const float alfa = 0.5f;
const float RADIAN_2_DEGREE = 57.296f;

Adafruit_MPU6050 mpu;
float theta_giroscopio = 0.0f, theta_acelerometro = 0.0f;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("Probando MPU6050");
  if (!mpu.begin()) {
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
  delay(100);
}

float angulo_giroscopio(float theta_anterior, sensors_vec_t* velocidad_angular) {
  // theta_nuevo = theta_previo + omega_x * Delta t
  return theta_anterior + velocidad_angular->x * ((float)(periodo_millis) / 1000.0f);
}

float angulo_acelerometro(sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
   return RADIAN_2_DEGREE * atan2(aceleracion->y, aceleracion->z);
}

void loop() {
  unsigned long tiempo_inicio = micros();

  // Lectura de los 7 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);

  // Calcular angulos
  theta_giroscopio = angulo_giroscopio(theta_giroscopio, &giroscopio.gyro);
  theta_acelerometro = angulo_acelerometro(&acelerometro.acceleration);

  // Filtro complementario
  float theta = alfa * theta_giroscopio + (1 - alfa) * theta_acelerometro;
  
  // Enviar datos
  enviar_datos_sensor(&acelerometro.acceleration, &giroscopio.gyro, theta_giroscopio, theta_acelerometro, theta);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos_sensor(sensors_vec_t* aceleracion, sensors_vec_t* velocidad_angular, float theta_giro, float theta_acc, float theta){
  const int cant_mediciones = 9;
  
  // Enviar header
  Serial.write("abcd");

  float mediciones[cant_mediciones] = { 
    aceleracion->x, aceleracion->y, aceleracion->z,
    velocidad_angular->x, velocidad_angular->y, velocidad_angular->z,
    theta_giro, theta_acc, theta,
  };

  for (int i = 0; i < cant_mediciones; i++) {
    byte* b = (byte*) (mediciones + i);
    Serial.write(b, sizeof(float));
  }
}
