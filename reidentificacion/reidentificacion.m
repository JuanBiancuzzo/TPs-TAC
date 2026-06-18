%% Datos de lectura - Intento 1
intento = 1;
kp = 36;
pwmEquilibrio = 0;
plataformaEquilibrio = 0.3;
corrimiento = 0;
archivo = sprintf("identificacion_completa_%d_kp_%.2f", intento, kp);
archivo = replace(archivo, ".", "_");

%% Omega
intento = 1;
pwmEquilibrio = 0;
plataformaEquilibrio = 0.5;
corrimiento = 0;
archivo = sprintf("omega_%i", intento);

%% Datos de lectura - Positivo
pwmEquilibrio = 0;
plataformaEquilibrio = 0.43;
corrimiento = 0.52;
archivo = sprintf("identificacion_completa_positivo");

%% Datos de lectura - Negativo
pwmEquilibrio = 0;
plataformaEquilibrio = 0.1977;
corrimiento = 0.5234;
archivo = sprintf("identificacion_completa_negativo");

%% CON CARRITO MEDICION PREVIA
intento = 3;
pwmEquilibrio = 0;
corrimiento = -12.4327;
plataformaEquilibrio = -0.91;
archivo = sprintf("identificacion_plataforma_%d", intento);
%archivo = sprintf("identificacion_completa_%d_kp_%.2f", intento, kp);

%% Lectura de datos
path = sprintf("mediciones/%s.csv", archivo);
datos = readtable(path);

tiempoInicio = 0;
tiempoFinal = 4;
dt = 0.02;
inv_dt = 50;

largo = size(datos.Tiempo);
recorteInicio = max(1, ceil(tiempoInicio / dt));
recorteFinal = min(largo(1), ceil(tiempoFinal / dt));

tiempo = (datos.Tiempo - datos.Tiempo(recorteInicio));
tiempo = tiempo(recorteInicio:recorteFinal);
pwm = (datos.ControlPWM - pwmEquilibrio);
pwm = pwm(recorteInicio:recorteFinal);
angulo = (datos.ThetaMedido - plataformaEquilibrio);
angulo = angulo(recorteInicio:recorteFinal);
posicion = (datos.PosicionMedido + corrimiento);
posicion = posicion(recorteInicio:recorteFinal);

largo = size(tiempo);
largo = largo(1);

%% SERVO-BARRA ORDEN 2
delay = 2;
prueba_largo = largo - delay; % Delay de dos muestras
orden = 2;
desplazar = @(lista, k) desplazar_general(lista, k, orden, prueba_largo);

y = desplazar(angulo, 0);
X = zeros(prueba_largo - orden, 3);
X(:, 1) = desplazar(angulo, 2);
X(:, 2) = desplazar(angulo, 1);
X(:, 3) = desplazar(circshift(pwm, delay), 0);

D = regresion_lineal(X, y);

alfa0 = -1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = 1;
beta0 = D(3) * alfa0;

z = tf('z', dt);
P_sb_z_2 = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2));

a1 = -inv_dt * (2 * alfa2 + alfa1);
a0 = inv_dt^2 * (alfa2 + alfa1 + alfa0);
b0 = inv_dt^2 * beta0;

s = tf('s');
P_sb = b0 / (s^2 + a1*s + a0);
P_sb_delay = exp(-s * delay * dt) * P_sb;

%% Planta del informe 
b0 = 1214;
a0 = 3.798e4;
a1 = 3285;
a2 = 203.6;

s = tf('s');
P_sb_informe = b0 / (s^3 + a2*s^2 + a1*s + a0);

%% VERIFICACION SERVO-BARRA
salida_simulada = lsim(P_sb, pwm, tiempo);
salida_simulada_informe = lsim(P_sb_informe, pwm, tiempo);

subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'b-')
plot(tiempo, salida_simulada_informe, 'm-')
plot(tiempo, angulo, 'r-')
xlabel('tiempo[s]')
ylabel('ángulo[°]')
title("Ángulo de barra")
legend("Simulada orden 2, con delay", "Simulada orden 3, del informe", "Real")

subplot(2, 1, 2);
hold on
grid on
plot(tiempo, pwm, 'm-')
xlabel('tiempo[s]')
ylabel('ancho[us]')
title("Ancho del PWM")

%% BARRA-CARRITO ORDEN 2 FORZANDO POLO EN EL ORIGEN
delay = 8; % 6
prueba_largo = largo - delay; % Delay de dos muestras
orden = 2;
desplazar = @(lista, k) desplazar_general(lista, k, orden, prueba_largo);

