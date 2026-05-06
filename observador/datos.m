angulo_contro = out.control;
theta_real = out.theta_real;
theta_obs = out.theta_obs;
velocidad_real = out.velocidad_real;
velocidad_obs = out.velocidad_obs;
tiempo = out.tout;

% nombres = [ sprintf("Tiempo (%f)", dt), "Señal de control", "Theta" ];
% datosMatriz = [tiempo angulo theta];
% datos = array2table(datosMatriz, 'VariableNames', nombres);

datosMatriz = [tiempo angulo_contro theta_real theta_obs velocidad_real velocidad_obs];
archivo = "polos_-0.005_-0.002_escalon_30_0_otro.csv";

writematrix(datosMatriz, archivo);