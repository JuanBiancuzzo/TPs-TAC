%% Lectura de datos por simulink
%  Primero correr el simulink para tener los valores de out
angulo = out.angulo;
theta = out.theta;
tiempo = out.tout;
dt = 0.02;

% nombres = [ sprintf("Tiempo (%f)", dt), "Señal de control", "Theta" ];
% datosMatriz = [tiempo angulo theta];
% datos = array2table(datosMatriz, 'VariableNames', nombres);

datosMatriz = [tiempo angulo theta];
carperta = "barra_carrito_extremo_igual";
archivo = "mediciones_escalera_-30_30_paso_10.csv";
% archivo = "mediciones_escalera_0_30_paso_5.csv";
% archivo = "mediciones_escalera_-20_20_paso_10.csv";
% archivo = "mediciones_escalera_0_30_paso_10.csv";
% archivo = "mediciones_0_10.csv";
% archivo = "mediciones_0_20.csv";
% archivo = "mediciones_0_30.csv";

writematrix(datosMatriz, sprintf("%s/%s", carperta, archivo));

%% Lectura de datos por archivo
% Vamos a leer un archivo en el mismo formato de como se guarda
% en la seccion anterior

datos = readmatrix("mediciones_alfas_0.12.csv");
dt = 0.02; % Hacer una forma que lo lea del archivo
tiempo = datos(:, 1);
angulo = datos(:, 2);
theta = datos(:, 3);

%% Identificacion

% Definimos el modelo de la planta
% theta/u = k / ( (s + p1) * (s + p2) )
% theta/u|discreto = c0 / ( (z + c1) * (z + c2) )
% yn = D0 * u_{n-2} + D1 * y_{n-1} + D2 * y_{n-2}
% y => theta
% u => angulo

largo = size(angulo);
largo = largo(1);

X = [ angulo(3:largo) theta(2:largo-1) theta(1:largo-2) ];
y = theta(3:largo);

D = inv(X' * X) * X' * y;

% Calculamos valores de transferencia a partir los D
%C0 = -D(1) / D(3);
%C2 = (-D(2) + sqrt(D(2) * D(2) + 4 * D(3))) / (-2 * D(3));
%C1 = ( D(2) / D(3) ) - C2;
C0 = D(1);
C2 = -( D(2) + sqrt( D(2) * D(2) + 4 * D(3) ) ) / 2;
C1 = (-D(2)) - C2;
C = [ C0 C1 C2 ];
%% Validacion
% Simulamos curva con nuestros parametros
z = tf('z', dt);

T = C0 / ((z + C1) * (z + C2));
y_simulada = lsim(T, angulo, tiempo);

figure
hold on 
grid on

plot(tiempo, y_simulado, 'y-')
plot(tiempo, theta, 'r-')

%% Simulacion en el tiempo con ecuacion en diferencias

y_simulado = zeros(largo, 1);
y_simulado(1) = theta(1);
y_simulado(2) = theta(2);

for i = 3:largo
    y_simulado(i) = D(1) * angulo(i) + D(2) * y_simulado(i - 1) + D(3) * y_simulado(i - 2);
end 

figure
hold on 
grid on

plot(tiempo, y_simulado, 'y-')
plot(tiempo, theta, 'r-')

legend("Simulado", "Real")

%% Tiempo continuo

Tc = zpk(d2c(T));

optionss=bodeoptions;
optionss.MagVisible='off';
optionss.PhaseMatching='on';
optionss.PhaseMatchingValue=-180;
optionss.PhaseMatchingFreq=1;
optionss.Grid='on';
optionss.MagVisible='on';

figure("Name", "Sistema continuo");
bode(Tc, optionss, {.1, 100});
