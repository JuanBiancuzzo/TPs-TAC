import serial # Libreria pyserial
import struct
import argparse
import sys

TAM_FLOAT = 4
SERIAL_TIMEOUT = 2

def parse_args():
    parser = argparse.ArgumentParser(description="Serial communication with Arduino")

    parser.add_argument(
        "-c", "--comm", 
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
        "-v", "--variables", 
        type = int, 
        help = "Cantidad de variables",
    )

    return parser.parse_args()


def main(args):
    largoHeader = len(args.header)
    headerBytes = [ bytes(b, "utf-8") for b in args.header ]
    def leerHeader(ser):
        header_receive = 0
        while header_receive < largoHeader:
            data_bytes = ser.read(1)
            if data_bytes == headerBytes[header_receive]:
                header_receive += 1
            else:
                header_receive = 0

    with serial.Serial(args.comm, args.baudrate, timeout = SERIAL_TIMEOUT) as ser:
        print(f"Conectado al puerto: {ser.portstr}", file = sys.stderr)

        while True:
            leerHeader(ser)

            dataBytes = ser.read(TAM_FLOAT * args.variables) # payload
            if not dataBytes:# or len(data_bytes) < 28:
                break

            nuevosDatos = [ 0 ] * args.variables
            for i in range(args.variables):
                inicio = i * TAM_FLOAT
                final = (i + 1) * TAM_FLOAT
                nuevosDatos[i] = struct.unpack('<f', dataBytes[inicio:final])[0]

            print(";".join(map(lambda valor: "{:.4f}".format(valor), nuevosDatos)))

if __name__ == "__main__":
    try: 
        main(parse_args())

    except KeyboardInterrupt:
        print("Terminando lectura de arduino...", file = sys.stderr)
