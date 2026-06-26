%% Archivo mediciones
intento = 2;
%archivo = sprintf("realimentacion_sin_referencia_%d", intento);
archivo = sprintf("realimentacion_feedforward_%d", intento);

path = sprintf("mediciones/%s.csv", archivo);
datos = readtable(path);

%% Guardado de datos simulacion
path = sprintf("simulacion/%s.csv", archivo);
datosSim = table();
datosSim.Tiempo = out.tout;
datosSim.ControlPWM = out.control;

datosSim.ThetaSimulado = out.theta;
datosSim.ThetaObservado = out.theta_obs;

datosSim.OmegaSimulado = out.omega;
datosSim.OmegaObservado = out.omega_obs;

datosSim.PosicionSimulado = out.posicion;
datosSim.PosicionObservado = out.posicion_obs;

datosSim.VelocidadSimulado = out.velocidad;
datosSim.VelocidadObservado = out.velocidad_obs;
writetable(datosSim, path);
datosSim = readtable(path);

%% Extraccion de datos de mediciones
% Posicion impulso positiva (sin ref), intento 1
%tiempoInicio = 10;
%tiempoFinal = 18.96;

% Posicion impulso negativa (sin ref), intento 2
%tiempoInicio = 172.8;
%tiempoFinal = 177.8;

% Con feedforward: intento 2
%tiempoInicio = 2.70662
%tiempoFinal = 48.6

tiempoInicio = 2.70662;
tiempoFinal = 48.6;
dt = 0.02;
inv_dt = 50;

largo = size(datos.Tiempo);
recorteInicio = max(1, ceil(tiempoInicio / dt));
recorteFinal = min(largo(1), ceil(tiempoFinal / dt));

% Tiempo
tiempo = (datos.Tiempo - datos.Tiempo(recorteInicio));
tiempo = tiempo(recorteInicio:recorteFinal);
% PWM
pwm = (datos.ControlPWM);
pwm = pwm(recorteInicio:recorteFinal);
% Referencia
ref = (datos.ReferenciaPosicion);
ref = ref(recorteInicio:recorteFinal);
% Theta medido
angulo_medido = (datos.ThetaMedido);
angulo_medido = angulo_medido(recorteInicio:recorteFinal);
% Theta observado
angulo_observado = (datos.ThetaObservado);
angulo_observado = angulo_observado(recorteInicio:recorteFinal);
% Omega medido
omega_medido = (datos.OmegaMedido);
omega_medido = omega_medido(recorteInicio:recorteFinal);
% Omega observado
omega_observado = (datos.OmegaObservado);
omega_observado = omega_observado(recorteInicio:recorteFinal);
% Posicion medido
posicion_medido = (datos.PosicionMedido);
posicion_medido = posicion_medido(recorteInicio:recorteFinal);
% Posicion observado
posicion_observado = (datos.PosicionObservado);
posicion_observado = posicion_observado(recorteInicio:recorteFinal);
% Velocidad medido
velocidad_medido = gradient(datos.PosicionMedido, dt);
velocidad_medido = velocidad_medido(recorteInicio:recorteFinal);
% Velocidad observado
velocidad_observado = (datos.VelocidadObservado);
velocidad_observado = velocidad_observado(recorteInicio:recorteFinal);

largo = size(tiempo);
largo = largo(1);

%% Extraccion de datos de simulacion
% Posicion impulso positiva (sin ref): tiempo de simulacion 9s, ganancia impulso -960, step
% time del primer escalon en 1.3281s y del segundo escalon en 2.2s
% En la simulacion tomo referencia nula.
% Posicion impulso negativa (sin ref): tiempo de simulacion 5s, ganancia impulso 1010, step
% time del primer escalon en 0s y del segundo escalon en 0.85s

tiempoInicioSim = 0;
tiempoFinalSim = 10000;

largoSim = size(datosSim.Tiempo);
recorteInicioSim = max(1, ceil(tiempoInicioSim / dt));
recorteFinalSim = min(largoSim(1), ceil(tiempoFinalSim / dt));

