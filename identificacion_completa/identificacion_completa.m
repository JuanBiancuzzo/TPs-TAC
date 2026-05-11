%% Lecuta de datos
kp = 0.1;
ki = 0;
intento = 1;

archivo = sprintf("identificacion_%d_kp_%.2f_ki_%.2f", intento, kp, ki);
path = sprintf("mediciones/%s.csv", replace(archivo, ".", "_"));
datos = readtable(path);

tiempo = datos.Tiempo;
control = datos.ControlPWM;
plataforma = datos.Plataforma;
posicion = datos.Posicion;
error = datos.Error;

dt = 0.2;

largo = size(angulo);
largo = largo(1);

orden = 5;
variables = 6;

function [D] = regresion_lineal(X, y)
    D = (X' * X) \ (X' * y);
end

function plot_verificacion(planta_d, tiempo, control, salida_real)
    salida_simulada = lsim(planta_d, control, tiempo);

    figure
    hold on 
    grid on
    
    plot(tiempo, salida_simulada, 'y-')
    plot(tiempo, salida_real, 'r-')
end

%% Identificacion con foward difference
% yn = beta5 * u_{n-5} - sum_{i = 1}^{5} alfa_i * y_{n-i}

X = zeros(largo - orden, variables);
X(:, 1) = control(orden:largo);
for i = 1:orden
    X(:, i + 1) = -posicion(i:largo-orden-i+1);  
end
y = theta(orden:largo);

D = regresion_lineal(X, y);

beta5 = D(1);
alfa1 = D(2);
alfa2 = D(3);
alfa3 = D(4);
alfa4 = D(5);
alfa5 = D(6);

z = tf('z', dt);
Pd = beta5 / (z^5 + alfa1 * z^4 + alfa2 * z^3 + alfa3 * z^2 + alfa4 * z + alfa5);

b0 = beta5 / (dt^5);
a4 = (alfa1 + 5) / dt;
a3 = (alfa2 + 4 * a4 * dt - 10) / (dt^2);
pa3 = (alfa3 - 6 * a4 * dt + 10) / (-3 * dt^2);
dist(sprint("a3 = %.4f, y debería ser igual que %.4f", a3, pa3))
a1 = (alfa4 + 4 * a4 * dt - 3 * a3 * dt^2 - 5) / (dt^4); 
a0 = (alfa5 + 1 - a4 * dt + a3 * dt^2 + a1 * dt^4) / (dt^5);

s = tf('s');
P = b0 / (s^5 + a4 * s4 + a3 * s3 + a1 * s + a0);

plot_verificacion(Pd, tiempo, control, salida)

%% Identificacion con backward difference
% yn = beta_0/alfa_0 * u_{n-5} + 1/alfa_0 * y_{n-5} - sum_{i = 1}^{4} alfa_i/alfa_0 * y_{n-i}

X = zeros(largo - orden, variables);
X(:, 1) = control(orden:largo);
X(:, orden + 1) = posicion(orden:largo-orden-orden+1);
for i = 1:orden-1
    X(:, i + 1) = -posicion(i:largo-orden-i+1);  
end
y = theta(orden:largo);

D = regresion_lineal(X, y);

alfa0 = 1 / D(6);
beta0 = alfa0 * D(1);
alfa1 = alfa0 * D(2);
alfa2 = alfa0 * D(3);
alfa3 = alfa0 * D(4);
alfa4 = alfa0 * D(5);

z = tf('z', dt);
Pd = beta5 / (z^5 + alfa1 * z^4 + alfa2 * z^3 + alfa3 * z^2 + alfa4 * z + alfa5);

b0 = beta0 / (dt^5);

plot_verificacion(Pd, tiempo, control, salida)

%% Identificacion con trapezoidal 

function [ lista_k_menos ] = desplazar(lista, k)
    lista_k_menos = lista(orden+k-1:largo-k);
end

y = desplazar(posicion, 0) + desplazar(posicion, 5);
X = zeros(largo - orden, variables);
X(1, :) = desplazar(posicion, 5);
X(2, :) = desplazar(posicion, 1) - desplazar(posicion, 5);
X(3, :) = desplazar(posicion, 2) + desplazar(posicion, 5);
X(4, :) = desplazar(posicion, 3) - desplazar(posicion, 5);
X(5, :) = desplazar(posicion, 4) + desplazar(posicion, 5);

escala = [1 5 10 10 5 1];
for posicion = range(0, orden)
    agregado = escala(posicion + 1) * desplazar(control, posicion);
    X(6, :) = X(6, :) + agregado;
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
Pd = (dt^5 * b0 * Z') / (Alfas * Z'); 

trans_d2c = zeros(orden, 6);
trans_d2c(1, :) = [1 1 1 1 1 1] * (1 / (dt^5 * 32));
trans_d2c(2, :) = [5 3 1 -1 -3 -5] * (1 / (dt^4 * 64));
trans_d2c(3, :) = [5 1 -1 -1 1 5] * (1 / (dt^3 * 64));
trans_d2c(4, :) = [5 -1 -1 1 1 -5] * (1 / (dt^2 * 128));
trans_d2c(5, :) = [5 -3 1 1 -3 5] * (1 / (dt * 512));

As = trans_d2c * Alfas;

s = tf('s');
S = [1 s s^2 s^3 s^4 s^5];
P = b0 / (S * A');

plot_verificacion(Pd, tiempo, control, salida)
