#!/usr/bin/env bash

while [[ $# -gt 0 ]]; do
  case $1 in
    -kp)
      KP="$2"
      shift # past argument
      shift # past value
      ;;
    -ki)
      KI="$2"
      shift # past argument
      shift # past value
      ;;
    -f|--file)
      ARCHIVO="$2"
      shift # past argument
      shift # past value
      ;;
    -d|--directory)
      DIRECTORIO="$2"
      shift # past argument
      shift # past value
      ;;
  esac
done

if [ ! -v KP ]; then
  echo "No se setteo kp"
  exit -1
fi

if [ ! -v KI ]; then
  echo "No se asigno ki, se usa ki=0"
  KI=0
fi

if [ ! -v ARCHIVO ]; then
  echo "No se asigno el nombre del archivo"
  exit -1
fi

if [ ! -v DIRECTORIO ]; then
  DIRECTORIO="mediciones"
fi


path=$(printf "%s/%s_kp_%.2f_ki_%.2f.csv" $DIRECTORIO $ARCHIVO $KP $KI)
printf "Guardando en: %s\n" $path

# Usamos el file descriptor 3 para la informacion del archivo
python3 arduino_serial_comentado.py 3>$path
