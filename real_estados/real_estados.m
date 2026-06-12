clc

%% Modelo ultima version
poli = [1 17.98 188.8];
k = 6.153;

%% Modelo discretizado
polos = roots(poli);
T = 0.02;
A_d = [ 1 T ; -polos(1)*polos(2)*T  1-(polos(1)+polos(2))*T ];
B_d = [ 0; k*T ];
C_d = [ 1 0 ];
D_d = 0;

%% Diseño de la matrix L de Luenberger
polos_d = exp([ -100 -100 ] * T);
L = acker(A_d', C_d', polos_d)'

%% Diseño de K
polos_r = exp([-50 -15]* T);
K = acker(A_d, B_d, polos_r)

K = [10 0.5];

%% Diseño del feed-forward
F = inv(C_d*(eye(2)-inv(A_d+B_d*K))*B_d)
