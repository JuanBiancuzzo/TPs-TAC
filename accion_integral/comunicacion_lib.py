import serial # Libreria pyserial
import struct
import queue

from docopt import docopt # Libreria docopt-ng
import numpy as np
import matplotlib.pyplot as plt
from enum import Enum
from dataclasses import make_dataclass
from abc import ABC, abstractmethod

class Grafico(ABC):
    def __init__(self, cantidad_puntos, variables):
        self.cantidad_puntos = cantidad_puntos

        self.raw_datos = np.zeros((variables.CANTIDAD, self.cantidad_puntos)) 
        self.punto_actual = 0

        data_fields = [
            (variables(i).name.lower(), np.ndarray) 
            for i in range(variables.CANTIDAD) 
        ]

        datos = make_dataclass("Datos", data_fields)
        def actualizar_variables_dato(nuevos_datos):
            valores = {}
            for i in range(variables.CANTIDAD):
                valores[variables(i).name.lower()] = nuevos_datos[i, :]
            return datos(**valores)
        self.actualizar_variables_dato = actualizar_variables_dato

    def _iniciar_plot(self):
        plt.ion() # Hacemos que sea interactivo el plot aka actualizable
        self.iniciar_plot()

    @abstractmethod
    def iniciar_plot(self):
        pass

    def _agregar_datos(self, nuevosDatos):
        if self.punto_actual >= self.cantidad_puntos - 1:
            self.punto_actual = self.cantidad_puntos - 1
            # Desplazamos un dato (-1) cuando se llenan
            self.raw_datos = np.roll(self.raw_datos, -1, axis = 1)

        else:
            self.punto_actual += 1

        for posicion, nuevoDato in enumerate(nuevosDatos):
            self.raw_datos[posicion, self.punto_actual] = nuevoDato

        datos = self.actualizar_variables_dato(self.raw_datos)
        self.actualizar_datos(datos)

    @abstractmethod
    def actualizar_datos(self, datos):
        pass

    def _finalizar_plot(self):
        plt.ioff()
        plt.show()

class Argumentos:
    class Tipo(Enum):
        SERIAL = "Comunicacion serial"
        ARCHIVO = "Archivo input"

    def __init__(self, periodo, puntos):
        self.periodo = periodo
        self.puntos = puntos
    
    def serial(self, comm, baudrate, header, timeout):
        self.tipo = Argumentos.Tipo.SERIAL
        self.comm = comm
        self.baudrate = baudrate
        self.header = header
        self.timeout = timeout

    def archivo_output(self, archivo_output, separador_output):
        self.tipo = Argumentos.Tipo.SERIAL
        self.archivo_output = archivo_output
        self.separador_output = separador_output

    def archivo_input(self, archivo_input, separador_input):
        self.tipo = Argumentos.Tipo.ARCHIVO
        self.archivo_input = archivo_input
        self.separador_input = separador_input

def parse_args(doc):
    dicc_args = docopt(doc, version = "0.2.0")
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

def lectura_serial(comm, baudrate, timeout, header, cant_variables, output_queue):
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

        return False

    with serial.Serial(comm, baudrate, timeout = timeout) as ser:
        print(f"Conectado al puerto: {ser.portstr}")

        while ser.is_open:
            esperarHeader(ser)

            data_bytes = ser.read(TAM_FLOAT * cant_variables)
            if not data_bytes:
                break

            nuevos_datos = [ 0 ] * cant_variables
            for i in range(cant_variables):
                inicio = i * TAM_FLOAT
                final = (i + 1) * TAM_FLOAT
                nuevos_datos[i] = struct.unpack('<f', data_bytes[inicio:final])[0]

            if not output_queue.put(nuevos_datos):
                break

    output_queue.shutdown()

def lectura_archivo(nombre_archivo, separador, output_queue):
    with open(nombre_archivo, "r") as archivo:
        _ = archivo.readline() # Descartamos header
        for linea in archivo:
            nuevos_datos = [ 
                float(valor) 
                for valor in linea.strip().split(separador) 
            ][1:] # Descartamos la primera columna

            if not output_queue.put(nuevos_datos):
                break

    output_queue.shutdown()

def escribir_archivo(nombre_archivo, separador, variables, periodo, input_queue):
    with open(nombre_archivo, "w") as archivo:
        nombres = [ 
            variables(i).name.replace("_", " ").capitalize() 
            for i in range(variables.CANTIDAD) 
        ]
        archivo.write(f"{separador.join(["Tiempo", *nombres])}\n")

        for i, nuevos_datos in enumerate(input_queue):
            input = [periodo * i, *nuevos_datos]
            input = map(lambda num: str(num), input)
            archivo.write(f"{separador.join(input)}\n")

def graficar_datos(grafico, input_queue):
    grafico._iniciar_plot()

    for nuevos_datos in input_queue:
        grafico._agregar_datos(nuevos_datos)

    grafico._finalizar_plot()

class IteratableQueue:
    def __init__(self, queue):
        self.queue = queue

    def __iter__(self):
        while True:
            try:
                valor = self.queue.get()
                self.queue.task_done()
                yield valor

            except queue.ShutDown:
                break

class MultipleQueue:
    def __init__(self, *queues):
        self.queues = list(queues)

    def put(self, valor):
        try:
            for queue in self.queues:
                queue.put(valor)
            return True
        except:
            return False

    def shutdown(self, immediate = False):
        for queue in self.queues:
            queue.shutdown(immediate)

    def join(self):
        for queue in self.queues:
            queue.join()

