const int cant_floats = 100;

byte informacion[4 * cant_floats];

unsigned long tiempo_acumulado = 0;
unsigned long contador = 0;
unsigned long tiempo_minimo = 1000000;
unsigned long tiempo_maximo = 0;

void setup(void) {
	Serial.begin(115200);

  for (int i = 0; i < cant_floats; i++) {
    informacion[4 * i + 0] = 'a';
    informacion[4 * i + 1] = 'b';
    informacion[4 * i + 2] = 'c';
    informacion[4 * i + 3] = 'd';   
  }
 
	delay(100);
}

void loop() {
  unsigned long tiempo_inicio = micros();
  
  matlab_send();

  unsigned long tiempo_transcurrido = micros() - tiempo_inicio;
  contador++;

  tiempo_acumulado += tiempo_transcurrido;
  if (tiempo_transcurrido > 200)
    tiempo_minimo = min(tiempo_minimo, tiempo_transcurrido);
  tiempo_maximo = max(tiempo_maximo, tiempo_transcurrido);

  unsigned long tiempo_promedio = tiempo_acumulado / contador;

  Serial.println("");
  Serial.println(tiempo_transcurrido);
  Serial.println(tiempo_promedio);
  Serial.println(tiempo_minimo);
  Serial.println(tiempo_maximo);
}

void matlab_send(){ 
  Serial.write(informacion, 4 * cant_floats);
}
