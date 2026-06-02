%% Valores controlador
kp = 28;
ki = 0;
kd = 0;
%% Planta, controlador, y sensibilidad

s = tf('s');
dt = 0.02;
delay = 0.28;
Psb = 1214 / (s^3 + 203.6 * s^2 + 3285 * s + 3.798e04);
Pbc = - 2.8 * 436.6 / (s * (s + 300)) * ((1 - s * (delay / 2)) / (1 + s * (delay / 2)));
P = Psb * Pbc;
Cc = - (kp + ki / s + kd * s);
Cd = c2d(Cc, dt ,'tustin');

L = P * Cc;
S = 1 / (1 + L);

%% Escalón - Intento 1 -Control PD
tipo = "escalon";
intento = 1;
tiempoInicio = 0.4; 
tiempoFinal = 6.9;
corrimiento = 4.437;

%% Impulso - Intento 1 - Control PD
tipo = "pulso";
intento = 1;
tiempoInicio = 6.76; 
tiempoFinal = 11.2;
corrimiento = 1.468;
%% Extracción de datos
archivo = sprintf("control_%s_%d_kp_%.2f_ki_%.2f_kd_%.4f", tipo, intento, kp, ki, kd);

datos = readtable(sprintf("mediciones/%s.csv", replace(archivo, ".", "_")));

largo = size(datos.Tiempo);
largo = largo(1);

recorteInicio = max(1, ceil(tiempoInicio / dt));
recorteFinal = min(largo(1), ceil(tiempoFinal / dt));
tiempo = datos.Tiempo(recorteInicio:recorteFinal) - datos.Tiempo(recorteInicio);
pwm = -datos.ControlPWM(recorteInicio:recorteFinal);
posicion = datos.Posicion(recorteInicio:recorteFinal) + corrimiento;
angulo = datos.Plataforma(recorteInicio:recorteFinal);

%% Simulación escalón
% Defino T a lazo cerrado, donde T = posicion / referencia y la referencia 
% es tipo escalón, se quiere ver al salida ante esta referencia.

T = 1 - S;

A = 10;      % amplitud final
tr = 0.4;    % rise time

escalon = zeros(size(tiempo));
% Rampa desde 0 hasta A durante tr segundos
idx_rampa = tiempo <= tr;
escalon(idx_rampa) = A/tr * tiempo(idx_rampa);
% Después queda constante en A
escalon(tiempo > tr) = A;

p_escalon = lsim(T, escalon, tiempo);

figure;
hold on
grid on
plot(tiempo, p_escalon, '-b')
plot(tiempo, posicion, '-r')
xlabel('tiempo[s]')
ylabel('posicion[cm]')
legend("Simulada", "Real")
%plot(tiempo, escalon)


%% Simulación impulso
% Para analizar la salida ante un impulso de perturbación de entrada puedo
% analizar PS = Y(s)/V(s). 

PS = P * S;

% Genero el impulso
A = -77;        % area del impulso
t0 = 0.21;     % instante del golpe

impulso = zeros(size(tiempo));

[~,k0] = min(abs(tiempo - t0));

impulso(k0) = A/dt;

p_impulso = lsim(PS, impulso, tiempo);

figure;
hold on
grid on
plot(tiempo, p_impulso, '-b')
plot(tiempo, posicion, '-r')
xlabel('tiempo[s]')
ylabel('posicion[cm]');
legend("Simulada", "Real")
%title("Posición del carrito ante una perturbación impulsiva de entrada")
%plot(tiempo, impulso)

%% Escalón - Intento 2 -Control P
tipo = "escalon";
intento = 2;
tiempoInicio = 1.96; 
tiempoFinal = 1000;
corrimiento = 5.92;

%% Impulso - Intento 1 - Control P
tipo = "pulso";
intento = 1;
tiempoInicio = 6.5; 
tiempoFinal = 11.4;
corrimiento = 1.738;

%% Escalón - Intento 1 - Control PI
tipo = "escalon";
intento = 1;
tiempoInicio = 0.34; 
tiempoFinal = 12.2;
corrimiento = 5.989;

%% Impulso - Intento 1 - Control PI
tipo = "pulso";
intento = 1;
tiempoInicio = 2.14; 
tiempoFinal = 8.18;
corrimiento = 0.456;

%% Escalón - Intento 3 -Control PI
tipo = "escalon";
intento = 3;
tiempoInicio = 0.44; 
tiempoFinal = 12.68;
corrimiento = 5.044;
