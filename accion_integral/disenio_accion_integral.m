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
C = [ 
    0  0  1  0  0 
    1  0  0  0  0 
];

sys = ss(A, B, C, 0);
sys_d = c2d(sys, dt, 'zoh');

A_d = sys_d.A;
B_d = sys_d.B;
C_d = sys_d.C;

%% Luenberger 
% No puede tener más de 1 (rank(C_d)) polos repetidos
polos = [ -40 -62 -22 -300 -14.28 ];
L = place(A_d', C_d', exp(polos * dt))';
mostrar_matriz(L', "L_T");

%% Usando Kalman Filter
% x = [theta omega posicion velocidad x_3]
V_d = diag([ 0.1, 1, 0.2, 4, 0.4 ]); 
% y = [posicion theta]
V_n = diag([ 3, 0.1 ]);
[Lkf_T, S, E] = dlqr(A_d', C_d', V_d, V_n);

mostrar_matriz(Lkf_T, "L_kf_T");
% disp(log(E') / dt);

%% Discretizacion extendida
ext = 1;
C_ai = [ 0 0 1 0 0 ];
A_e = [
        A zeros(orden, ext);
    -C_ai zeros(ext, ext)
];
B_e = [ B; zeros(ext) ]; 
C_e = [ C_ai zeros(ext) ]; 
 
sys_e = ss(A_e, B_e, C_e, 0);
sys_ed = c2d(sys_e, dt, 'zoh');

A_ed = sys_ed.A;
B_ed = sys_ed.B;

%% Matriz de realimentacion K y H
polos = [ -40 -35 -20 -25 -30 -45 ];
KH = place(A_ed, B_ed, exp(polos * dt));

K = KH(1:orden);
mostrar_matriz(K, "K");

H = KH(orden+1:end);
mostrar_matriz(H, "H");

%% Usando LQR
% x = [theta omega posicion velocidad x_3 q_1 ]
Q = diag([ 5000, 200, 22000, 800, 0.001, 150000]); 
% u = [pwm]
R = diag( 0.01 );
[KHlqr, S, E] = dlqr(A_ed, B_ed, Q, R);

mostrar_matriz(KHlqr, "KH_lqr");

Klqr = KHlqr(1:orden);
% mostrar_matriz(Klqr, "K_lqr");

Hlqr = KHlqr(orden+1:end);
% mostrar_matriz(Hlqr, "H_lqr");

mostrar_matriz(log(E') / dt, "KH_lambda");

%% Simulacion continua
polos_obc = [-11.9-5.74i -11.9+5.74i -24.32 -40.5 -300];
Lc = place(A', C', polos_obc)';
Ld = place(A_d', C_d', exp(polos_obc*dt));
mostrar_matriz(Ld, "Ld_T");

%polos_c = [-8-3i -8+3i -15 -7 -5 -300];
% polos_c = [-3.59 -3.591 -7.136 -8.397 -276.93 -308.81];
polos_c = [-3.5680e+00, -3.5681e+00, -7.1370e+00, -8.6085e+00, -2.7385e+02, -3.0831e+02];
Ke_c = place(A_e, B_e, polos_c);
Kc = Ke_c(1:orden);
Hc = Ke_c(orden+1:end);

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
