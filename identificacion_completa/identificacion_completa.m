%% Datos de lectura - Intento 1
intento = 1;
kp = 36;
pwmEquilibrio = 0;
plataformaEquilibrio = 0.3;
corrimiento = 0;
%% Datos de lectura - Positivo
pwmEquilibrio = 0;
plataformaEquilibrio = 0.43;
corrimiento = 0.52;
%% Datos de lectura - Negativo
pwmEquilibrio = 0;
plataformaEquilibrio = 0.1977;
corrimiento = 0.5234;
%% CON CARRITO MEDICION PREVIA
intento = 3;
pwmEquilibrio = 0;
corrimiento = -12.4327;
plataformaEquilibrio = -0.91;
%% Lectura de datos

%archivo = sprintf("identificacion_completa_%d_kp_%.2f", intento, kp);
%archivo = sprintf("identificacion_7_kp_45_00_ki_0_00");
archivo = sprintf("identificacion_completa_positivo");
path = sprintf("mediciones/%s.csv", replace(archivo, ".", "_"));
%path = sprintf("mediciones/identificacion_plataforma_%d.csv", intento);
datos = readtable(path);

tiempoInicio = 1;
tiempoFinal = 3.32;
%tiempoFinal = 2.5;
%tiempoFinal = 1000;

dt = 0.02;
inv_dt = 50;

largo = size(datos.Tiempo);
recorteInicio = max(1, ceil(tiempoInicio / dt));
recorteFinal = min(largo(1), ceil(tiempoFinal / dt));

tiempo = (datos.Tiempo - datos.Tiempo(recorteInicio));
tiempo = tiempo(recorteInicio:recorteFinal);
senialPwm = (datos.ControlPWM - pwmEquilibrio);
senialPwm = -senialPwm(recorteInicio:recorteFinal);
control = (datos.Plataforma - plataformaEquilibrio);
control = control(recorteInicio:recorteFinal);
posicion = (datos.Posicion + corrimiento);
posicion = posicion(recorteInicio:recorteFinal);

senial_servo = datos.ControlPWM(recorteInicio:recorteFinal);
senial_error = datos.Error(recorteInicio:recorteFinal);

largo = size(tiempo);
largo = largo(1);

% Correcciones
%control(ceil(3.8 / dt):ceil(8.0 / dt)) = 0;
%control(ceil(11 / dt):largo) = 0;

%posicion(1:ceil(1.2 / dt)) = 0;
%posicion(ceil(11 / dt):largo) = 0;

%% SERVO-BARRA ORDEN 3 SIN POLO EN EL ORIGEN
% senialPwm = -senialPwm;
orden = 3;
desplazar = @(lista, k) desplazar_general(lista, k, orden, largo);

y = desplazar(control, 0);
X = zeros(largo - orden, 4);
X(:, 1) = desplazar(control, 3);
X(:, 2) = desplazar(control, 1);
X(:, 3) = desplazar(control, 2);
X(:, 4) = desplazar(senialPwm, 0);

D = regresion_lineal(X, y);

alfa0 = 1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = -D(3) * alfa0;
alfa3 = -1;
beta0 = D(4) * alfa0;

z = tf('z', dt);
P_sb_z = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2) + alfa3 * z^(-3));

b0 = beta0 / (dt)^3;
a2 = 3/dt * alfa3 + 1/dt * alfa2;
a1 = -3/(dt)^2 * alfa3 - 2/(dt)^2 * alfa2 -1/(dt)^2 * alfa1;
a0 = 1/(dt)^3 * alfa3 + 1/(dt)^3 * alfa2 + 1/(dt)^3 * alfa1 + 1/(dt)^3 * alfa0;

s = tf('s');
P_sb = b0 / (s^3 + a2*s^2 + a1*s + a0); 
%% VERIFICACION SERVO-BARRA
salida_simulada = lsim(P_sb, senialPwm, tiempo);

subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'b-')
plot(tiempo, control, 'r-')
xlabel('tiempo[s]')
ylabel('ángulo[°]')
title("Ángulo de barra")
legend("Simulada", "Real")

subplot(2, 1, 2);
hold on
grid on
plot(tiempo, senialPwm, 'm-')
xlabel('tiempo[s]')
ylabel('ancho[us]')
title("Ancho del PWM")

