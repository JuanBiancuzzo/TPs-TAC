%% Validación de la planta con la entrada medida
figure
subplot(3, 1, 1);
hold on
grid on
plot(tiempo, angulo)
plot(tiempo, lsim(Psb, pwm, tiempo))
title("Ángulo real y simulado")

subplot(3, 1, 2);
hold on
grid on
plot(tiempo, posicion)
plot(tiempo, lsim(Pbc, angulo, tiempo))
title("Posición del carrito real y simulado")

subplot(3, 1, 3);
hold on
grid on
plot(tiempo, posicion)
plot(tiempo, lsim(Psb*Pbc, pwm, tiempo))
title("Posición del carrito con planta completa real y simulado")
% plot(tiempo, y + y_inicio)

%% Posición del carrito ante escalón real + simulado
figure;
hold on
grid on
plot(tiempo, posicion)
plot(tiempo, y)
plot(tiempo, u)
title("Posición del carrito y respuesta a referencia escalón de la planta")
%% Representación en espacio de estados
A = [     0       1      0      0      0 ;  
          0      -1      1      0      0 ; 
     -3.789e04 -3082.4 -202.6   0      0 ;
          0       0      0      0      1 ;
  -1.2225e+03     0      0      0    -300  ];
B = [0; 0; 1214; 0; 0];
C = [0 0 0 1 0];
D = 0;

Pe = ss(A, B, C, D);
%x0 = [-0.335, 0, 0, -5.92, 0];
x0 = [-0.335, 0, 0, 0, 0];

delay = 0.28;
%Pe_delay = Pe;
%Pe_delay.InputDelay = delay;
%Pe_delay = Pe * ((1 - s * (delay / 2)) / (1 + s * (delay / 2)));
Pe_delay = P * ((1 - s * (delay / 2)) / (1 + s * (delay / 2)));

figure;
hold on
grid on
plot(tiempo, lsim(Pe_delay, pwm, tiempo))
plot(tiempo, posicion)

Le_delay = Pe_delay * Cc;
Tdelay = feedback(Le_delay, 1);
y_sim = lsim(Tdelay, u, tiempo);
figure;
hold on
grid on
plot(tiempo, y_sim)
plot(tiempo, posicion)
plot(tiempo, u)