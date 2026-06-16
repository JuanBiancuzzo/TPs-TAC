clear
clc

%% Modelo general del sistema propuesto
% x = [ theta; omega; posicion; velocidad; x_3 ]
% y = [ posicion; theta ]
orden = 5;
dt = 0.02;

A_sb = [
       0     1;
    -188.8 -17.98
];
B_sb = [ 0; 6.15 ];
C_sb = [ 1 0 ];

a = 446.4;
b = -1583.6;
c_inv = 2/0.28;
A_bc = [
    0  1    0;
    0 -a  2*b;
    0  0 -c_inv
];
B_bc = [ 0; -b; c_inv ];
C_bc = [ 1 0 0 ];

A = [
   A_sb       zeros(2, 3);
   B_bc*C_sb  A_bc
];
B = [ B_sb; 0; 0; 0 ];

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
clc
polos = [ -50 -50 -350 -350 -400 ];
L = place(A_d', C_d', exp(polos * dt))';

clc
for i = 1:5
    disp(L(i, 1:2))
end

