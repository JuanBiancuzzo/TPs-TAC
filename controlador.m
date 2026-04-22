%% Opciones del bode
optionss=bodeoptions;
optionss.MagVisible='off';
optionss.PhaseMatching='on';
optionss.PhaseMatchingValue=0;
optionss.PhaseMatchingFreq=0.001;
optionss.Grid='on';
optionss.MagVisible='on';

%% Planta
dt = 0.02;
z = tf('z', dt);

C0 = 0.0249;
C1 = -0.7269 + 0.0800i;
C2 = -0.7269 - 0.0800i;

P = zpk(d2c( C0 / ((z + C1) * (z + C2)), "tustin"));

figure("Name", "Sistema continuo");
bode(P/s, optionss, {.01, 100});

%% Controlador teniendo en cuenta el efecto del delay de Ts
% La planta ya tiene incluido el delay de Ts

k = db2mag(18); % 16.5
C = k / ( s );

L = P * C;

figure("Name", "Planta Controlador");
bode(L, optionss, {.01, 100});
margin(L, optionss, {.01, 100});

%% Agregando delay por la discretizacion del controlador
Pd = (1 - dt / 4) / (1 + dt / 4);
Ld = P * Pd * C;
figure("Name", "Planta Controlador");
bode(Ld, optionss, {.01, 100});
margin(Ld, optionss, {.01, 100});

%% Discretizando el controlador
Cc = zpk(c2d(C, dt, 'tustin'));

%% Step response
S = 1 / (1 + L);
T = 1 - S;

figure("Name", "Planta Controlador");
step(T, 10);