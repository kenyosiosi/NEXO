# Variables
CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -g -O0
INCLUDES  = -Iinclude

# Carpetas
SRCDIR    = src
OBJDIR    = obj
BINDIR    = bin

# --- 1.-CAMBIO ---
# Busca TODOS los .cpp dentro de src y sus subcarpetas
# Busca archivos en las carpetas específicas (más seguro)
SOURCES   = $(wildcard $(SRCDIR)/*.cpp) \
            $(wildcard $(SRCDIR)/Storage/*.cpp) \
            $(wildcard $(SRCDIR)/B-tree/*.cpp) \
            $(wildcard $(SRCDIR)/query_engine/*.cpp)

# Genera los objetos
OBJECTS   = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SOURCES))


TARGET    = $(BINDIR)/gestor_db.exe

all: setup $(TARGET)

setup:
	@mkdir -p $(BINDIR)
	@# No creamos OBJDIR aquí porque lo haremos dinámicamente

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)
	@echo "--- Compilacion terminada con exito: $(TARGET) ---"

# --- OTRO CAMBIO---
# Esta regla compila el .o y crea la subcarpeta en 'obj' si no existe
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "--- Limpieza completada ---"