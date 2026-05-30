"""
Serial communication with Arduino.

Usage:
    comunicacion.py serial -c=<comm> [-b=<boudrate>] [-h=<header>] [-t=<timeout>] -o=<archivo-output> [-T=<periodo>] [-p=<puntos>] [-s=<separador>]
    comunicacion.py archivo -i=<archivo-input> [--separador-input=<separador-input>] [-T=<periodo>] [-p=<puntos>]
    comunicacion.py --help
    comunicacion.py --version

Options:
    --help       Mostrar los argumentos posibles.
    --version    Version.

    -T=<periodo>, --periodo=<periodo>           Periodo en segundos. [default: 0.02]
    -p=<puntos>, --cantidad-puntos=<puntos>     Cantidad de puntos en el diagrama. [default: 1000]

    -c=<comm>, --comm=<comm>                    Puerto de comunicación con el arduino.
    -b=<boudrate>, --boudrate=<boudrate>        Velocidad de la comunicación [default: 115200].
    -h=<header>, --header=<header>              Header de los mensajes. [default: abcd]
    -t=<timeout>, --timeout=<timeout>           Timeout en segundos de la lecutra de serial. [default: 2]

    -o=<archivo-output>, --archivo-output=<archivo-output>  Nombre del archivo output csv. 
    -s=<separador>, --separador=<separador>                 Separador del csv generado. [default: ;]

    -i=<archivo-input>, --archivo-input=<archivo-input>     Nombre del archivo csv de input.
    --separador-input=<separador-input>                     Separador para el archivo input. [default: ;]
"""

import serial # Libreria pyserial
import struct
import threading
import queue

from docopt import docopt # Libreria docopt-ng
import numpy as np
import matplotlib.pyplot as plt
from enum import Enum, IntEnum, auto

# Para correrlo ejemplo:
# python3 comuncacion.py --comm COMM3 -o mediciones/observaciones.csv

class Variable(IntEnum):
    ACCION_DE_CONTROL = 0

    THETA_MEDIDO = 1
    THETA_ESTIMADO = 2

    OMEGA_MEDIDA = 3
    OMEGA_ESTIMADA = 4

    POSICION_MEDIDO = 5
    POSICION_ESTIMADO = 6

    VELOCIDAD_ESTIMADA = 7

    CANTIDAD = auto()

