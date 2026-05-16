#!/bin/bash
# NEXO ENGINE - Script de Configuración para Linux

echo "--- CONFIGURANDO NEXO ENGINE (LINUX) ---"

# 1. Instalar dependencias esenciales
sudo apt-get update
sudo apt-get install -y build-essential git curl unzip libsfml-dev

# 2. Estructura de carpetas
mkdir -p build
mkdir -p data
mkdir -p libs

# 3. Descargar librerías externas
cd libs
if [ ! -d "imgui" ]; then
    git clone --branch v1.89 https://github.com/ocornut/imgui.git
fi

if [ ! -d "imgui-sfml" ]; then
    git clone --branch v2.6 https://github.com/SFML/imgui-sfml.git
fi
cd ..

# Nota: Las DLLs de Windows no son necesarias para ejecutar en Linux nativo,
# pero se crean los directorios para mantener paridad en la estructura.
echo "--- ENTORNO LISTO ---"
echo "Ejecuta: cd build && cmake .. && make"