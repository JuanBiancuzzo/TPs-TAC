%% Datos de lectura - Intento 7
intento = 7;
kp = 45;
ki = 0;
plataformaEquilibrio = -2;
corrimiento = 0;

%% Datos de lectura - Intento 8
intento = 8;
kp = 45;
ki = 0;
plataformaEquilibrio = -2;
corrimiento = 0;

%% Lectura de datos

archivo = sprintf("identificacion_%d_kp_%.2f_ki_%.2f", intento, kp, ki);
path = sprintf("mediciones/%s.csv", replace(archivo, ".", "_"));
datos = readtable(path);

tiempoInicio = 3;
tiempoFinal = 6;

dt = 0.02;
inv_dt = 50;

largo = size(datos.Tiempo);
recorteInicio = max(1, ceil(tiempoInicio / dt));
recorteFinal = min(largo(1), ceil(tiempoFinal / dt));

tiempo = (datos.Tiempo - datos.Tiempo(recorteInicio));
tiempo = tiempo(recorteInicio:recorteFinal);
control = (datos.Plataforma - plataformaEquilibrio);
control = control(recorteInicio:recorteFinal);
posicion = (datos.Posicion + corrimiento);
posicion = posicion(recorteInicio:recorteFinal);

senial_servo = datos.ControlPWM(recorteInicio:recorteFinal);
senial_error = datos.Error(recorteInicio:recorteFinal);

largo = size(tiempo);
largo = largo(1);

%% Identificacion con backward difference
% Orden 2

orden = 2;
desplazar = @(lista, k) desplazar_general(lista, k, orden, largo);

y = desplazar(posicion, 0);
X = zeros(largo - orden, 3);
X(:, 1) = desplazar(posicion, 2);
X(:, 2) = desplazar(posicion, 1);
X(:, 3) = desplazar(control, 0);

D = regresion_lineal(X, y);

alfa0 = -1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = 1;
beta0 = D(3) * alfa0;

z = tf('z', dt);
Pd = beta0 / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2));

a1 = -inv_dt * (2 * alfa2 + alfa1);
a0 = inv_dt^2 * (alfa2 + alfa1 + alfa0);
b0 = inv_dt^2 * beta0;

s = tf('s');
P = b0 / (s^2 + a1 * s + a0);

%% Verificacion

salida_simulada = lsim(Pd, control, tiempo);

subplot(2, 1, 1);
hold on 
grid on

plot(tiempo, salida_simulada, 'g-')
plot(tiempo, posicion, 'r-')
legend("Simulada", "Real")

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
bode(P, optionss, {0.1, 200});

%% Funciones

function [D] = regresion_lineal(X, y)
    D = (X' * X) \ (X' * y);
end

function [ lista_k_menos ] = desplazar_general(lista, k, orden, largo)
    lista_k_menos = lista(orden-k+1:largo-k);
end