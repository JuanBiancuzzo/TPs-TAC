%% Modelo sin sobrepico - Intento 3
poli = [1 28.1 353.3];
k = 10.92;

%% Modelo con sobrepico - Intento 1
poli = [1 8.029 105.9];
k = 3.206;

%% Modelo discretizado
polos = roots(poli);
T = 0.02;
A_d = [ 1 T ; -polos(1)*polos(2)*T  1-(polos(1)+polos(2))*T ];
B_d = [ 0; k*T ];
C_d = [ 1 0 ];
D_d = 0;

%% Diseño de la matrix L de Luenberger

% A_d' + C_d' L'
polos_d = exp([ -100 -100 ] * T);
L = acker(A_d', C_d', polos_d)';

%% Cargamos los datos de mediciones

intento = 1;
datos = readtable(sprintf("mediciones/estimaciones_%d.csv", intento));

tiempo = datos.Tiempo;
u_k = datos.ControlPWM;
y_k = datos.ThetaMedido;
% datos.ThetaEstimado;
w_k = datos.VelocidadMedida;
% datos.VelocidadEstimada;

%% Simulacion con valores guardados

largo = size(u_k);
largo = largo(1);

x_hat = zeros(2, largo);
y_hat = zeros(largo, 1);

for i = 1:largo - 1
    y_hat(i) = C_d * x_hat(:, i);
    x_hat(:, i + 1) = A_d * x_hat(:, i) + B_d * u_k(i) + L * ( y_k(i) - y_hat(i) ); 
end
y_hat(largo) = C_d * x_hat(:, largo);


%% Variable de estado
subplot(2,1,1)
hold on
grid on

plot(tiempo, y_k, 'b-')
plot(tiempo, x_hat(1, :), 'r-')
legend("Angulo real", "Angulo estimado")

subplot(2,1,2)
hold on
grid on

plot(tiempo, w_k, 'b-')
plot(tiempo, x_hat(2, :), 'r-')
title('Velocidad angular estimado')


