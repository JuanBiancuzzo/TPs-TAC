clear
clc

%% Modelo general del sistema propuesto
% x = [ theta; omega; posicion; velocidad; x_3 ]
% y = [ posicion; theta ]
orden = 5;
dt = 0.02;

b0 = 6.142;
a0 = 201.9;
a1 = 13.75;
A_sb = [
      0   1;
    -a0 -a1
];
B_sb = [ 0; b0 ];
C_sb = [ 1 0 ];

a = 300; 
b = -1222; 
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

sys = ss(A, B, C, 0);
sys_d = c2d(sys, dt, 'zoh');

A_d = sys_d.A;
mostrar_matriz(A_d, "A_d");
B_d = sys_d.B;
mostrar_matriz(B_d', "B_d");
C_d = sys_d.C;
mostrar_matriz(C_d, "C_d");

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
polos = [ -40 -40 -20 -25 -30 ];
L = place(A_d', C_d', exp(polos * dt))';
mostrar_matriz(L, "L");

%% Mostrar matriz
function [] = mostrar_matriz(matriz, nombre)
    tamanio = size(matriz);
    fprintf("%s = {\n", nombre);
    for i = 1:tamanio(1)
        fprintf("  { ");
        for j = 1:tamanio(2)
            if j == tamanio, delimitador = ""; else, delimitador = ", "; end
            
            fprintf("%+.4e%s", matriz(i, j), delimitador);
        end
        fprintf("}, \n");
    end
    fprintf("}\n");
end

