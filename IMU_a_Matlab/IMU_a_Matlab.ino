#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const float offset_giroscopio = -0.07175f;

const float alfa = 0.1f;

Adafruit_MPU6050 mpu;
float theta_giroscopio = 0.0f, theta_complementario = 0.0f;

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
  delay(100);
}

// Devuelve en radianes
float angulo_giroscopio(float theta_anterior, sensors_vec_t* velocidad_angular) {
  // theta_nuevo = theta_previo + omega_x * Delta t
  return theta_anterior + (velocidad_angular->x - offset_giroscopio) * ((float)(periodo_millis) / 1000.0f);
}

// Devuelve en radianes
float angulo_acelerometro(sensors_vec_t* aceleracion) {
  // tan(theta) = aceleracion.y / aceleracion.z;
   return atan2(aceleracion->y, aceleracion->z);
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
  
  // Enviar datos
  enviar_datos_sensor(&acelerometro.acceleration, &giroscopio.gyro, theta_giroscopio, theta_acelerometro, theta_complementario);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos_sensor(sensors_vec_t* aceleracion, sensors_vec_t* velocidad_angular, float theta_giro, float theta_acc, float theta_complementario){
  const int cant_mediciones = 9;
  
  // Enviar header
  Serial.write("abcd");

  float mediciones[cant_mediciones] = { 
    aceleracion->x, aceleracion->y, aceleracion->z,
    velocidad_angular->x, velocidad_angular->y, velocidad_angular->z,
    theta_giro, theta_acc, theta_complementario,
  };

  for (int i = 0; i < cant_mediciones; i++) {
    byte* b = (byte*) (mediciones + i);
    Serial.write(b, sizeof(float));
  }
}
