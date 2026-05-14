%% Lectura de datos
kp = 25;
ki = 0;
intento = 3;

archivo = sprintf("identificacion_%d_kp_%.2f_ki_%.2f", intento, kp, ki);
path = sprintf("mediciones/%s.csv", replace(archivo, ".", "_"));
datos = readtable(path);

accionEquilibrio = 1472;
dt = 0.02;
recorteInicio = 1 + ceil(0 / dt);
recorteFinal = ceil(25 / dt);

tiempo = datos.Tiempo(recorteInicio:recorteFinal);
control = (datos.ControlPWM - accionEquilibrio);
control = control(recorteInicio:recorteFinal);
plataforma = datos.Plataforma(recorteInicio:recorteFinal);
posicion = datos.Posicion(recorteInicio:recorteFinal);
error = datos.Error(recorteInicio:recorteFinal);

largo = size(control);
largo = largo(1);

orden = 5;
variables = 6;

function [D] = regresion_lineal(X, y)
    D = (X' * X) \ (X' * y);
end

function [ lista_k_menos ] = desplazar_general(lista, k, orden, largo)
    lista_k_menos = lista(orden-k+1:largo-k);
end

desplazar = @(lista, k) desplazar_general(lista, k, orden, largo);

%% Verificando de contenido de control
% Calculamos el fft para ver la frecuencia de la acción de control
Cfft = fft(control);
frecuencias = (1/dt) * (0:(largo/2)) / largo;

% Descarto la mitad de 
Cff = abs(Cfft / largo);
Cff = Cff(1:largo/2+1);
Cff(2:end-1) = 2 * Cff(2:end-1); % es lo mismo que dividir f(0) por 2

figure
grid on

plot(frecuencias, Cff)
title("FFT de la señal de control, debería tener valores en frecuencia tipo 20Hz")
%% Identificacion con foward difference
% yn = beta5 * u_{n-5} - sum_{i = 1}^{5} alfa_i * y_{n-i}

X = zeros(largo - orden, variables);
X(:, 1) = desplazar(control, 0);
for i = 1:orden
    X(:, i + 1) = -desplazar(posicion, i);  
end
y = desplazar(posicion, 0);

D = regresion_lineal(X, y);

beta5 = D(1);
alfa1 = D(2);
alfa2 = D(3);
alfa3 = D(4);
alfa4 = D(5);
alfa5 = D(6);

z = tf('z', dt);
Pd = beta5 / (z^5 + alfa1 * z^4 + alfa2 * z^3 + alfa3 * z^2 + alfa4 * z + alfa5);

% b0 = beta5 / (dt^5);
% a4 = (alfa1 + 5) / dt;
% a3 = (alfa2 + 4 * a4 * dt - 10) / (dt^2);
% pa3 = (alfa3 - 6 * a4 * dt + 10) / (-3 * dt^2);
% dist(sprint("a3 = %.4f, y debería ser igual que %.4f", a3, pa3))
% a1 = (alfa4 + 4 * a4 * dt - 3 * a3 * dt^2 - 5) / (dt^4); 
% a0 = (alfa5 + 1 - a4 * dt + a3 * dt^2 + a1 * dt^4) / (dt^5);

% s = tf('s');
% P = b0 / (s^5 + a4 * s4 + a3 * s3 + a1 * s + a0);

%% Identificacion con backward difference
%

y = desplazar(posicion, 0);
X = zeros(largo - orden, variables);
X(:, 1) = desplazar(posicion, 5);
X(:, 2) = desplazar(posicion, 1);
X(:, 3) = desplazar(posicion, 2);
X(:, 4) = desplazar(posicion, 3);
X(:, 5) = desplazar(posicion, 4);
X(:, 6) = desplazar(control, 0);

D = regresion_lineal(X, y);

alfa0 = 1 / D(1);
alfa1 = -D(2) * alfa0;
alfa2 = -D(3) * alfa0;
alfa3 = -D(4) * alfa0;
alfa4 = -D(5) * alfa0;
alfa5 = -1;
beta  = D(6) * alfa0;

z = tf('z', dt);
Pd = beta / (alfa0 + alfa1 * z^(-1) + alfa2 * z^(-2) + alfa3 * z^(-3) + alfa4 * z^(-4) + alfa5 * z^(-5));

