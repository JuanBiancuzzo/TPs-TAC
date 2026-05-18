#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Servo.h>
#include <NewPing.h>
#include <Wire.h>

const unsigned long periodo_millis = 20; 
const unsigned long periodo_micros = periodo_millis * 1000;

const unsigned int MIN_MICROS = 544;
const unsigned int MID_MICROS = 1472;  
const unsigned int DIFF_MICROS = MID_MICROS - MIN_MICROS;

const int MIN_ANGULO = -90; 
const int MID_ANGULO = 0; 
const int DIFF_ANGULO = MID_ANGULO - MIN_ANGULO;

const int MIN_ANGULO_RANGO = -42; 
const int MAX_ANGULO_RANGO = 66; 

// Se esta teniendo en cuenta que la funcion micros descarta los primeros 2 bits
// por lo que esta division entre enteros no causa ningun efecto sobre el resultado
const unsigned int MIN_MICROS_RANGO = MIN_MICROS + (unsigned int) (((MIN_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
// const unsigned int MAX_MICROS_RANGO = MIN_MICROS + (unsigned int) (((MAX_ANGULO_RANGO - MIN_ANGULO) * DIFF_MICROS) / DIFF_ANGULO);
const unsigned int MAX_MICROS_RANGO = 2152;

const float VELOCIDAD_CM_MICROS = 337.4f * 1e-4; // a 10 grados
const float CENTRO_PLATAFORMA = 16.06f;

const float RADIANES_2_GRADOS = 57.2958f;
const float ALFA = 0.07f;

const float K_P = 25.0f;
const float K_I = 0.02f;
const unsigned int ACCION_EQUILIBRIO = MID_MICROS;

const int PIN_SERVO = 9;

const int TRIGGER_PIN  = 11;  
const int ECHO_PIN     = 12;  
const int MAX_DISTANCE = 200; 

typedef struct {
  float err_actual;
  float err_previo;
  float acumulada;
  float posicion_ref;
} info_control_t;

typedef struct {
  float accion_control;
  float theta_plataforma; 
  float posicion_carro; 
  float error_posicion;
  unsigned long tiempo_transcurrido;
} info_enviar_t;

Adafruit_MPU6050 mpu;
Servo servo;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

info_control_t datos_control = {
  .err_actual = 0.0f,
  .err_previo = 0.0f,
  .acumulada = 0.0f, 
  .posicion_ref = 0.0f,  
};
float theta_complementario = 0.0f;

info_enviar_t info_enviar;

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
  float theta_acelerometro = RADIANES_2_GRADOS * atan2(aceleracion->y, aceleracion->z);

  // theta_nuevo = theta_previo + omega_x * Delta t
  float theta_giroscopio = theta_anterior + RADIANES_2_GRADOS * velocidad_angular->x * ((float)(periodo_millis) / 1000.0f);
  
  return ALFA * theta_acelerometro + (1 - ALFA) * theta_giroscopio;
}

float calcular_posicion(unsigned int tiempo_ida_vuelta_micros) {
  return CENTRO_PLATAFORMA - ((float) (tiempo_ida_vuelta_micros) * VELOCIDAD_CM_MICROS) / 2.0f;
}

float avanzar_control(float posicion_medida, info_control_t* control_prev) {
  // Calculo de la accion
  float error_actual = control_prev->posicion_ref - posicion_medida;
  float factor_integracion = control_prev->acumulada + \
    (periodo_millis * (control_prev->err_actual + control_prev->err_previo)) / 2.0f;
  float accion_actual = K_P * error_actual + K_I * factor_integracion;

  // Actualizacion de los datos de control
  control_prev->acumulada = factor_integracion;
  control_prev->err_previo = control_prev->err_actual;
  control_prev->err_actual = error_actual;

  // Es negativo porque el signo del servo (angulo) es disntito al signo de la posicion
  return accion_actual;
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
  unsigned long tiempo_inicio = micros();

  // Lectura de los 7 sensores
  sensors_event_t acelerometro, giroscopio, temperatura;
  mpu.getEvent(&acelerometro, &giroscopio, &temperatura);
  unsigned int tiempo_ida_vuelta_micros = sonar.ping();

  // Procesamiento de las mediciones
  theta_complementario = calcular_angulo_complementario(theta_complementario, &giroscopio.gyro, &acelerometro.acceleration);
  float posicion_carro = calcular_posicion(tiempo_ida_vuelta_micros);

  // Calculamos la accion a realizar
  float accion_control = avanzar_control(posicion_carro, &datos_control);

  // Actuamos sobre el servo
  unsigned int pwm_control = ACCION_EQUILIBRIO - (unsigned int)accion_control;
  mover_servo(pwm_control);
  
  // Enviar dato
  info_enviar.accion_control = accion_control,
  info_enviar.theta_plataforma = theta_complementario,
  info_enviar.posicion_carro = posicion_carro,
  info_enviar.error_posicion = datos_control.err_actual,
  enviar_datos(&info_enviar);

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  info_enviar.tiempo_transcurrido = tiempo_transcurrido;

  if (tiempo_transcurrido < periodo_micros) {
    unsigned long tiempo_espera = periodo_micros - tiempo_transcurrido;
    delay(tiempo_espera / 1000);
  }
}

void enviar_datos(info_enviar_t* info){  
  // Enviar header
  Serial.write("abcd");

  const int cant_mediciones = 5;
  float mediciones[cant_mediciones] = { 
    info->accion_control, info->theta_plataforma, 
    info->posicion_carro, info->error_posicion,
    (float)info->tiempo_transcurrido,
  };

  // Enviar los floats como bytes
  Serial.write((byte*) mediciones, sizeof(float) * cant_mediciones);
}
