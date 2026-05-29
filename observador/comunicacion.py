import serial # Libreria pyserial
import struct
import sys
import threading
import queue

import argparse
import numpy as np
import matplotlib.pyplot as plt
from enum import IntEnum, auto

TAM_FLOAT = 4

class Variable(IntEnum):
    ACCION_DE_CONTROL = 0
    ANGULO_PLATAFORMA = 1
    POSICION_CARRO = 2
    ERROR_EN_POSICION = 3
    TIEMPO_TRANSCURRIDO_US = 4

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
        axTiempo.plot(self.tiempo, 1000 * self.periodo * np.ones(self.cantidadPuntos))

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

        self.lineaControl.set_ydata(self.datos[Variable.ACCION_DE_CONTROL, :])

        self.lineaTheta.set_ydata(self.datos[Variable.ANGULO_PLATAFORMA, :])

        self.lineaPosicion.set_ydata(self.datos[Variable.POSICION_CARRO, :])
        self.lineaReferencia.set_ydata(self.datos[Variable.POSICION_CARRO, :] + self.datos[Variable.ERROR_EN_POSICION, :])

        self.lineaTiempo.set_ydata(self.datos[Variable.TIEMPO_TRANSCURRIDO_US, :] / 1000)

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
        required = True,
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
