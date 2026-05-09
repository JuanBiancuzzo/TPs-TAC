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

trans_c2d = zeros(orden, orden);
trans_c2d(:, 1) = [5 10 10 5 1]' * (dt/2)^5;
trans_c2d(:, 2) = [3 1 -1 -3 -1]' * (dt/2)^4;
trans_c2d(:, 3) = [0 1 -1 -1 1]' * (dt/2)^2;
trans_c2d(:, 4) = [-3 2 -2 -3 1]' * (dt/2);
trans_c2d(:, 5) = [-5 10 -10 5 -1]';

trans_d2c = inv(trans_c2d);
C = zeros(orden);
C(5) = trans_d2c(5, 5);
C(1:4) = trans_d2c(5, 1:4) / C(5);


X = zeros(largo - orden, variables);
for i = 0:orden
    coef = nchoosek(orden, i);
    X(:, 1) = X(:, 1) + coef * control(orden-i:largo-i);
end 

for i = 1:4
    X(:, i + 1) = C(i) * posicion(1:largo-orden) - posicion(orden-i:largo-i);
end

y = posicion(orden:largo) + C(5) * posicion(1:largo-orden);

D = regresion_lineal(X, y);

beta0 = D(1);
Beta = [1 5 10 10 5 1] * beta0;

alfa1 = D(2);
alfa2 = D(3);
alfa3 = D(4);
alfa4 = D(5);
alfa5 = C(5) - C(1:4) * D(2:5)';
Alfa = [1 alfa1 alfa2 alfa3 alfa4 alfa5];

z = tf('z', dt);
Z = [1 z^(-1) z^(-2) z^(-3) z^(-4) z^(-5)];
Pd = (Beta * Z') / (Alfa * Z'); 

A = trans_c2d / Alfa(2:6);
b0 = beta0 * (2 / dt)^5;

s = tf('s');
S = [1 s s^3 s^4 s^5];
P = b0 / (S * A');

plot_verificacion(Pd, tiempo, control, salida)
