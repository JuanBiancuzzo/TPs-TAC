import sys
import numpy as np
import matplotlib.pyplot as plt

PERIODO = 0.02
SEPARADOR = ";"

NOMBRES_DATOS = ["Accion de control", "Angulo plataforma", "Posicion carro", "Error en posicion", "Tiempo transcurrido us"]
CANT_VARIABLES = len(NOMBRES_DATOS) 
POS_ACCION_CONTROL = 0
POS_THETA_PLATAFORMA = 1
POS_POSICION_CARRO = 2
POS_POSICION_ERROR = 3
POS_TIEMPO_TRANSCURRIDO_MICROS = 4

class Grafico:
    def __init__(self):
        self.cantidadPuntos = 1000
        self.datos = np.zeros((CANT_VARIABLES, self.cantidadPuntos)) 
        self.tiempo = PERIODO * np.arange(0, self.cantidadPuntos) 
        self.puntoActual = 0

    def iniciarPlot(self):
        plt.ion() # Hacemos que sea interactivo el plot aka actualizable
        self.figure, (axControl, axTheta, axPosicion, axTiempo) = plt.subplots(4, figsize = (10, 10))

        # Lo inicializamos en ceros, ya que la actualizacion va a agarrar los valores reales
        ceros = np.zeros(self.cantidadPuntos)
        
        # Plot de control
        midMicros = 1472 # 0 grados
        minMicrosRango = 1038 - midMicros # -44 grados
        maxMicrosRango = 2152 - midMicros # 66 grados

        axControl.plot(self.tiempo, minMicrosRango * np.ones(self.cantidadPuntos))
        self.lineaControl, = axControl.plot(self.tiempo, ceros)
        axControl.plot(self.tiempo, maxMicrosRango * np.ones(self.cantidadPuntos))

        axControl.grid(True)
        axControl.set_ylabel("Acción de control")

        # Plot angulo de la plataforma
        self.lineaTheta, = axTheta.plot(self.tiempo, ceros)

        axTheta.grid(True)
        axTheta.set_ylabel("Angulo de la plataforma [deg]")

        # Plot posicion
        self.lineaPosicion, = axPosicion.plot(self.tiempo, ceros, label = "Posicion")
        self.lineaReferencia, = axPosicion.plot(self.tiempo, ceros, label = "referencia")

        axPosicion.grid(True)
        axPosicion.set_ylabel("Posicion del carro [cm]")
        axPosicion.legend()

        # Plot tiempo transcurrido
        self.lineaTiempo, = axTiempo.plot(self.tiempo, ceros)
        axTiempo.plot(self.tiempo, 1000 * PERIODO * np.ones(self.cantidadPuntos))

        axTiempo.grid(True)
        axTiempo.set_ylabel("Tiempo transcurrido [ms]")

        self.figure.suptitle("Identificación", fontsize = 20)

    def agregarDatos(self, nuevosDatos):
        if self.puntoActual >= self.cantidadPuntos - 1:
            self.puntoActual = self.cantidadPuntos - 1
            # Desplazamos un dato (-1) cuando se llenan
            self.datos = np.roll(self.datos, -1, axis = 1)

        else:
            self.puntoActual += 1

        for posicion, nuevoDato in enumerate(nuevosDatos):
            self.datos[posicion, self.puntoActual] = nuevoDato

        self.lineaControl.set_ydata(self.datos[POS_ACCION_CONTROL, :])

        self.lineaTheta.set_ydata(self.datos[POS_THETA_PLATAFORMA, :])

        self.lineaPosicion.set_ydata(self.datos[POS_POSICION_CARRO, :])
        self.lineaReferencia.set_ydata(self.datos[POS_POSICION_CARRO, :] + self.datos[POS_POSICION_ERROR, :])

        self.lineaTiempo.set_ydata(self.datos[POS_TIEMPO_TRANSCURRIDO_MICROS, :] / 1000)

        self.figure.canvas.draw()
        self.figure.canvas.flush_events()

def main():
    grafico = Grafico()
    grafico.iniciarPlot()

    print(SEPARADOR.join(["Tiempo", *NOMBRES_DATOS]))
    tiempo = 0

    for linea in sys.stdin:
        nuevosDatos = linea.split(SEPARADOR)
        grafico.agregarDatos(nuevosDatos)

        print(SEPARADOR.join(map(lambda valor: "{:.4f}".format(valor), [tiempo, *nuevosDatos])))
        tiempo += PERIODO

    print(f"Finalizado, duracion: {tiempo}", file = sys.stderr)

if __name__ == "__main__":
    try: 
        main()

    except KeyboardInterrupt:
        print("Terminando de graficar...", file = sys.stderr)

