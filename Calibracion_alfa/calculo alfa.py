import matplotlib.pyplot as plt
import csv

# import re
# Tiene el formato "Tiempo (%f)"
# print(header.split(",")[0])
# matcheo = re.search("\\((.*?)\\)", header.split(",")[0])
# datos["dt"] = float(matcheo.group()) 

NOMBRE_ARCHIVO = "datos.csv"
RADIANES_2_GRADOS = 57.2958

AMPLITUD_PLATAFORMA = 0.3347
OFFSET_PLATAFORMA = 1.0843

OFFSET_GIROSCOPIO = -0.07175

def leer_archivo(nombre_archivo):
    datos = {}
    with open(nombre_archivo, "r") as archivo:
        lineas = archivo.readlines()
        _, lineas = lineas[0], lineas[1:]

        tiempos, angulos, thetas_acc, giros_x = [], [], [], []
        tiempo_acumulado = 0
        dt = 0
        for linea in lineas:
            theta_acc, giro_x, dt_actual, angulo = tuple(map(lambda valor: float(valor), linea.split(",")))

            dt = dt_actual 
            angulos.append(angulo)
            thetas_acc.append(theta_acc)
            giros_x.append(giro_x)
            tiempos.append(tiempo_acumulado)

            tiempo_acumulado += dt_actual

        datos["tiempo"] = tiempos 
        datos["angulo"] = angulos
        datos["theta_acc"] = thetas_acc
        datos["giro_x"] = giros_x
        datos["dt"] = dt 

    return datos

def obtener_angulo_giroscopio(angulo_anterior, giro_x, dt):
    return angulo_anterior + RADIANES_2_GRADOS * giro_x * dt

def angulos_complementarios(thetas_acc, giros_x, dt, alfa):
    thetas = []
    theta_complementario = 0
    for theta_acc, giro_x in zip(thetas_acc, giros_x):
        theta_giro = obtener_angulo_giroscopio(theta_complementario, giro_x, dt) 
        theta_complementario = alfa * theta_acc + (1 - alfa) * theta_giro

        thetas.append(theta_complementario)

    return thetas

def graficar_alfa(alfa, datos):
    dt = datos["dt"]
    tiempos = datos["tiempo"]
    angulos = datos["angulo"]

    fig, (ax1, ax2) = plt.subplots(2, 1, sharex = True)
    fig.suptitle("Variación de alfa")

    angulos_escalados = list(map(lambda angulo: AMPLITUD_PLATAFORMA * (angulo - OFFSET_PLATAFORMA), angulos))
    ax1.plot(tiempos, angulos_escalados)
    ax1.grid()

    theta_giro = 0
    thetas_giro = []
    for giro_x in datos["giro_x"]:
        theta_giro = obtener_angulo_giroscopio(theta_giro, giro_x - OFFSET_GIROSCOPIO, dt)
        thetas_giro.append(theta_giro)

    ax2.plot(tiempos, datos["theta_acc"], label = f"Theta acelerometro")
    ax2.plot(tiempos, thetas_giro, label = f"Theta giroscopio")

    thetas = angulos_complementarios(datos["theta_acc"], datos["giro_x"], dt, alfa)
    ax2.plot(tiempos, thetas, label = f"Alfa = {alfa}")

    ax2.grid()
    ax2.legend()

    plt.show()

def graficar_alfas(alfas, datos):
    dt = datos["dt"]
    tiempos = datos["tiempo"]
    angulos = datos["angulo"]

    fig, (ax1, ax2) = plt.subplots(2, 1, sharex = True)
    fig.suptitle("Variación de alfa")

    ax1.plot(tiempos, angulos)
    ax1.grid()

    angulos_escalados = list(map(lambda angulo: AMPLITUD_PLATAFORMA * (angulo - OFFSET_PLATAFORMA), angulos))
    ax2.plot(tiempos, angulos_escalados, label = "Ideal")

    for alfa in alfas:
        thetas = angulos_complementarios(datos["theta_acc"], datos["giro_x"], dt, alfa)
        ax2.plot(tiempos, thetas, label = f"Alfa = {alfa}")

    ax2.grid()
    ax2.legend()

    plt.show()

def guardar_datos_alfa(alfa, datos):
    dt = datos["dt"]
    tiempos = datos["tiempo"]
    angulos = datos["angulo"]
    thetas = angulos_complementarios(datos["theta_acc"], datos["giro_x"], dt, alfa)

    with open(f"mediciones_alfas_{alfa}.csv", "w", newline="", encoding="utf-8") as archivo:
        writer = csv.writer(archivo)
        writer.writerow([ f"Tiempo ({dt})", "Señal de control", "Theta" ])
        writer.writerows(zip( tiempos, angulos, thetas ))

def main():
    datos = {}
    try:
        datos = leer_archivo(NOMBRE_ARCHIVO)

    except Exception as error:
        print("Hubo un error al leer el archivo, con error: ", error)
        return 

    # graficar_alfas([0.06, 0.08, 0.1, 0.12], datos)
    guardar_datos_alfa(0.12, datos)
    # graficar_alfa(0.12, datos)



if __name__ == "__main__":
    main()