class Grafico:
    def __init__(self, periodo, cantidad_puntos):
        self.cantidad_puntos = cantidad_puntos
        self.periodo = periodo

        self.datos = np.zeros((Variable.CANTIDAD, self.cantidad_puntos)) 
        self.tiempo = self.periodo * np.arange(0, self.cantidad_puntos) 

        self.punto_actual = 0
        self.actualizar = []

    def iniciarPlot(self):
        plt.ion() # Hacemos que sea interactivo el plot aka actualizable
        self.figure = plt.figure(layout = "constrained")
        axis = self.figure.subplot_mosaic([
            ["Theta", "Posicion"],
            ["Omega", "Velocidad"],
            ["Control", "Control"],
        ])
        axControl, axTheta, axOmega, axPosicion, axVelocidad = tuple([ axis[i] for i in ["Control", "Theta", "Omega", "Posicion", "Velocidad"] ])

        # Lo inicializamos en ceros, ya que la actualizacion va a agarrar los valores reales
        ceros = np.zeros(self.cantidad_puntos)

        def actualizar_rangos(ax, lineaMedida, posicionMedida, lineaEstimada, posicionEstimada):
            def actualizar_ejes(datos):
                medida = datos[posicionMedida, :]
                estimada = datos[posicionEstimada, :]

                lineaMedida.set_ydata(medida)
                lineaEstimada.set_ydata(estimada)

                concat = np.concatenate((medida, estimada))
                minimo, maximo = np.min(concat), np.max(concat)
                rango = max(1, maximo - minimo)
                ax.set_ylim(minimo - 0.05 * rango, maximo + 0.05 * rango)
            return actualizar_ejes
        
        # Plot de control
        minMicrosRango = -1038 # -44 grados
        maxMicrosRango = 2152 # 66 grados

        axControl.plot(self.tiempo, minMicrosRango * np.ones(self.cantidad_puntos))
        lineaControl, = axControl.plot(self.tiempo, ceros)
        axControl.plot(self.tiempo, maxMicrosRango * np.ones(self.cantidad_puntos))

        axControl.grid(True)
        axControl.set_ylabel("PWM [us]")
        axControl.set_title("Acción de control")

        def actualizar_control(datos):
            lineaControl.set_ydata(datos[Variable.ACCION_DE_CONTROL, :])
        self.actualizar.append(actualizar_control)

        # Plot estimacion angulo
        lineaThetaMedida, = axTheta.plot(self.tiempo, ceros, label = "Medicion")
        lineaThetaEstimada, = axTheta.plot(self.tiempo, ceros, label = "Estimada")

        axTheta.set_title("Angulo de la plataforma")
        axTheta.set_ylabel("Angulo [deg]")
        axTheta.set_ylim(-15, 22)

        def actualizar_angulo(datos):
            lineaThetaMedida.set_ydata(datos[Variable.THETA_MEDIDO, :])
            lineaThetaEstimada.set_ydata(datos[Variable.THETA_ESTIMADO, :])
        self.actualizar.append(actualizar_angulo)

        # Plot estimacion velocidad angular
        lineaOmegaMedida, = axOmega.plot(self.tiempo, ceros, label = "Medicion")
        lineaOmegaEstimada, = axOmega.plot(self.tiempo, ceros, label = "Estimada")

        axOmega.set_title("Velocidad angular de la plataforma")
        axOmega.set_ylabel("Velocidad angular [deg/s]")

        self.actualizar.append(actualizar_rangos(
            axOmega, lineaOmegaMedida, Variable.OMEGA_MEDIDA, lineaOmegaEstimada, Variable.OMEGA_ESTIMADA,
        ))

        # Plot estimacion posicion
        lineaPosicionMedida, = axPosicion.plot(self.tiempo, ceros, label = "Medicion")
        lineaPosicionEstimada, = axPosicion.plot(self.tiempo, ceros, label = "Estimada")

        axPosicion.set_title("Posicion del carro")
        axPosicion.set_ylabel("Posicion [cm]")
        axPosicion.set_ylim(-20, 20)

        def actualizar_posicion(datos):
            lineaPosicionMedida.set_ydata(datos[Variable.POSICION_MEDIDO, :])
            lineaPosicionEstimada.set_ydata(datos[Variable.POSICION_ESTIMADO, :])
        self.actualizar.append(actualizar_posicion)

        # Plot estimacion velocidad
        lineaVelocidadMedida, = axVelocidad.plot(self.tiempo, ceros, label = "Medida")
        lineaVelocidadEstimada, = axVelocidad.plot(self.tiempo, ceros, label = "Estimada")

        axVelocidad.set_title("Velocidad del carro")
        axVelocidad.set_ylabel("Velocidad [cm/s]")

        def actualizar_velocidad(datos):
            velocidad_estimada = datos[Variable.POSICION_ESTIMADO, :]
            velocidad_medida = (velocidad_estimada - np.roll(velocidad_estimada, -1)) / self.periodo

            # Limpiando la derivada
            velocidad_medida[self.punto_actual] = 0 
            velocidad_medida[-1] = 0

            lineaVelocidadMedida.set_ydata(velocidad_medida)
            lineaVelocidadEstimada.set_ydata(velocidad_estimada)

            concat = np.concatenate((velocidad_medida, velocidad_estimada))
            minimo, maximo = np.min(concat), np.max(concat)
            rango = max(1, maximo - minimo)
            ax.set_ylim(minimo - 0.05 * rango, maximo + 0.05 * rango)
        self.actualizar.append(actualizar_velocidad)

        for ax in [axTheta, axOmega, axPosicion, axVelocidad]:
            ax.grid(True)
            ax.legend(loc = "upper right")

        self.figure.suptitle("Observador", fontsize = 16)

    def agregarDatos(self, nuevosDatos):
        if self.punto_actual >= self.cantidad_puntos - 1:
            self.punto_actual = self.cantidad_puntos - 1
            # Desplazamos un dato (-1) cuando se llenan
            self.datos = np.roll(self.datos, -1, axis = 1)

        else:
            self.punto_actual += 1

        for posicion, nuevoDato in enumerate(nuevosDatos):
            self.datos[posicion, self.punto_actual] = nuevoDato

        for actualizacion in self.actualizar:
            actualizacion(self.datos)

        self.figure.canvas.draw()
        self.figure.canvas.flush_events()

    def finalizarPlot(self):
        plt.ioff()
        plt.show()

class TipoArgumento(Enum):
    SERIAL = "Comunicacion serial"
    ARCHIVO = "Archivo input"

class Argumentos:
    def __init__(self, periodo, puntos):
        self.periodo = periodo
        self.puntos = puntos
    
    def serial(self, comm, baudrate, header, timeout):
        self.tipo = TipoArgumento.SERIAL
        self.comm = comm
        self.baudrate = baudrate
        self.header = header
        self.timeout = timeout

    def archivo_output(self, archivo_output, separador_output):
        self.tipo = TipoArgumento.SERIAL
        self.archivo_output = archivo_output
        self.separador_output = separador_output

    def archivo_input(self, archivo_input, separador_input):
        self.tipo = TipoArgumento.ARCHIVO
        self.archivo_input = archivo_input
        self.separador_input = separador_input

