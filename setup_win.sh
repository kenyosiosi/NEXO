#!/bin/bash
# NEXO ENGINE - Script de Configuración para Windows (MinGW)

echo -e "\e[33m--- CONFIGURANDO ENTORNO NEXO (WINDOWS) ---\e[0m"

# 1. Crear estructura de carpetas
mkdir -p build
mkdir -p data
mkdir -p libs

cd libs

# 2. SFML 2.6.1 (Binarios oficiales para GCC MinGW)
if [ ! -d "SFML-2.6.1" ]; then
    echo "Descargando SFML 2.6.1..."
    curl -L -o sfml.zip https://www.sfml-dev.org/files/SFML-2.6.1-windows-gcc-13.1.0-mingw-64-bit.zip
    unzip sfml.zip
    rm sfml.zip
fi

# 3. ImGui v1.89 e ImGui-SFML
if [ ! -d "imgui" ]; then
    echo "Descargando ImGui..."
    git clone --branch v1.89 https://github.com/ocornut/imgui.git
fi

if [ ! -d "imgui-sfml" ]; then
    echo "Descargando ImGui-SFML..."
    git clone --branch v2.6 https://github.com/SFML/imgui-sfml.git
fi

cd ..

# 4. GESTIÓN DE DLLS DE RUNTIME (libstdc++-6.dll, libgcc_s_seh-1.dll, libwinpthread-1.dll)
echo -e "\e[34m[RUNTIME] Buscando librerías del compilador...\e[0m"
FILES=("libstdc++-6.dll" "libgcc_s_seh-1.dll" "libwinpthread-1.dll")

for dll in "${FILES[@]}"; do
    if [ ! -f "build/$dll" ]; then
        # Intenta copiar desde la carpeta bin de MinGW (si está en el PATH)
        cp "$(which $dll)" "build/" 2>/dev/null
        if [ $? -eq 0 ]; then
            echo "✓ $dll copiada desde el sistema a /build"
        else
            echo "⚠ $dll no encontrada en el sistema. Descargando versión genérica..."
            # Link de respaldo para descargar DLLs de runtime comunes
            curl -L -o "build/$dll" "https://github.com/brechtsanders/winlibs_mingw/raw/master/bin/$dll" 2>/dev/null
        fi
    fi
done

echo -e "\e[32m--- ENTORNO LISTO ---\e[0m"
echo "Ejecuta: cd build && cmake -G \"MinGW Makefiles\" .. && make"