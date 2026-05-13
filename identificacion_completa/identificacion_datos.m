%% Lectura de datos por simulink
%  Primero correr el simulink para tener los valores de out
kp = 25;
ki = 0.02;

datos = table();
datos.Tiempo = out.tout;
datos.ControlPWM = out.control;
datos.Plataforma = out.plataforma;
datos.Posicion = out.posicion;
datos.Error = out.error;

for intento = 1:100 
    archivo = sprintf("identificacion_%d_kp_%.2f_ki_%.2f", intento, kp, ki);
    path = sprintf("mediciones/%s.csv", replace(archivo, ".", "_"));

    if isfile(path)
        continue;
    end

    writetable(datos, path);
    break;
end

if intento == 100
    disp("No se pudo cargar los datos")
end