%% Modelo con sobrepico - Intento 1
poli = [1 8.029 105.9];
k = 3.206;

%% Modelo discretizado
polos = roots(poli);
T = 0.02;
A_d = [ 1 T 0 ; -polos(1)*polos(2)*T  1-(polos(1)+polos(2))*T 0; 0 0 1];
B_d = [ 0; k*T; 0 ];
C_d = [ 1 0 0; 0 1 1];
D_d = [0; 0];

%% Obs

O = [ C_d; C_d * A_d; C_d * A_d^2 ];
rank(O) % -> 3 es observable

%% Diseño de la matrix L de Luenberger

% A_d' + C_d' L'
polos_d = exp([ -100 -100 -0.1 ] * T);
L = place(A_d', C_d', polos_d)';

%% Cargamos los datos de mediciones

intento = 1;
datos = readtable(sprintf("mediciones/estimaciones_%d.csv", intento));

tiempo = datos.Tiempo;
u_k = datos.ControlPWM;
y_k = [ datos.ThetaMedido datos.VelocidadMedida ]';

%% Simulacion con valores guardados

largo = size(u_k);
largo = largo(1);

x_hat = zeros(3, largo);
y_hat = zeros(2, largo);

for i = 1:largo - 1
    y_hat(:, i) = C_d * x_hat(:, i);
    x_hat(:, i + 1) = A_d * x_hat(:, i) + B_d * u_k(i) + L * ( y_k(:, i) - y_hat(:, i) ); 
end
y_hat(:, largo) = C_d * x_hat(:, largo);


%% Variable de estado
subplot(2,1,1)
hold on
grid on

plot(tiempo, y_k(1, :), 'b-')
plot(tiempo, x_hat(1, :), 'r-')
legend("Angulo real", "Angulo estimado")

subplot(2,1,2)
hold on
grid on

plot(tiempo, y_k(2, :), 'b-')
plot(tiempo, x_hat(2, :), 'r-')
title('Velocidad angular estimado')


