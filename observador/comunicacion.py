import serial # Libreria pyserial
import struct
import threading
import queue

import argparse
import numpy as np
import matplotlib.pyplot as plt
from enum import IntEnum, auto

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
    def __init__(self, periodo):
        self.cantidadPuntos = 1000
        self.periodo = periodo
        self.datos = np.zeros((Variable.CANTIDAD, self.cantidadPuntos)) 
        self.tiempo = self.periodo * np.arange(0, self.cantidadPuntos) 
        self.puntoActual = 0

    def iniciarPlot(self):
        plt.ion() # Hacemos que sea interactivo el plot aka actualizable
        self.figure = plt.figure(layout="constrained")
        axis = self.figure.subplot_mosaic([
            [1, 1],
            [2, 4],
            [3, 5],
        ])
        axControl, axTheta, axOmega, axPosicion, axVelocidad = tuple([ axis[i] for i in range(5) ])

        # Lo inicializamos en ceros, ya que la actualizacion va a agarrar los valores reales
        ceros = np.zeros(self.cantidadPuntos)
        
        # Plot de control
        minMicrosRango = -1038 # -44 grados
        maxMicrosRango = 2152 # 66 grados

        axControl.plot(self.tiempo, minMicrosRango * np.ones(self.cantidadPuntos))
        self.lineaControl, = axControl.plot(self.tiempo, ceros)
        axControl.plot(self.tiempo, maxMicrosRango * np.ones(self.cantidadPuntos))

        axControl.grid(True)
        axControl.set_ylabel("Acción de control")

        # Plot estimacion angulo
        self.lineaThetaMedida, = axTheta.plot(self.tiempo, ceros, label = "Medicion")
        self.lineaThetaEstimada, = axTheta.plot(self.tiempo, ceros, label = "Estimada")

        axTheta.grid(True)
        axTheta.set_ylabel("Angulo de la plataforma [deg]")

        # Plot estimacion velocidad angular
        self.lineaOmegaMedida, = axOmega.plot(self.tiempo, ceros, label = "Medicion")
        self.lineaOmegaEstimada, = axOmega.plot(self.tiempo, ceros, label = "Estimada")

        axTheta.grid(True)
        axTheta.set_ylabel("Velocidad angular de la plataforma [deg/s]")

        # Plot estimacion posicion
        self.lineaPosicionMedida, = axPosicion.plot(self.tiempo, ceros, label = "Medicion")
        self.lineaPosicionEstimada, = axPosicion.plot(self.tiempo, ceros, label = "Estimada")

        axPosicion.grid(True)
        axPosicion.set_ylabel("Posicion del carro [cm]")
        axPosicion.legend()

        # Plot estimacion velocidad
        self.lineaVelocidadMedida, = axVelocidad.plot(self.tiempo, ceros, label = "Medida")
        self.lineaVelocidadEstimada, = axVelocidad.plot(self.tiempo, ceros, label = "Estimada")

        axPosicion.grid(True)
        axPosicion.set_ylabel("Velocidad del carro [cm/s]")
        axPosicion.legend()

        self.figure.suptitle("Observadores", fontsize = 20)

    def agregarDatos(self, nuevosDatos):
        if self.puntoActual >= self.cantidadPuntos - 1:
            self.puntoActual = self.cantidadPuntos - 1
            # Desplazamos un dato (-1) cuando se llenan
            self.datos = np.roll(self.datos, -1, axis = 1)

        else:
            self.puntoActual += 1

        for posicion, nuevoDato in enumerate(nuevosDatos):
            self.datos[posicion, self.puntoActual] = nuevoDato

        self.lineaControl.set_ydata(self.datos[Variable.ACCION_DE_CONTROL, :])

        self.lineaThetaMedida.set_ydata(self.datos[Variable.THETA_MEDIDO, :])
        self.lineaThetaEstimada.set_ydata(self.datos[Variable.THETA_ESTIMADO, :])

        self.lineaOmegaMedida.set_ydata(self.datos[Variable.OMEGA_MEDIDA, :])
        self.lineaOmegaEstimada.set_ydata(self.datos[Variable.OMEGA_ESTIMADA, :])

        self.lineaPosicionMedida.set_ydata(self.datos[Variable.POSICION_MEDIDO, :])
        self.lineaPosicionEstimada.set_ydata(self.datos[Variable.POSICION_ESTIMADO, :])

        velocidad_estimada = self.datos[Variable.VELOCIDAD_ESTIMADA, :]
        velocidad_medida = (velocidad_estimada - np.roll(velocidad_estimada, -1)) / self.periodo

        # Limpiando la derivada
        velocidad_medida[self.puntoActual] = 0 
        velocidad_medida[-1] = 0

        self.lineaVelocidadMedida.set_ydata(velocidad_medida)
        self.lineaVelocidadEstimada.set_ydata(velocidad_estimada)

        self.figure.canvas.draw()
        self.figure.canvas.flush_events()

def parse_args():
    parser = argparse.ArgumentParser(description="Serial communication with Arduino")

    parser.add_argument(
        "-c", "--comm", 
        required = True,
        help = "Puerto de comunicación con el arduino"
    )
    parser.add_argument(
        "-b", "--baudrate", 
        type = int, 
        default = 115200,
        help = "Velocidad de la comunicación",
    )
    parser.add_argument(
        "-h", "--header", 
        default = "abcd",
        help = "Header de los mensajes",
    )
    parser.add_argument(
        "-t", "--timeout",
        default = 2,
        help = "Timeout en segundos de la lecutra de serial",
    )
    parser.add_argument(
        "-T", "--periodo", 
        type = float, 
        default = 0.02,
        help = "Periodo en segundos",
    )
    parser.add_argument(
        "-o", "--archivo-output", 
        required = True,
        help = "Nombre del archivo csv",
    )
    parser.add_argument(
        "-s", "--separador", 
        default = ";",
        help = "Separador del csv generado",
    )

    return parser.parse_args()

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

            nuevosDatos = [ 0 ] * Variable.CANTIDAD
            for i in range(Variable.CANTIDAD):
                inicio = i * TAM_FLOAT
                final = (i + 1) * TAM_FLOAT
                nuevosDatos[i] = struct.unpack('<f', data_bytes[inicio:final])[0]

            output_queue.put(nuevosDatos)

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

def graficar_datos(periodo, input_queue):
    grafico = Grafico(periodo)
    grafico.iniciarPlot()

    for nuevos_datos in input_queue:
        grafico.agregarDatos(nuevos_datos)

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
    serial_queue = queue.Queue()
    archivo_queue = queue.Queue()

    threading.Thread(target = graficar_datos, args = (
        args.periodo, IteratableQueue(serial_queue),
    )).start()

    threading.Thread(target = escribir_archivo, args = (
        args.archivo_output, args.separador, args.periodo, IteratableQueue(serial_queue),
    )).start()

    lectura_serial(
        args.comm, args.baudrate, args.timeout, args.header, 
        MultipleQueue(serial_queue, archivo_queue),
    )

if __name__ == "__main__":
    try: 
        args = parse_args()
        main(args)

    except KeyboardInterrupt:
        print("Terminando lectura de arduino...")
