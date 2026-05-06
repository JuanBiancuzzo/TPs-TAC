%% Modelo discretizado
poli = [1 31.13 271];
polos = roots(poli);
k = 0.0083317;
T = 0.02;
A_d = [ 1 T ; -polos(1)*polos(2)*T  1-(polos(1)+polos(2))*T ];
B_d = [ 0; k*T ];
C_d = [ 1 0 ];
D_d = 0;

%% Diseño de la matrix L de Luenberger

% A_d' + C_d' L'
L = acker(A_d', C_d', [ -0.005 -0.002 ])';

%% Cargamos los datos de mediciones

datos = readmatrix("solo_barra/mediciones_escalera_0_30_paso_10.csv");
tiempo = datos(:, 1);
u_k = datos(:, 2);
y_k = datos(:, 3);

%% Simulacion con valores guardados

largo = size(u_k);
largo = largo(1);

x_hat = zeros(2, largo);
y_hat = zeros(largo);

for i = 1:largo - 1
    y_hat(i) = C_d * x_hat(:, i);
    x_hat(:, i + 1) = A_d * x_hat(:, i) + B_d * u_k(i) + L * ( y_k(i + 1) - y_hat(i) ); 
end
y_hat(largo) = C_d * x_hat(:, largo);


%% Visualizacion
figure
hold on 
grid on

plot(tiempo, x_hat(1, :), 'r-')
plot(tiempo, y_k, 'y-')

legend("Simulado", "Real")

%% Variable de estado
subplot(3,1,1)
plot(tiempo, y_k, 'y-')
title('angulo real')

subplot(3,1,2)
plot(tiempo, x_hat(1, :), 'r-')
title('Theta')

subplot(3,1,3)
plot(tiempo, x_hat(2, :), 'y-')
title('Velocidad angular')


