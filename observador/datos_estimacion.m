datos = table();
datos.Tiempo = out.tout;
datos.ControlPWM = out.control;
datos.ThetaMedido = out.thetaMedida;
datos.ThetaEstimado = out.thetaEstimado;
datos.VelocidadMedida = out.velocidadMedida;
datos.VelocidadEstimada = out.velocidadEstimada;

for intento = 1:100 
    path = sprintf("mediciones/estimaciones_%d.csv", intento);
    
    if isfile(path)
        continue;
    end

    writetable(datos, path);
    break
end