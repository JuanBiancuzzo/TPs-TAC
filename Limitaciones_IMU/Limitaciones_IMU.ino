#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

unsigned long tiempo_acumulado = 0;
unsigned long contador = 0;
unsigned long tiempo_minimo = 1000000;
unsigned long tiempo_maximo = 0;

Adafruit_MPU6050 mpu;

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); 

  if (!mpu.begin()) {
    Serial.println("No se encontro el MPU6050");
    while (1) {
      delay(10);
    }
  }
  
  Serial.println("Se encontro el MPU6050!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  delay(100);
}

void loop() {
  unsigned long tiempo_inicio = micros();

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  contador++;

  tiempo_acumulado += tiempo_transcurrido;
  if (tiempo_transcurrido > 200) // es para eliminar mediciones sin sentido
    tiempo_minimo = min(tiempo_minimo, tiempo_transcurrido);
  tiempo_maximo = max(tiempo_maximo, tiempo_transcurrido);

  unsigned long tiempo_promedio = tiempo_acumulado / contador;

  Serial.println("");
  Serial.println(tiempo_transcurrido);
  Serial.println(tiempo_promedio);
  Serial.println(tiempo_minimo);
  Serial.println(tiempo_maximo);
}
