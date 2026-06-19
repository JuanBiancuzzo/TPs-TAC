import threading
import queue
import signal

import numpy as np
import matplotlib.pyplot as plt
from enum import IntEnum, auto
import comunicacion_lib as cl

# Para correrlo ejemplo:
# python3 visualizar_real_estados.py serial --comm COMM3 -o mediciones/observaciones.csv
# python3 visualizar_real_estados.py archivo -i mediciones/observaciones.csv --separador-input=,

class Variable(IntEnum):
    ACCION_DE_CONTROL = 0

    REFERENCIA_POSICION = auto()
    REFERENCIA_THETA = auto()

    THETA_MEDIDA = auto()
    THETA_OBSERVADA = auto() 

    OMEGA_MEDIDA = auto() 
    OMEGA_OBSERVADA = auto()

    POSICION_MEDIDA = auto() 
    POSICION_OBSERVADA = auto()

    VELOCIDAD_OBSERVADA = auto()

    TIEMPO_TRANSCURRIDO = auto()

    CANTIDAD = auto()

class GraficoRealimentacionEstados(cl.Grafico):
    def __init__(self, periodo, cantidad_puntos):
        super().__init__(cantidad_puntos, Variable)
        self.cantidad_puntos = cantidad_puntos
        self.periodo = periodo

        self.tiempo = self.periodo * np.arange(0, self.cantidad_puntos) 

    def iniciar_plot(self):
        self.figure = plt.figure(layout = "constrained")
        axis = self.figure.subplot_mosaic([
            ["Theta", "Posicion"],
            ["Omega", "Velocidad"],
            ["Control", "Control"],
        ])
        self.axis = tuple([ axis[i] for i in ["Control", "Theta", "Omega", "Posicion", "Velocidad"] ])
        ax_control, ax_theta, ax_omega, ax_posicion, ax_velocidad = self.axis

        # Lo inicializamos en ceros, ya que la actualizacion va a agarrar los valores reales
        ceros, unos = np.zeros(self.cantidad_puntos), np.ones(self.cantidad_puntos)

        # Plot de control
        pwm_minimo, pwm_maximo = -1038, 2152
        for pwm_rango in [pwm_minimo, pwm_maximo]:
            ax_control.plot(self.tiempo, pwm_rango * unos)
        self.linea_control, = ax_control.plot(self.tiempo, ceros)

        ax_control.set_title("Acción de control", fontsize = 12)
        ax_control.set_ylabel("PWM [us]", fontsize = 10)
        ax_control.set_ylim(pwm_minimo, pwm_maximo)
        ax_control.grid(True)

        # Plot estimacion angulo
        self.linea_theta_medida, = ax_theta.plot(self.tiempo, ceros, label = "Medicion")
        self.linea_theta_observada, = ax_theta.plot(self.tiempo, ceros, label = "Observado")

        ax_theta.set_title("Angulo de la plataforma", fontsize = 12)
        ax_theta.set_ylabel("Angulo [deg]", fontsize = 10)
        ax_theta.legend(loc = "upper right")
        ax_theta.set_ylim(-15, 22)
        ax_theta.grid(True)

        # Plot estimacion velocidad angular
        self.linea_omega_medida, = ax_omega.plot(self.tiempo, ceros, label = "Medicion")
        self.linea_omega_observada, = ax_omega.plot(self.tiempo, ceros, label = "Observado")

        ax_omega.set_title("Velocidad angular de la plataforma", fontsize = 12)
        ax_omega.set_ylabel("Velocidad angular [deg/s]", fontsize = 10)
        ax_omega.legend(loc = "upper right")
        ax_omega.set_ylim(auto = True)
        ax_omega.grid(True)

        # Plot estimacion posicion
        self.linea_posicion_medida, = ax_posicion.plot(self.tiempo, ceros, label = "Medicion")
        self.linea_posicion_observada, = ax_posicion.plot(self.tiempo, ceros, label = "Observado")

        ax_posicion.set_title("Posicion del carro", fontsize = 12)
        ax_posicion.set_ylabel("Posicion [cm]", fontsize = 10)
        ax_posicion.legend(loc = "upper right")
        ax_posicion.set_ylim(auto = True)
        ax_posicion.grid(True)

        # Plot estimacion velocidad
        self.linea_velocidad_medida, = ax_velocidad.plot(self.tiempo, ceros, label = "Medicion")
        self.linea_velocidad_observada, = ax_velocidad.plot(self.tiempo, ceros, label = "Observado")

        ax_velocidad.set_title("Velocidad del carro", fontsize = 12)
        ax_velocidad.set_ylabel("Velocidad [cm/s]", fontsize = 10)
        ax_velocidad.legend(loc = "upper right")
        ax_velocidad.set_ylim(auto = True)
        ax_velocidad.grid(True)

        self.figure.suptitle("Realimentacion por variables de estados", fontsize = 14)

    def actualizar_datos(self, datos):
        self.linea_control.set_ydata(datos.accion_de_control)

        self.linea_theta_medida.set_ydata(datos.theta_medida)
        self.linea_theta_observada.set_ydata(datos.theta_observada)

        self.linea_omega_medida.set_ydata(datos.omega_medida)
        self.linea_omega_observada.set_ydata(datos.omega_observada)

        self.linea_posicion_medida.set_ydata(datos.posicion_medida)
        self.linea_posicion_observada.set_ydata(datos.posicion_observada)

        posicion_shifteada = np.zeros(self.cantidad_puntos)
        posicion_shifteada[:-1] = datos.posicion_medida[1:]

        velocidad_medida = (posicion_shifteada - datos.posicion_medida) / self.periodo

        self.linea_velocidad_medida.set_ydata(velocidad_medida)
        self.linea_velocidad_observada.set_ydata(datos.velocidad_observada)

        for ax in list(self.axis):
            ax.relim() 
            ax.autoscale_view() 
            self.figure.canvas.blit(ax.bbox)

        self.figure.canvas.flush_events()

def main(args):
    handle_cancel = lambda: print("\nManejando la interrupcion")

    input_queue = queue.Queue()
    match args.tipo:
        case cl.Argumentos.Tipo.SERIAL:
            archivo_queue = queue.Queue()

            threading.Thread(target = cl.escribir_archivo, args = (
                args.archivo_output, args.separador_output, Variable, args.periodo, 
                cl.IteratableQueue(archivo_queue),
            )).start()

            serial_output = cl.MultipleQueue(input_queue, archivo_queue)
            threading.Thread(target = cl.lectura_serial, args = (
                args.comm, args.baudrate, args.timeout, args.header, Variable.CANTIDAD, serial_output,
            )).start()

            def handle_cancel():
                print("\nManejando el cierre de la comunicacion serial")
                serial_output.shutdown(immediate = True)

        case cl.Argumentos.Tipo.ARCHIVO:
            archivo_output = cl.MultipleQueue(input_queue)
            threading.Thread(target = cl.lectura_archivo, args = (
                args.archivo_input, args.separador_input, archivo_output, 
            )).start()

            def handle_cancel():
                print("\nManejando el cierre de la comunicacion con el archivo")
                archivo_output.shutdown(immediate = True)

    def handle_ctrl_c(signum, frame):
        handle_cancel()
        plt.close("all")
    signal.signal(signal.SIGINT, handle_ctrl_c)

    try:
        grafico = GraficoRealimentacionEstados(args.periodo, args.puntos)
        cl.graficar_datos(grafico, cl.IteratableQueue(input_queue))

    except:
        handle_cancel()

if __name__ == "__main__":
    doc = """Serial communication with Arduino.

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
    main(cl.parse_args(doc))
