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

%% Identificacion con foward difference
% yn = b0 * u_{n-5} - sum_{i = 1}^{5} alfa_i * y_{n-i}

orden = 5;
variables = 6;

X = zeros(largo - orden, variables);
X(:, 1) = control(orden:largo);
for i = 1:orden
    X(:, i + 1) = -posicion(i:largo-orden-i+1);  
end
y = theta(3:largo);

D = (X' * X) \ (X' * y);

b0 = D(1);
alfa1 = D(2);
alfa2 = D(3);
alfa3 = D(4);
alfa4 = D(5);
alfa5 = D(6);

z = tf('z', dt);
Pd = b0 / (z^5 + alfa1 * z^4 + alfa2 * z^3 + alfa3 * z^2 + alfa4 * z + alfa5);

a4 = (alfa1 + 5) / dt;
a3 = (alfa2 + 4 * a4 * dt - 10) / (dt^2);
pa3 = (alfa3 - 6 * a4 * dt + 10) / (-3 * dt^2);
dist(sprint("a3 = %.4f, y debería ser igual que %.4f", a3, pa3))
a1 = (alfa4 + 4 * a4 * dt - 3 * a3 * dt^2 - 5) / (dt^4); 
a0 = (alfa5 + 1 - a4 * dt + a3 * dt^2 + a1 * dt^4) / (dt^5);

s = tf('s');
P = b0 / (s^5 + a4 * s4 + a3 * s3 + a1 * s + a0);

%% Identificacion con backward difference


%% Identificacion con trapezoidal 


%% Validacion