trans_d2c = [
    1/dt^(5)  1/dt^(5)  1/dt^(5)  1/dt^(5)  1/dt^(5)   1/dt^(5)       0 ;
          0  -1/dt^(4) -2/dt^(4) -3/dt^(4) -4/dt^(4)  -5/dt^(4)       0 ;
          0         0   1/dt^(3)  3/dt^(3)  6/dt^(3)  10/dt^(3)       0 ;
          0         0         0  -1/dt^(2) -4/dt^(2) -10/dt^(2)       0 ;
          0         0         0         0   1/dt^(1)   5/dt^(1)       0 ;
          0         0         0         0         0   -1/dt^(0)       0 ;
          0         0         0         0         0          0  1/dt^(5)
];

A = trans_d2c * [alfa0 alfa1 alfa2 alfa3 alfa4 alfa5 beta]';

a0 = A(1);
a1 = A(2);
a2 = A(3);
a3 = A(4);
a4 = A(5);
a5 = A(6);
b0 = A(7);

s = tf('s');
P = b0 / (a0 + a1 * s + a2 * s^2 + a3 * s^3 + a4 * s^4 + a5 * s^5);


%% Identificacion con trapezoidal - No confiable

y = desplazar(posicion, 0) + desplazar(posicion, 5);
X = zeros(largo - orden, variables);
X(:, 1) = desplazar(posicion, 5);
X(:, 2) = desplazar(posicion, 1) - desplazar(posicion, 5);
X(:, 3) = desplazar(posicion, 2) + desplazar(posicion, 5);
X(:, 4) = desplazar(posicion, 3) - desplazar(posicion, 5);
X(:, 5) = desplazar(posicion, 4) + desplazar(posicion, 5);

escalares = [1 5 10 10 5 1];
for i = 0:orden
    agregado = escalares(i + 1) * desplazar(control, i);
    X(:, 6) = X(:, 6) + agregado;
end

D = regresion_lineal(X, y);

alfa0 = -1024 / D(1);
alfa1 = D(2) * alfa0;
alfa2 = D(3) * alfa0;
alfa3 = D(4) * alfa0;
alfa4 = D(5) * alfa0;
alfa5 = alfa0 - alfa1 + alfa2 - alfa3 + alfa4 - 1024;
Alfas = [ alfa0 alfa1 alfa2 alfa3 alfa4 alfa5 ];
b0 = D(6) * alfa0 / (dt^5);

z = tf('z', dt);
Z = [1 z^(-1) z^(-2) z^(-3) z^(-4) z^(-5)];
Pd = (dt^5 * b0 * escalares * Z') / (Alfas * Z'); 

trans_d2c = zeros(orden, 6);
trans_d2c(1, :) = [1 1 1 1 1 1] * (1 / (dt^5 * 32));
trans_d2c(2, :) = [5 3 1 -1 -3 -5] * (1 / (dt^4 * 64));
trans_d2c(3, :) = [5 1 -1 -1 1 5] * (1 / (dt^3 * 64));
trans_d2c(4, :) = [5 -1 -1 1 1 -5] * (1 / (dt^2 * 128));
trans_d2c(5, :) = [5 -3 1 1 -3 5] * (1 / (dt * 512));

As = trans_d2c * Alfas';

s = tf('s');
S = [1 s s^2 s^3 s^4 s^5];
% P = b0 / (S * As');


%% Verificacion

salida_simulada = lsim(Pd, control, tiempo);

figure
hold on 
grid on

plot(tiempo, salida_simulada, 'y-')
plot(tiempo, posicion, 'r-')
plot(tiempo, control, 'g-')

legend("Simulacion", "Real", "Control")

%% Prueba con pulso

nuevo_control = zeros(largo, 1);
nuevo_control(10:40) = 10 * ones(40 - 10 + 1, 1);
salida_simulada = lsim(Pd, nuevo_control, tiempo);

figure
hold on
grid on

plot(tiempo, nuevo_control, 'r-')
plot(tiempo, salida_simulada, 'y-')

legend("Control", "Salida")

%% Bode

optionss=bodeoptions;
optionss.MagVisible='off';
optionss.PhaseMatching='on';
optionss.PhaseMatchingValue=-180;
optionss.PhaseMatchingFreq=1;
optionss.Grid='on';
optionss.MagVisible='on';

figure("Name", "Sistema continuo");
bode(P, optionss);