# NEXO

Este es un Sistema de Gestión de Bases de Datos (DBMS) no relacional desarrollado desde cero en C++. El motor utiliza una arquitectura basada en páginas, persistencia en archivos binarios y un sistema de indexación mediante Árboles B para optimizar la búsqueda y recuperación de información.

    Características Principales

-Motor de Almacenamiento (Storage Engine): Implementación de Heap Files con manejo de páginas de 4096 bytes.   
-Indexación Eficiente: Uso de Árboles B de grado parametrizable para búsquedas con complejidad de tiempo O(log n).
-Motor de Consultas (Query Engine): Incluye un Tokenizer y un Parser que interpretan comandos de tipo NoSQL/JSON.
-Persistencia Binaria: Serialización personalizada de mapas de datos para almacenamiento en disco (.bin y .idx).

    Arquitectura Técnica

El proyecto se divide en tres capas fundamentales:

-Capa de Almacenamiento (Storage): Gestiona la lectura/escritura física en disco, el manejo de metadatos y la organización de registros en slots dentro de cada página.

-Capa de Indexación (B-tree): Controla la lógica de los nodos, divisiones (splits) y la jerarquía de la base de datos para asegurar que los datos sean accesibles rápidamente mediante IDs.

-Capa de Lenguaje (Query Engine): Procesa las cadenas de texto del usuario, las convierte en tokens y genera un plan de ejecución para las operaciones CRUD.


    Instalación y Uso (W64Devkit)

Dado que el proyecto utiliza herramientas de compilación de entorno Linux, se recomienda el uso de w64devkit.

1. Requisitos
w64devkit (GCC, Make).

2. Compilación
Abre la consola de w64devkit en la raíz del proyecto y ejecuta:

    make

Esto generará los archivos objeto en /obj y el ejecutable en /bin.

3. Ejecución

./bin/gestor_db.exe

    Estructura del Proyecto:

├── bin/                # Ejecutable final (.exe)
├── obj/                # Archivos objeto (.o)
├── include/            # Archivos de cabecera (.h)
├── src/                # Código fuente (.cpp)
│   ├── B-tree/         # Lógica del Árbol B
│   ├── query_engine/   # Tokenizer y Parser
│   ├── Storage/        # Gestión de archivos y serialización
│   └── main.cpp        # Punto de entrada
├── Makefile            # Script de automatización de compilación
└── README.md

.-Ejemplos de Consultas:

El motor soporta una sintaxis inspirada en JSON:

-Crear Colección: CREATE "estudiantes" ["id", "nombre", "carrera"]

-Insertar: INSERT INTO "estudiantes" {"id": 101, "nombre": "Rafael", "carrera": "Sistemas"}

-Buscar: GET "estudiantes" {"id": 101}

Autores:

-Rafael Angel Ramirez Ibarra
GitHub User:Rafael-A69

-Rafael Andres Castillo Ortega
GitHub User:Chinoto03

-Keny Joan Rondon Perez
GitHub User:kenyosiosi

.-Institución

Universidad de Los Andes.

Facultad: Ingeniería.

Curso: Programación 3.

Profesor: Alejandro Mujica.