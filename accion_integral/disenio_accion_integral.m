clear
clc

%% Modelo general del sistema propuesto
% x = [ theta; omega; posicion; velocidad; x_3 ]
% y = [ posicion ]
orden = 5;
dt = 0.02;

b0 = 6.15;
a0 = 188.8;
a1 = 17.98;
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
C = [ 0  0  1  0  0 ];

sys = ss(A, B, C, 0);
sys_d = c2d(sys, dt, 'zoh');

A_d = sys_d.A;
B_d = sys_d.B;
C_d = sys_d.C;

%% Luenberger 
% No puede tener más de 1 (rank(C_d)) polos repetidos
polos = [ -40 -30 -20 -25 -30 ];
L = place(A_d', C_d', exp(polos * dt))';
mostrar_matriz(L', "L_T");

%% Matriz de realimentacion K y H
ext = 1;
polos = [ -40 -35 -20 -25 -30 -45 ];
A_e = [
        A_d zeros(orden, ext);
    -C_d*dt     eye(ext, ext)
];
B_e = [ B_d; zeros(ext) ]; 
KH = place(A_e, B_e, exp(polos * dt));

K = KH(1:orden);
mostrar_matriz(K, "K");

H = KH(orden+1:end);
mostrar_matriz(H, "H");

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

