import threading
import queue
import signal

import numpy as np
import matplotlib.pyplot as plt
from enum import IntEnum, auto
import comunicacion_lib as cl

# Para correrlo ejemplo:
# python3 comuncacion.py serial --comm COMM3 -o mediciones/observaciones.csv
# python3 comuncacion.py archivo -i mediciones/observaciones.csv --separador-input=,

class Variable(IntEnum):
    ACCION_DE_CONTROL = 0

    THETA = auto()
    OMEGA = auto() 

    TIEMPO_TRANSCURRIDO = auto()

    CANTIDAD = auto()

class GraficoGeneral(cl.Grafico):
    def __init__(self, periodo, cantidad_puntos):
        super().__init__(cantidad_puntos, Variable)
        self.cantidad_puntos = cantidad_puntos
        self.periodo = periodo

        self.tiempo = self.periodo * np.arange(0, self.cantidad_puntos) 

    def iniciar_plot(self):
        self.figure = plt.figure(layout = "constrained")
        axis = self.figure.subplot_mosaic([
            ["Control", "Control"],
            ["Theta", "Omega"],
            ["Tiempo", "Tiempo"],
        ])
        self.axis = tuple([ axis[i] for i in ["Control", "Theta", "Omega", "Tiempo"] ])
        ax_control, ax_theta, ax_omega, ax_tiempo = self.axis

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

        # Plot angulo
        self.linea_theta, = ax_theta.plot(self.tiempo, ceros)

        ax_theta.set_title("Angulo de la plataforma", fontsize = 12)
        ax_theta.set_ylabel("Angulo [deg]", fontsize = 10)
        ax_theta.legend(loc = "upper right")
        ax_theta.set_ylim(auto = True)
        ax_theta.grid(True)

        # Plot velocidad angular
        self.linea_omega, = ax_omega.plot(self.tiempo, ceros)

        ax_omega.set_title("Velocidad angular de la plataforma", fontsize = 12)
        ax_omega.set_ylabel("Velocidad angular [deg/s]", fontsize = 10)
        ax_omega.legend(loc = "upper right")
        ax_omega.set_ylim(auto = True)
        ax_omega.grid(True)

        # Plot tiempo transcurrido
        self.linea_tiempo, = ax_tiempo.plot(self.tiempo, ceros)

        ax_tiempo.set_title("Velocidad del carro", fontsize = 12)
        ax_tiempo.set_ylabel("Velocidad [cm/s]", fontsize = 10)
        ax_tiempo.legend(loc = "upper right")
        ax_tiempo.set_ylim(auto = True)
        ax_tiempo.grid(True)

        self.figure.suptitle("Datos generales", fontsize = 14)

    def actualizar_datos(self, datos):
        self.linea_control.set_ydata(datos.accion_de_control)

        self.linea_theta.set_ydata(datos.theta)
        self.linea_omega.set_ydata(datos.omega)

        self.linea_tiempo.set_ydata(datos.tiempo_transcurrido)

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
        grafico = GraficoGeneral(args.periodo, args.puntos)
        cl.graficar_datos(grafico, args.batch_len, cl.IteratableQueue(input_queue))

    except:
        handle_cancel()

if __name__ == "__main__":
    main(cl.parse_args())