%% BARRA-CARRITO ORDEN 2 FORZANDO POLO EN EL ORIGEN
orden = 2;
desplazar = @(lista, k) desplazar_general(lista, k, orden, largo);

y = desplazar(posicion, 0) - desplazar(posicion, 1);
X = zeros(largo - orden, 2);
X(:, 1) = desplazar(posicion, 1) - desplazar(posicion, 2);
X(:, 2) = desplazar(control, 0);

D = regresion_lineal(X, y);

alfa0 = - 1 / D(1);
alfa1 = 1 - alfa0;
alfa2 = - 1;
beta0 = alfa0 * D(2);

z = tf('z', dt);
P_bc_z = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2));

b0 = - 1/(dt)^2 * beta0;
a1 = (1 / dt) * (alfa1 + 2 * alfa0);

s = tf('s');
P_bc =  b0 / (s * (s - a1));
%%
% 200 -1.7

b0_m = - 2.8 * 436.6;
a1_m = 300;
beta0_m = b0_m * (dt)^2;
alfa0_m = 1 + dt * a1_m;
alfa1_m = - 2 - dt * a1_m;
alfa2_m = 1;
Pbc_zm = beta0_m / (alfa0_m + alfa1_m * z^(-1) + alfa2_m * z^(-2));
P_bc_m = b0_m / (s * (s + a1_m));

% Pbc = 1.5 * 436.6 / (s * (s + 100));
%% VERIFICACION BARRA-CARRITO
% salida_simulada = lsim(Pbc_zm, control, tiempo);
delay = 0.28;
Pe_delay = P_bc_m;
Pe_delay.InputDelay = delay;
Pdelay = P_bc_m * ((1 - s * (delay / 2)) / (1 + s * (delay / 2)));
salida_simulada = lsim(Pdelay, control, tiempo);
%salida_simulada = lsim(P_bc_m, control, tiempo);

subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'b-')
plot(tiempo, posicion, 'r-')
xlabel('tiempo[s]')
ylabel('posicion[cm]')

title("Posición del carrito")
legend("Simulada", "Real")

subplot(2, 1, 2);
hold on
grid on
plot(tiempo, control, 'm-')
xlabel('tiempo[s]')
ylabel('ángulo[°]')
title("Ángulo de la barra")
%% VERIFICACION COMPLETA
salida_simulada = lsim(P_bc_z*P_sb_z, -senialPwm, tiempo);

subplot(3, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'b-')
plot(tiempo, posicion, 'r-')
title("Posición del carrito")
legend("Simulada", "Real")

subplot(3, 1, 2);
hold on
grid on
plot(tiempo, control, 'm-')
subplot(3, 1, 3);
hold on
grid on
plot(tiempo, -senialPwm, 'b-')

%% VERIFICACION DE LA PLANTA POR REPRESENTACION EN ESPACIO DE ESTADOS
A = [   0    1    0        0    0    0     ;
        0   -1    1        0    0    0     ;
-3.798e04 -3082.4 -202.6   0    0    0     ;
        0    0    0        0    1    0     ;
     1222    0    0        0  -300  -2444  ;
     7.14    0    0        0    0   -7.14  ] ;
 
 B = [0; 0; 1214; 0; 0; 0];
 C = [0 0 0 1 0 0];
 D = 0;

 
 P = ss(A, B, C, D);
 
 posicion_sim = lsim(P, senialPwm, tiempo);
 
 figure;
 hold on
 grid on
 plot(tiempo, posicion_sim, 'm-');
 plot(tiempo, posicion, 'b-');
 
  A = [   0    1    0        0    0    0     ;
        0   -1    1        0    0    0     ;
-3.798e04 -3082.4 -202.6   0    0    0     ;
        0    0    0        -307.14    1    0     ;
     1222.14    0    0        -2142.86  0  1  ;
     -8728.6    1    0        0    0   0 ] ;
  A = [0 1 0; 0 -1 1; -3.798e04 -3082.4 -202.6];
 B = [0; 0; 1214];
 C = [1 0 0];
 D = 0;
  A = [0 1 0; 0 -300 -2444; 0 0 -7.14];
 B = [0; 1222; 7.14];
 C = [1 0 0];
 D = 0;

%% Funciones

function [D] = regresion_lineal(X, y)
    D = (X' * X) \ (X' * y);
end

function [ lista_k_menos ] = desplazar_general(lista, k, orden, largo)
    lista_k_menos = lista(orden-k+1:largo-k);
end

