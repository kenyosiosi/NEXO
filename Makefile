# Variables
CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -g -O0
# -I especifica dónde buscar los headers (.hpp)
INCLUDES  = -Iinclude

# Carpetas
SRCDIR    = src
OBJDIR    = obj
BINDIR    = bin

# Archivos
# Buscamos todos los .cpp en src/
SOURCES   = $(wildcard $(SRCDIR)/*.cpp)
# Creamos la lista de .o basados en los .cpp encontrados
OBJECTS   = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
# Nombre del ejecutable final
TARGET    = $(BINDIR)/gestor_db.exe

# Regla principal (la que se ejecuta al escribir 'make')
all: setup $(TARGET)

# Crear carpetas si no existen
setup:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Enlace final: Une todos los .o para crear el .exe
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)
	@echo "--- Compilación terminada con éxito: $(TARGET) ---"

# Compilación de objetos: Convierte cada .cpp en un .o
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Limpiar archivos generados
.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "--- Limpieza completada ---"