y = desplazar(posicion, 0);
X = zeros(prueba_largo - orden, 3);
X(:, 1) = desplazar(posicion, 2);
X(:, 2) = desplazar(posicion, 1);
X(:, 3) = desplazar(circshift(angulo, delay), 0);

D = regresion_lineal(X, y);

alfa0 = -1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = 1;
beta0 = D(3) * alfa0;

z = tf('z', dt);
P_bc_z = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2));

a1 = inv_dt * (2 * alfa2 + alfa1);
a0 = inv_dt^2 * (alfa2 + alfa1 + alfa0);
b0 = -inv_dt^2 * beta0;

s = tf('s');
P_bc = b0 / (s^2 + a1*s + a0);
P_bc_delay = (1-(delay*dt*s)/2) / (1+(delay*dt*s)/2)  * P_bc;

%% Modelo previo
s = tf('s');
%P_bc_prev = -385.3 / (s^2 + 168.7 * s);
P_bc_prev = -1222 / (s*(s+300));
P_bc_delay_prev = exp(-s * 0.28) * P_bc_prev;

%% Modelo propuesto
s = tf('s');
P_bc_m = -300 / ((s + 50) * (s + 1));
P_bc_delay_m = (1-(delay*dt*s)/2) / (1+(delay*dt*s)/2)  * P_bc_m;

%% VERIFICACION BARRA-CARRITO
salida_simulada = lsim(P_bc_delay, angulo, tiempo);
salida_simulada_prev = lsim(P_bc_delay_prev, angulo, tiempo);
salida_simulada_m = lsim(P_bc_delay_m, angulo, tiempo);
 
subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada_m, 'b-')
plot(tiempo, salida_simulada_prev, 'g-')
plot(tiempo, salida_simulada, 'm-')
plot(tiempo, posicion, 'r-')
xlabel('tiempo[s]')
ylabel('posicion[cm]')

title("Posición del carrito")
legend("Simulada modificada", "Simulacion previa", "Simulada", "Real")

subplot(2, 1, 2);
hold on
grid on
plot(tiempo, angulo, 'm-')
xlabel('tiempo[s]')
ylabel('ángulo[°]')
title("Ángulo de la barra")

%% VERIFICACION COMPLETA
P_completo_delay = P_bc_delay * P_sb_delay;
salida_simulada_completa = lsim(2 * exp(-s * 6 * dt) * P_completo_delay, pwm, tiempo);
salida_simulada_parcial = lsim(P_sb_delay, pwm, tiempo);

subplot(3, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada_completa, 'b-')
plot(tiempo, posicion, 'r-')
title("Posición del carrito")
legend("Simulada", "Real")

subplot(3, 1, 2);
hold on
grid on

plot(tiempo, salida_simulada_parcial, 'b-')
plot(tiempo, angulo, 'm-')
title("Angulo de la plataforma")
legend("Simulada", "Real")

subplot(3, 1, 3);
hold on
grid on
plot(tiempo, pwm, 'b-')

%% VERIFICACION DE LA PLANTA POR REPRESENTACION EN ESPACIO DE ESTADOS

A = [   0       1    0        0    0     ;
    -188.8  -17.98   0        0    0     ;
        0       0    0        1    0     ;
     1222       0    0      -300  -2444  ;
     7.14       0    0        0   -7.14  ] ;
 
 B = [0; 6.15; 0; 0; 0];
 C = [0 0 1 0 0];
 D = 0;


Aa = [   0    1    0        0    0    0     ;
        0   -1    1        0    0    0     ;
-3.798e04 -3082.4 -202.6   0    0    0     ;
        0    0    0        0    1    0     ;
     1222    0    0        0  -300  -2444  ;
     7.14    0    0        0    0   -7.14  ] ;
 
 Bb = [0; 0; 1214; 0; 0; 0];
 Cc = [0 0 0 1 0 0];
 Dd = 0;
 
 P = ss(A, B, C, D);
 Pp = ss(Aa, Bb, Cc, Dd);
 
 posicion_sim = lsim(P, pwm, tiempo);
 posicion_sim_anterior = lsim(Pp, pwm, tiempo);
 
 figure;
 hold on
 grid on
 plot(tiempo, posicion_sim, 'm-');
 plot(tiempo, posicion_sim_anterior, 'g-');
 plot(tiempo, posicion, 'b-');
 legend('Sim nueva', 'Sim vieja', 'Real')

%% Funciones

function [D] = regresion_lineal(X, y)
    D = (X' * X) \ (X' * y);
end

function [ lista_k_menos ] = desplazar_general(lista, k, orden, largo)
    lista_k_menos = lista(orden-k+1:largo-k);
end

