theta_acc = out.theta_acc;
giro_x = out.giro_x;
dt = out.dt;
angulo = out.angulo;

datos = [theta_acc giro_x dt angulo];
writematrix(datos, "datos.csv");