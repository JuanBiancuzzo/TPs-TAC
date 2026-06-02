%% Modelo general del sistema propuesto
% x = [ theta; omega; corriente; posicion; velocidad; x_3 ]
orden = 6;
A = [   0    1    0       0    0   0   ;
        0   -1    1       0    0   0   ;
-3.798e04 -3082.4 -202.6  0    0   0   ;
        0    0    0       0    1   0   ;
     1222    0    0       0  -300 -2444;
     7.14    0    0       0    0  -7.14];
B = [0; 0; 1214; 0; 0; 0];
C = [ 
    0 0 0 1 0 0;
    1 0 0 0 0 0 
];
D = [ 0; 0 ];

%% Observabilidad
% Probamos que pasa si solo se mide la posicion
Cposicon = [ 0 0 0 1 0 0 ];
rango = rank(obsv(A, Cposicion)) 
if rango == orden
    disp("Es observable cuando solo se mide la posicion");
else 
    disp("No es observable cuando solo se mide la posicion");
end

% Probamos que pasa si solo se mide el angulo theta
Cangulo = [ 1 0 0 0 0 0 ];
rango = rank(obsv(A, Cangulo)) 
if rango == orden
    disp("Es observable cuando solo se mide el angulo");
else 
    disp("No es observable cuando solo se mide el angulo");
end

% Demostramos que es observable al medir ambas 
rango = rank(obsv(A, C)) 
if rango == orden
    disp("Es observable el sistema");
else 
    disp("No es observable el sistema");
end

%% Planta discreta
T = 0.02;
A_d = eye(orden) + T * A
B_d = T * B
C_d = C
D_d = D

%% Luenberger 
polos_d = exp([ 1 1 1 1 1 1] * T);
L = place(A_d', C_d', polos_d)'
