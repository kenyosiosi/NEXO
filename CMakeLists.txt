cmake_minimum_required(VERSION 3.12)
project(NexoEngine VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COLOR_MAKEFILE ON)

message(STATUS "------------------------------------------------")
message(STATUS "  CONFIGURANDO NEXO ENGINE V1.2 (WORKSPACE UI)  ")
message(STATUS "------------------------------------------------")

# --- LOCALIZACIÓN DE DEPENDENCIAS ---
if(WIN32)
    # SFML descargado por setup_win.sh
    set(SFML_DIR "${CMAKE_SOURCE_DIR}/libs/SFML-2.6.1/lib/cmake/SFML")
endif()

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# --- RUTAS DE INCLUSIÓN ---
include_directories(
    include
    src
    libs/imgui
    libs/imgui-sfml
    libs/imgui/backends
    ${SFML_INCLUDE_DIR}
)

# --- FUENTES DEL PROYECTO ---
set(NEXO_SOURCES
    src/main.cpp
    src/Storage/Serializer.cpp
    src/Storage/StorageManager.cpp
    src/B-tree/IndexManager.cpp
    src/Query_engine/tokenizer.cpp
    src/Query_engine/parser.cpp
    src/Query_engine/QueryExecutor.cpp
)

set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui-sfml/imgui-SFML.cpp
)

# --- EJECUTABLE ---
add_executable(${PROJECT_NAME} ${NEXO_SOURCES} ${IMGUI_SOURCES})

# --- LINKING ---
target_link_libraries(${PROJECT_NAME}
    sfml-graphics
    sfml-window
    sfml-system
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
    
    # Automatizar copia de DLLs de SFML al compilar
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/libs/SFML-2.6.1/bin"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copiando binarios de SFML a la carpeta build..."
    )
    
    message(STATUS "[DEPLOY] Las DLLs de compilador deben estar en: ${CMAKE_BINARY_DIR}")
endif()

message(STATUS "[OK] Configuración finalizada para ${PROJECT_NAME}")