% Tiempo
tiempoSim = (datosSim.Tiempo - datosSim.Tiempo(recorteInicioSim));
tiempoSim = tiempoSim(recorteInicioSim:recorteFinalSim);
% PWM
pwmSim = (datosSim.ControlPWM);
pwmSim = pwmSim(recorteInicioSim:recorteFinalSim);
% Theta medido
angulo_medidoSim = (datosSim.ThetaSimulado);
angulo_medidoSim = angulo_medidoSim(recorteInicioSim:recorteFinalSim);
% Theta observado
angulo_observadoSim = (datosSim.ThetaObservado);
angulo_observadoSim = angulo_observadoSim(recorteInicioSim:recorteFinalSim);
% Omega medido
omega_medidoSim = (datosSim.OmegaSimulado);
omega_medidoSim = omega_medidoSim(recorteInicioSim:recorteFinalSim);
% Omega observado
omega_observadoSim = (datosSim.OmegaObservado);
omega_observadoSim = omega_observadoSim(recorteInicioSim:recorteFinalSim);
% Posicion medido
posicion_medidoSim = (datosSim.PosicionSimulado);
posicion_medidoSim = posicion_medidoSim(recorteInicioSim:recorteFinalSim);
% Posicion observado
posicion_observadoSim = (datosSim.PosicionObservado);
posicion_observadoSim = posicion_observadoSim(recorteInicioSim:recorteFinalSim);
% Velocidad medido
velocidad_medidoSim = (datosSim.VelocidadSimulado);
velocidad_medidoSim = velocidad_medidoSim(recorteInicioSim:recorteFinalSim);
% Velocidad observado
velocidad_observadoSim = (datosSim.VelocidadObservado);
velocidad_observadoSim = velocidad_observadoSim(recorteInicioSim:recorteFinalSim);

largo = size(tiempo);
largo = largo(1);

%%
%{
PWM
figure;
hold on
grid on
plot(tiempo, pwm)
plot(tiempoSim, pwmSim)
title('PWM')
%}


% Theta
subplot(2, 2, 1)
hold on
grid on
plot(tiempo, angulo_medido, 'm-', 'LineWidth', 0.8)
%plot(tiempo, angulo_observado)
plot(tiempoSim, angulo_medidoSim, 'b-', 'LineWidth', 0.8)
%plot(tiempoSim, angulo_observadoSim)
xlabel('Tiempo[s]')
ylabel('Ángulo[º]')
legend('Curva medida', 'Curva simulada')
title('Ángulo de la barra')

% Omega
subplot(2, 2, 2)
hold on
grid on
plot(tiempo, omega_medido, 'm-', 'LineWidth', 0.8)
%plot(tiempo, omega_observado)
plot(tiempoSim, omega_medidoSim, 'b-', 'LineWidth', 0.8)
%plot(tiempoSim, omega_observadoSim)
xlabel('Tiempo[s]')
ylabel('Velocidad angular[º/s]')
legend('Curva medida', 'Curva simulada')
title('Velocidad angular')

% Posicion
subplot(2, 2, 3)
hold on
grid on
%plot(tiempo, ref, 'r-', 'LineWidth', 0.8)
plot(tiempo, posicion_medido, 'm-', 'LineWidth', 0.8)
%plot(tiempo, posicion_observado)
plot(tiempoSim, posicion_medidoSim, 'b-', 'LineWidth', 0.8)
%plot(tiempoSim, posicion_observadoSim)
xlabel('Tiempo[s]')
ylabel('Posición[cm]')
legend('Curva medida', 'Curva simulada')
%legend('Curva de referencia', 'Curva medida', 'Curva simulada')
title('Posicion del carrito')

% Velocidad
subplot(2, 2, 4)
hold on
grid on
plot(tiempo, velocidad_medido, 'm-', 'LineWidth', 0.8)
%plot(tiempo, velocidad_observado)
plot(tiempoSim, velocidad_medidoSim, 'b-', 'LineWidth', 0.8)
%plot(tiempoSim, velocidad_observadoSim)
xlabel('Tiempo[s]')
ylabel('Velocidad[cm/s]')
legend('Curva medida', 'Curva simulada')
title('Velocidad del carrito')

%%
fig1 = openfig("variables_feedforward.fig");
ax = findobj(fig1, "Type", "Axes");
ax = flipud(ax);
hold(ax(1), "on")

plot(ax(1), tiempo, angulo_observado, "LineWidth", 0.8, "DisplayName", "Observador medido")

hold(ax(2), "on")
plot(ax(2), tiempo, omega_observado, "LineWidth", 0.8, "DisplayName", "Observador medido")
hold(ax(3), "on")
plot(ax(3), tiempo, posicion_observado, "LineWidth", 0.8, "DisplayName", "Observador medido")
hold(ax(4), "on")
plot(ax(4), tiempo, velocidad_observado, "LineWidth", 0.8, "DisplayName", "Observador medido")


