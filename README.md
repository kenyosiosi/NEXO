# NEXO Engine - Workspace UI

Este es un Sistema de Gestión de Bases de Datos (DBMS) no relacional desarrollado desde cero en C++. El motor utiliza una arquitectura basada en páginas, persistencia en archivos binarios, un sistema de indexación mediante Árboles B para optimizar la búsqueda y una **interfaz gráfica interactiva (GUI)** para la gestión visual de los datos.

### 🚀 Características Principales

- **Interfaz Gráfica (Workspace UI):** Panel de control desarrollado con ImGui y SFML que incluye un explorador de colecciones, visualizador de documentos en formato árbol (JSON) y una terminal interactiva.
- **Motor de Almacenamiento (Storage Engine):** Implementación de Heap Files con manejo de páginas de 4096 bytes y soporte para **Auto-Compactación** (eliminación física de registros muertos).
- **Indexación Eficiente:** Uso de Árboles B de grado parametrizable para búsquedas con complejidad de tiempo O(log n). Los índices se reconstruyen automáticamente tras una compactación.
- **Motor de Consultas (Query Engine):** Tokenizer y Parser integrados que interpretan comandos de tipo NoSQL directamente desde la terminal visual.
- **Persistencia Automatizada:** Auto-descubrimiento de esquemas (`.schema`), índices (`.idx`) y archivos binarios (`.bin`) al iniciar el motor.



### ⚙️ Arquitectura Técnica

El proyecto se divide en tres capas fundamentales:

1. **Capa de Interfaz (UI):** Gestiona el renderizado visual, la interacción del usuario y la sincronización en tiempo real con el estado del disco.
2. **Capa de Lenguaje (Query Engine):** Procesa las cadenas de texto del usuario, las convierte en tokens y genera un plan de ejecución (AST) para las operaciones CRUD.
3. **Capa de Almacenamiento e Índices (Storage & B-Tree):** Gestiona la lectura/escritura física en disco, fragmentación, control de slots dentro de las páginas y la jerarquía de los nodos del árbol para un acceso rápido.



### 🛠️ Instalación y Compilación

El proyecto ha sido migrado a **CMake** para garantizar su portabilidad multiplataforma. 

#### 1. Requisitos Previos
* **CMake** (v3.12 o superior).
* **Git** (para descargar dependencias).
* **Compilador C++17:** * Windows: MinGW-w64 (Se recomienda usar la terminal **Git Bash**).
  * Linux: GCC (`build-essential`).

#### 2. Configurar el Entorno (Descarga de dependencias)
El proyecto incluye scripts que descargan automáticamente ImGui y SFML. Abre tu terminal en la raíz del proyecto y ejecuta:

**En Windows (Usa Git Bash):**
```bash
bash setup_win.sh

```

**En Linux:**

```bash
bash setup_linux.sh

```

#### 3. Compilación con CMake

Una vez que el script termine de preparar el entorno, compila el motor con los siguientes comandos:

```bash
cd build
cmake ..  # En Windows puede que necesites: cmake -G "MinGW Makefiles" ..
cmake --build .

```

#### 4. Ejecución

El ejecutable se generará dentro de la carpeta `build`. Además, en Windows, CMake copiará automáticamente las `.dll` necesarias de SFML.

```bash
# Estando en la carpeta build:
./NexoEngine

```



### 📂 Estructura del Proyecto

```text
├── build/              # Archivos de compilación y ejecutable final
│   └── data/           # Archivos de la base de datos (.bin, .idx, .schema)
├── include/            # Archivos de cabecera (.h)
├── libs/               # Dependencias externas (SFML, ImGui) bajadas por el script
├── src/                # Código fuente (.cpp)
│   ├── B-tree/         # Lógica del Árbol B
│   ├── Query_engine/   # Tokenizer, Parser y Ejecutor
│   ├── Storage/        # Gestión de HeapFiles y serialización
│   └── main.cpp        # Interfaz Gráfica (ImGui + SFML)
├── CMakeLists.txt      # Configuración de compilación multiplataforma
├── setup_win.sh        # Script de entorno para Windows
├── setup_linux.sh      # Script de entorno para Linux
└── README.md

```



### 💻 Ejemplos de Consultas (Terminal NEXO)

El motor soporta una sintaxis inspirada en JSON que puedes escribir directamente en la terminal de la aplicación:

* **Crear Colección:** `CREATE "usuarios" {fields: "nombre,edad,cedula"}`
*(Inicializa el archivo usuarios.bin y su esquema).*
* **Índices Secundarios:** `INDEX "nombre" ON "usuarios"`
*(Crea el archivo usuarios_nombre.idx para búsquedas rápidas).*
* **Insertar:** `INSERT INTO "usuarios" {"id": "1", "nombre": "Keny", "edad": "21"}`
*(Guarda el registro y actualiza los árboles B correspondientes).*
* **Buscar:** `GET FROM "usuarios" {"id": "1"}`
*(Busca el registro usando el índice).*
* **Actualizar:** `UPDATE "usuarios" SET {"edad": "22"} WHERE {"id": "1"}`
*(Aplica borrado lógico al registro original y crea uno nuevo al final del archivo físico).*
* **Borrar:** `DELETE FROM "usuarios" WHERE {"id": "2"}`
*(Aplica un Tombstone lógico al registro).*
* **Explorar:** `SCAN "usuarios"`
*(Muestra todos los registros vivos de la colección).*



### 👥 Autores

* **Rafael Angel Ramirez Ibarra** - [Rafael-A69](https://github.com/Rafael-A69)
* **Rafael Andres Castillo Ortega** - [Chinoto03](https://github.com/Chinoto03)
* **Keny Joan Rondon Perez** - [kenyosiosi](https://github.com/kenyosiosi)

### 🏛️ Institución

**Universidad de Los Andes** Facultad: Ingeniería

Curso: Programación 3

Profesor: Alejandro Mujica