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
B_d = sys_d.B;
C_d = sys_d.C;

%% Luenberger 
% No puede tener más de 2 (rank(C_d)) polos repetidos
polos = [ -40 -40 -20 -25 -30 ];
L = place(A_d', C_d', exp(polos * dt))';
mostrar_matriz(L', "L_T");

%% Matriz de realimentacion K
polos = [ -40 -35 -20 -25 -30 ];
K = place(A_d, B_d, exp(polos * dt));
mostrar_matriz(K, "K");

%% Matriz de feedforward F
% pinv -> es la pseudo inversa
F = pinv(C_d*inv(eye(5)-(A_d+B_d*K))*B_d);
mostrar_matriz(F, "F");


%% Mostrar matriz
function [] = mostrar_matriz(matriz, nombre)
    tamanio = size(matriz);
    fprintf("%s = {\n", nombre);
    for i = 1:tamanio(1)
        fprintf("  { ");
        for j = 1:tamanio(2)
            if j == tamanio(2), delimitador = ""; else, delimitador = ", "; end
    
            fprintf("%+.4e%s", matriz(i, j), delimitador);
        end
        fprintf(" }, \n");
    end
    fprintf("}\n");
end

