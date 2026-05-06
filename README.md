# TPs-TAC - Práctica 7
En esta práctica se busca crear un observador, e implementarlo para el micro de arduino 

Primer paso es obtener el modelo continuo de la planta, la cual es la barra con el servo, dado por la identificación del sistema $$ P = \frac{0.0083317}{s^2 + 31.13s + 271} = \frac{0.0083317}{(s - 15.565 + j ~ 5.360) (s - 15.565 - j ~ 5.360)} $$

Después lo pasamos a variables de estado donde estas representan el ángulo y la derivada del ángulo de la barra $$ \begin{align*}
    A &= \begin{bmatrix}  
        0 & 1 \\
        -p_1 p_2 & -(p_1 + p_2) \\
    \end{bmatrix} &&= \begin{bmatrix}  
        0 & 1 \\
        -271.00 & 31.13 \\
    \end{bmatrix} \\
    B &= \begin{bmatrix} 0 \\ k  \end{bmatrix} &&= \begin{bmatrix} 0 \\ 0.0083317 \end{bmatrix} \\
    C &= \begin{bmatrix} 1 & 0 \end{bmatrix} \\
    D &= 0 \\
\end{align*} $$ con $$ x = \begin{bmatrix} \theta \\ \dot{\theta} \end{bmatrix} $$

Pasamos este modelo continuo, de variables de estado, a discreto. Tomando como periodo $T = 20 ~ \text{ms}$, se obtiene $$ \begin{align*}
    A_d &= (\mathbb{I} + A ~ T) = \begin{bmatrix}  
        1 & T \\
        -p_1 p_2 ~ T & 1 - (p_1 + p_2) ~ T \\
    \end{bmatrix} &&= \begin{bmatrix}  
        1 & 0.02 \\
        -5.42 & 0.38 \\
    \end{bmatrix} \\
    B_d &= B ~ T = \begin{bmatrix} 0 \\ k ~ T  \end{bmatrix} &&= \begin{bmatrix} 0 \\ 166.64 \times 10^{-6} \end{bmatrix} \\
    C_d &= C = \begin{bmatrix} 1 & 0 \end{bmatrix} \\
    D_d &= D = 0 \\
\end{align*} $$ 

Obteniendo este modelo, planteamos la idea del observador $L$ 

Lo simulamos en arduino guardandonos los datos obtenidos