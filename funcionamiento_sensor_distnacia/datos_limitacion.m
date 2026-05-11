%% Lectura de datos por simulink
datos = table();
datos.Tiempo = out.tout;
datos.TiempoMedido = out.tiempo;
datos.DistanciaMedida = out.distancia;

writetable(datos, "limitaciones_rangos_medio_15_71_cm.csv");
