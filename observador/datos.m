datos = table();
datos.Tiempo = out.tout;
datos.ControlPWM = out.control;
datos.ThetaMedido = out.theta_med;
datos.ThetaObservado = out.theta_obs;
datos.OmegaMedido = out.omega_med;
datos.OmegaObservado = out.omega_obs;
datos.PosicionMedido = out.posicion_med;
datos.PosicionObservado = out.posicion_obs;
datos.VelocidadObservado = out.velocidad_obs;

for intento = 1:100 
    path = sprintf("mediciones/observador_%d.csv", intento);
    
    if isfile(path)
        continue;
    end

    writetable(datos, path);
    break
end
