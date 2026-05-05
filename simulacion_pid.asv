%% Planta
s = tf('s');
k = -0.099863;
G = (k*s) / ((s+10.14)*(s-10.14));
 
%% 
kp = -160; % -100;
ki = -2028; % -1350;
kd = -0.0;
Ts = 0.01;
Cd = (1 - (Ts/4 * s)) / (1 + (Ts/4 * s));
C = kp + ki/s + kd*s;
Cdig = c2d(C, Ts, 'tustin');


%% 
figure 
hold on
for ki = 20:30
    kp = 1;
    kd = 0;
    C = -60*(kp + ki/s + kd*s);
    L = minreal(P*C);
    S = 1 / (1 + L);    
    T = 1 - S;

    [y, t] = initial(ss(T), [4*pi/180 0], 3.5);
    plot(t, y, 'DisplayName', sprintf("ki = %d", ki))
end
legend('show'); 

%%
A = [0 (10.14)^2 ; 1 0];
B = [1 ; 0];
C = [k 0];
D = 0;
P = ss(A, B, C, D);