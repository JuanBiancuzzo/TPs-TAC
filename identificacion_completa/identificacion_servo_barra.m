%% Datos de lectura - Intento 1 - Sin carrito
intento = 1;
accionEquilibrio = 1472;
corrimiento = 0.51;

%% Datos de lectura - Intento 3 - Con carrito
intento = 3;
accionEquilibrio = 0;
corrimiento = 0.91;

%% Lectura de datos
path = sprintf("mediciones/identificacion_plataforma_%d.csv", intento);
datos = readtable(path);

tiempoInicio = 1.2;
tiempoFinal = 1000;

dt = 0.02;
inv_dt = 50;
largo = size(datos.Tiempo);
largo = largo(1);

recorteInicio = max(1, ceil(tiempoInicio / dt));
recorteFinal = min(largo, ceil(tiempoFinal / dt));

tiempo = (datos.Tiempo - datos.Tiempo(recorteInicio));
tiempo = tiempo(recorteInicio:recorteFinal);
control = (datos.ControlPWM - accionEquilibrio);
control = control(recorteInicio:recorteFinal);
plataforma = (datos.Plataforma + corrimiento);
plataforma = plataforma(recorteInicio:recorteFinal);

largo = size(tiempo);
largo = largo(1);

%% Identificacion con backward difference
% Orden 3

orden = 3;
desplazar = @(lista, k) desplazar_general(lista, k, orden, largo);

y = desplazar(plataforma, 0);
X = zeros(largo - orden, 4);
X(:, 1) = desplazar(plataforma, 3);
X(:, 2) = desplazar(plataforma, 1);
X(:, 3) = desplazar(plataforma, 2);
X(:, 4) = desplazar(control, 0);

D = regresion_lineal(X, y);

alfa0 = 1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = -D(3) * alfa0;
alfa3 = -1;
beta0 = D(4) * alfa0;

z = tf('z', dt);
Pd3 = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2) + alfa3 * z^(-3));

%% Identificacion con backward difference
% Orden 2

orden = 2;
desplazar = @(lista, k) desplazar_general(lista, k, orden, largo);

y = desplazar(plataforma, 0);
X = zeros(largo - orden, 3);
X(:, 1) = desplazar(plataforma, 2);
X(:, 2) = desplazar(plataforma, 1);
X(:, 3) = desplazar(control, 0);

D = regresion_lineal(X, y);

alfa0 = -1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = 1;
beta0 = D(3) * alfa0;

z = tf('z', dt);
Pd2 = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2));

a1 = -inv_dt * (2 * alfa2 + alfa1);
a0 = inv_dt^2 * (alfa2 + alfa1 + alfa0);
b0 = inv_dt^2 * beta0;

s = tf('s');
P2 = b0 / (s^2 + a1 * s + a0);

%% Verificacion

salida_simulada = lsim(Pd2, control, tiempo);

subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'g-')
plot(tiempo, plataforma, 'r-')
legend("Simulada", "Real")

subplot(2, 1, 2);
hold on
grid on
plot(tiempo, control, 'b-')

%% Comparacion

salida_simulada_2 = lsim(Pd2, control, tiempo);
salida_simulada_3 = lsim(Pd3, control, tiempo);

figure
hold on 
grid on

plot(tiempo, salida_simulada_2, 'b-')
plot(tiempo, salida_simulada_3, 'g-')
plot(tiempo, plataforma, 'r-')
legend("Orden 2", "Orden 3", "Real")


%% Tocar los valores
p1 = -8+6i;
p2 = -8-6i;
b0_nuevo = b0 / 4;

P2_nuevo = zpk([], [p1 p2], b0_nuevo);
a2_nuevo = 1;
a1_nuevo = (p1 + p2) / 2;
a0_nuevo = (p1 * p2);

alfa2_nuevo = 1;
alfa1_nuevo = -2 - dt * a1_nuevo;
alfa0_nuevo = 1 + dt * a1_nuevo + dt^2 * a0_nuevo;
beta0_nuevo = dt^2 * b0_nuevo;

z = tf('z', dt);
Pd2_nuevo = beta0_nuevo / (alfa0_nuevo + alfa1_nuevo * z^(-1) + alfa2_nuevo * z^(-2));
Pd2_nuevo = zpk(c2d(P2_nuevo, dt, "tustin"));
% Pd2_nuevo = 4 * 0.000274 / (z^2 - 1.692 * z + 0.7265);

%% Validación 2 - Mejorada ultra

salida_simulada = lsim(Pd2, control, tiempo);
salida_simulada_nuevo = lsim(Pd2_nuevo, control, tiempo);

subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'g-')
plot(tiempo, salida_simulada_nuevo, 'b-')
plot(tiempo, plataforma, 'r-')
legend("Simulada Reg linea", "Simulada tuneada", "Real")

subplot(2, 1, 2);
hold on
grid on
plot(tiempo, control, 'b-')

%% Bode

optionss=bodeoptions;
optionss.MagVisible='off';
optionss.PhaseMatching='on';
optionss.PhaseMatchingValue=-180;
optionss.PhaseMatchingFreq=1;
optionss.Grid='on';
optionss.MagVisible='on';

figure("Name", "Sistema continuo");
hold on

bode(P2, optionss, {0.1, 200}, "r-")
bode(P2_nuevo, optionss, {0.1, 200}, "b-")

legend("Original", "Nuevo")

%% Funciones

function [D] = regresion_lineal(X, y)
    D = (X' * X) \ (X' * y);
end

function [ lista_k_menos ] = desplazar_general(lista, k, orden, largo)
    lista_k_menos = lista(orden-k+1:largo-k);
end