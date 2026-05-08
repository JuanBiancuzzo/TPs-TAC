%% Lectura de datos por simulink
%  Primero correr el simulink para tener los valores de out
control = out.control;
plataforma = out.plataforma;
tiempo = out.tout;

carperta = "mediciones";
posicion_carro = "centro";

datos = table();
datos.Tiempo = tiempo;
datos.Control = control;
datos.Plataforma = plataforma;

writetable(datos, sprintf("%s/carro_en_%s.csv", carperta, posicion_carro));