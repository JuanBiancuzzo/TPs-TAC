import matplotlib.pyplot as plt
import csv

# import re
# Tiene el formato "Tiempo (%f)"
# print(header.split(",")[0])
# matcheo = re.search("\\((.*?)\\)", header.split(",")[0])
# datos["dt"] = float(matcheo.group()) 

CARPETA = "barra_carrito_extremo_igual"
NOMBRE_ARCHIVO = f"{CARPETA}/mediciones_escalera_0_30_paso_10.csv"
RADIANES_2_GRADOS = 57.2958

AMPLITUD_PLATAFORMA = 0.3347
OFFSET_PLATAFORMA = 1.0843

OFFSET_GIROSCOPIO = -0.07175

def leer_archivo(nombre_archivo):
    datos = {}
    with open(nombre_archivo, "r") as archivo:
        lineas = archivo.readlines()

        tiempos, angulos, thetas = [], [], []
        for linea in lineas:
            tiempo, angulo, theta = tuple(map(lambda valor: float(valor), linea.split(",")))

            angulos.append(angulo)
            thetas.append(theta)
            tiempos.append(tiempo)

        datos["tiempo"] = tiempos 
        datos["angulo"] = angulos
        datos["theta"] = thetas

    return datos

def graficar_resultado(datos):
    tiempos = datos["tiempo"]
    angulos = datos["angulo"]
    thetas = datos["theta"]

    fig, (ax1, ax2) = plt.subplots(2, 1, sharex = True)
    fig.suptitle("Mediciones")

    ax1.plot(tiempos, angulos)
    ax1.grid()

    ax2.plot(tiempos, thetas)
    ax2.grid()

    plt.show()

def main():
    datos = {}
    try:
        datos = leer_archivo(NOMBRE_ARCHIVO)

    except Exception as error:
        print("Hubo un error al leer el archivo, con error: ", error)
        return 

    graficar_resultado(datos)

if __name__ == "__main__":
    main()