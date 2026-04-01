import serial
import struct
import time
import csv
import numpy as np
import matplotlib.pyplot as plt

# Nota, esto se puede separar en 2 archivos, uno de leer la informacion especifica de Serial, y otro para recibir
# la informacion de stdin. Ejecutandolos "python3 leer_seria.py | python3 plot_data.py"
# Esto nos permite usar el comando "tee" para poder guardarnos los datos en caso de querer analizarlos despues,
# teniendo el commando "python3 leer_seria.py | tee output_data.csv |  python3 plot_data.py"

# Configurar puerto y baudrate
PORT = "COM5"
BAUDRATE = 115200
SERIAL_TIMEOUT = 2
CANT_VARIABLES = 9
VARIABLES_RECIBIDAS = (3, 3)
CANT_PUNTOS = 1000
HEADER = "abcd"

# Constantes calculables
LARGO_HEADER = len(HEADER)
HEADER_BYTES = [ bytes(b, "utf-8") for b in HEADER ]

# Constantes
FORMATO_TIEMPO = "%Y%m%d-%H%M%S"
TAM_FLOAT = 4

def plot(axis, datos, indices):
    axAcelerometro, axGiroscopio = axis

    plotAcelerometro(axAcelerometro, datos[:3, :], indices)
    plotAcelerometro(axAcelerometro, datos[3:, :], indices)

def plotAcelerometro(axis, datos, indices):
    pass

def plotGiroscopio(axis, datos, indices):
    pass

# --- Codigo de ejecucion ---
def leer_header(ser):
    header_receive = 0
    while header_receive < LARGO_HEADER:
        data_bytes = ser.read(1)
        if data_bytes == HEADER_BYTES[header_receive]:
            header_receive += 1
        else:
            header_receive = 0

if __name__ == "__main__":
    # Inicializar listas para guardar los datos recibidos y graficar
    y = np.zeros((CANT_VARIABLES, CANT_PUNTOS)) # 6 variables: ax, ay, az, gx, gy, gz
    x = np.arange(0, CANT_PUNTOS)  # vector de indices para graficar: [0,...,1000]

    # Permite que los plots sean iteractivos, posiblemente refiere que se pueden actualizar
    plt.ion()
    
    # Crear la figura que vamos a ir actualizando con los datos
    figure, axis = plt.subplots(*VARIABLES_RECIBIDAS, figsize = (10, 10))
    plot(axis, y, x)

    # Hay que ver como modularizar esto, porque los datos se actualizan 
    ax1, ax2 = axis

    # subfigura 1: ax, ay, az
    line11, = ax1.plot(x, y[0,:], color='r') # ax
    line12, = ax1.plot(x, y[1,:], color='g') # ay
    line13, = ax1.plot(x, y[2,:], color='b') # az
    ax1.legend(["ax","ay","az"])
    ax1.set_ylim([-15,15])
    ax1.grid(True)
    ax1.set_ylabel("Aceleraciones")

    # subfigura 2: gx, gy, gz
    line21, = ax2.plot(x, y[3,:], color='r')
    line22, = ax2.plot(x, y[4,:], color='g')
    line23, = ax2.plot(x, y[5,:], color='b')
    ax2.legend(["gx","gy","gz"])
    ax2.set_ylim([-10,10])
    ax2.grid(True)
    ax2.set_ylabel("Velocidades angulares")

    figure.suptitle("Labo. de Control - MPU6050", fontsize = 20)

    try: 
        ser = serial.Serial(PORT, BAUDRATE, timeout = SERIAL_TIMEOUT)
        print(f"Connected to serial port: {ser.portstr}")

        while True:
            leer_header(ser)

            data_bytes = ser.read(TAM_FLOAT * CANT_VARIABLES) # payload
            if not data_bytes:# or len(data_bytes) < 28:
                break

            # desplaza los elementos de la lista y en la que guardamos las mediciones
            y = np.roll(y,-1,axis=1)

            nuevos_datos = [ 0 ] * CANT_VARIABLES
            for i in range(CANT_VARIABLES):
                inicio = i * TAM_FLOAT
                final = (i + 1) * TAM_FLOAT
                nuevos_datos[i] = struct.unpack('<f', data_bytes[inicio:final])[0]

            # agregamos las nuevas lecturas recibidas
            y[:,-1] = nuevos_datos

            # actualizamos los plots
            line11.set_xdata(x)
            line11.set_ydata(y[0,:])
            line12.set_xdata(x)
            line12.set_ydata(y[1,:])
            line13.set_xdata(x)
            line13.set_ydata(y[2,:])
            line21.set_xdata(x)
            line21.set_ydata(y[3,:])
            line22.set_xdata(x)
            line22.set_ydata(y[4,:])
            line23.set_xdata(x)
            line23.set_ydata(y[5,:])

            # dibuja los valores actualizados
            figure.canvas.draw()
            figure.canvas.flush_events()


    except KeyboardInterrupt:
        print("Terminanding...")

    finally:
        # Cierra la conexión
        ser.close()
        print("Chaucha!")