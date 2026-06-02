%% Planta linealizada

h0 = 0.45;

s = tf('s');
P = -0.004233 / (s + 0.002397);

%% Bode
Ts = 1;

Cmp = - db2mag(11.2) * (s + 0.002397) / (s * (s + 0.5));
Cap = (1 - Ts / 2 * s) / (1 + Ts / 2 * s);
C = Cmp * Cap;
L = C * P;

Cd = backwards(Cmp, Ts);


%% 
optionss=bodeoptions;
optionss.MagVisible='off';
optionss.PhaseMatching='on';
optionss.PhaseMatchingValue=-180;
optionss.PhaseMatchingFreq=1;
optionss.Grid='on';
optionss.MagVisible='on';

figure("Name", "Sistema continuo");
bode(L, optionss, {0.00001, 1});


%% Diseño del control

u = zeros(100, 1);
u(10:100) = 1;
tiempo = linspace(0, 99, 100)';

figure;
hold on
grid on
plot(tiempo, lsim(Cmp, u, tiempo), 'g-');
plot(tiempo, lsim(Cd, u, tiempo), 'r-');