def parse_args(dicc_args):
    args = Argumentos(float(dicc_args["--periodo"]), max(1, int(dicc_args["--cantidad-puntos"])))

    if dicc_args["serial"]:
        args.serial(
            dicc_args["--comm"], 
            int(dicc_args["--boudrate"]),
            dicc_args["--header"],
            float(dicc_args["--timeout"]),
        )

        args.archivo_output(
            dicc_args["--archivo-output"], 
            dicc_args["--separador"],
        )

    elif dicc_args["archivo"]: 
        args.archivo_input(
            dicc_args["--archivo-input"], 
            dicc_args["--separador-input"],
        )

    return args

def lectura_serial(comm, baudrate, timeout, header, output_queue):
    TAM_FLOAT = 4

    largo_header = len(header)
    header_bytes = [ bytes(b, "utf-8") for b in header ]
    def esperarHeader(ser):
        header_receive = 0
        while header_receive < largo_header:
            data_bytes = ser.read(1)
            if data_bytes == header_bytes[header_receive]:
                header_receive += 1
            else:
                header_receive = 0

    with serial.Serial(comm, baudrate, timeout = timeout) as ser:
        print(f"Conectado al puerto: {ser.portstr}")

        while True:
            esperarHeader(ser)

            data_bytes = ser.read(TAM_FLOAT * Variable.CANTIDAD)
            if not data_bytes:
                break

            nuevos_datos = [ 0 ] * Variable.CANTIDAD
            for i in range(Variable.CANTIDAD):
                inicio = i * TAM_FLOAT
                final = (i + 1) * TAM_FLOAT
                nuevos_datos[i] = struct.unpack('<f', data_bytes[inicio:final])[0]

            output_queue.put(nuevos_datos)

    output_queue.shutdown()
    output_queue.join()

def lectura_archivo(nombre_archivo, separador, output_queue):
    import time

    with open(nombre_archivo, "r") as archivo:
        _ = archivo.readline() # Descartamos header
        for linea in archivo:
            nuevos_datos = [ 
                float(valor) 
                for valor in linea.strip().split(separador) 
            ][1:] # Descartamos la primera columna
            output_queue.put(nuevos_datos)

            time.sleep(1)

    output_queue.shutdown()
    output_queue.join()

def escribir_archivo(nombre_archivo, separador, periodo, input_queue):
    with open(nombre_archivo, "w") as archivo:
        nombres = [ 
            Variable(i).name.replace("_", " ").capitalize() 
            for i in range(Variable.CANTIDAD) 
        ]
        archivo.write(f"{separador.join(["Tiempo", *nombres])}\n")

        for i, nuevos_datos in enumerate(input_queue):
            archivo.write(f"{separador.join([ periodo * i, *nuevos_datos])}\n")

def graficar_datos(periodo, cantidad_puntos, input_queue):
    grafico = Grafico(periodo, cantidad_puntos)
    grafico.iniciarPlot()

    for nuevos_datos in input_queue:
        grafico.agregarDatos(nuevos_datos)

    grafico.finalizarPlot()

class IteratableQueue:
    def __init__(self, queue):
        self.queue = queue

    def __iter__(self):
        while True:
            try:
                valor = self.queue.get()
                yield valor
                self.queue.task_done()

            except queue.ShutDown:
                break

class MultipleQueue:
    def __init__(self, *queues):
        self.queues = list(queues)

    def put(self, valor):
        for queue in self.queues:
            queue.put(valor)

    def shutdown(self, immediate = False):
        for queue in self.queues:
            queue.shutdown(immediate)

    def join(self):

        for queue in self.queues:
            queue.join()

def main(args):
    input_queue = queue.Queue()

    match args.tipo:
        case TipoArgumento.SERIAL:
            archivo_queue = queue.Queue()

            threading.Thread(target = escribir_archivo, args = (
                args.archivo_output, args.separador, args.periodo, IteratableQueue(archivo_queue),
            )).start()

            threading.Thread(target = lectura_archivo, args = (
                args.comm, args.baudrate, args.timeout, args.header, 
                MultipleQueue(input_queue, archivo_queue),
            )).start()

        case TipoArgumento.ARCHIVO:
            threading.Thread(target = lectura_archivo, args = (
                args.archivo_input, args.separador_input,
                input_queue,
            )).start()

    graficar_datos(args.periodo, args.puntos, IteratableQueue(input_queue))

if __name__ == "__main__":
    try: 
        args = parse_args(docopt(__doc__, version = "0.1.0"))
        main(args)

    except KeyboardInterrupt:
        print("Terminando lectura de arduino...")
