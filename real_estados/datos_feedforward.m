datos = table();
datos.Tiempo = out.tout;
datos.ControlPWM = out.control;

datos.ReferenciaPosicion = out.ref_posicion;
datos.ReferenciaTheta = out.ref_theta;

datos.ThetaMedido = out.theta_med;
datos.ThetaObservado = out.theta_obs;

datos.OmegaMedido = out.omega_med;
datos.OmegaObservado = out.omega_obs;

datos.PosicionMedido = out.posicion_med;
datos.PosicionObservado = out.posicion_obs;

datos.VelocidadObservado = out.velocidad_obs;

datos.TiempoTranscurrido = out.tiempo;

for intento = 1:100 
    path = sprintf("mediciones/realimentacion_feedforward_%d.csv", intento);
    
    if isfile(path)
        continue;
    end

    writetable(datos, path);
    break
end
