clear
clc

%% Modelo general del sistema propuesto
% x = [ theta; omega; posicion; velocidad; x_3 ]
% y = [ posicion; theta ]
orden = 5;
dt = 0.02;

A = [
     0.00    1.00  0     0       0.00; 
  -188.80  -17.98  0     0       0.00; 
     0.00    0.00  0     1       0.00; 
  1222.00    0.00  0  -300   -2444.00; 
     7.14    0.00  0     0      -7.14
];
B = [0; 6.15; 0; 0; 0];

C = [ 
    0  0  1  0  0;
    1  0  0  0  0
];

A_d = eye(orden) + dt * A
B_d = dt * B
C_d = C
D_d = 0

%% Pruebas

% Probamos que pasa si solo se mide la posicion
Cposicion = [ 0 0 1 0 0 ];
rango = rank(obsv(A, Cposicion)); 
if rango == orden
    disp("Es observable cuando solo se mide la posicion");
else 
    disp("No es observable cuando solo se mide la posicion");
end

% Probamos que pasa si solo se mide el angulo theta
Cangulo = [ 1 0 0 0 0 ];
rango = rank(obsv(A, Cangulo));
if rango == orden
    disp("Es observable cuando solo se mide el angulo");
else 
    disp("No es observable cuando solo se mide el angulo");
end

% Demostramos que es observable al medir ambas 
rango = rank(obsv(A, C));
if rango == orden
    disp("Es observable el sistema");
else 
    disp("No es observable el sistema");
end

%% Luenberger 
% No puede tener más de 2 (rank(C_d)) polos repetidos
polos = [ -25 -20 -50 -30 -30 ];
L = place(A_d', C_d', exp(polos * dt))'

