cmake_minimum_required(VERSION 3.24)
project(NexoEngine
  VERSION 1.0.0
  DESCRIPTION "NEXO: a non-relational DBMS engine with a Workspace UI"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "=== NEXO ENGINE v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE}, ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}) ===")

option(NEXO_BUILD_GUI   "Build the NexoEngine graphical client" ON)
option(NEXO_BUILD_TESTS "Build the nexo_tests suite"            ON)

# ---------------------------------------------------------------------------
# nexo_core — the database engine. Pure C++17, no GUI dependencies, testable
# headless on any platform.
# ---------------------------------------------------------------------------
add_library(nexo_core STATIC
  src/Storage/HeapFile.cpp
  src/Storage/Serializer.cpp
  src/Storage/StorageManager.cpp
  src/B-tree/IndexManager.cpp
  src/Query_engine/tokenizer.cpp
  src/Query_engine/parser.cpp
  src/Query_engine/QueryExecutor.cpp
)
target_include_directories(nexo_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# NexoEngine — the Workspace UI (SFML 3 + Dear ImGui + ImGui-SFML)
# All dependencies are pinned by tag and pulled with FetchContent, so a fresh
# clone builds with no manual setup beyond having a compiler + CMake.
# ---------------------------------------------------------------------------
if(NEXO_BUILD_GUI)
  include(FetchContent)

  # Engine core is enough for the GUI client: skip audio/network modules.
  set(SFML_BUILD_AUDIO   OFF)
  set(SFML_BUILD_NETWORK OFF)
  set(BUILD_SHARED_LIBS  OFF)

  FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
  )
  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
  )
  # ImGui-SFML expects IMGUI_DIR to be set before its CMakeLists runs.
  FetchContent_MakeAvailable(SFML imgui)

  set(IMGUI_DIR             ${imgui_SOURCE_DIR})
  set(IMGUI_SFML_FIND_SFML  OFF)
  FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v3.0
  )
  FetchContent_MakeAvailable(ImGui-SFML)

  if(WIN32)
    enable_language(RC)
    # Absolute path so windres resolves the icon regardless of build directory.
    set(NEXO_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/nexo.ico")
    configure_file(resources.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resources.rc @ONLY)
  endif()

  add_executable(NexoEngine
    src/main.cpp
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_BINARY_DIR}/resources.rc>
  )
  target_link_libraries(NexoEngine PRIVATE nexo_core ImGui-SFML::ImGui-SFML)

  if(WIN32)
    # Static-link the GCC runtime so the .exe is self-contained (no DLL hunt).
    target_link_options(NexoEngine PRIVATE -static -static-libgcc -static-libstdc++)
  endif()
endif()

# ---------------------------------------------------------------------------
# nexo_tests — headless unit tests for the engine core.
# ---------------------------------------------------------------------------
if(NEXO_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  include(FetchContent)
  FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.12
  )
  FetchContent_MakeAvailable(doctest)

  enable_testing()
  add_executable(nexo_tests
    tests/test_storage.cpp
    tests/test_btree.cpp
    tests/test_query.cpp
  )
  target_link_libraries(nexo_tests PRIVATE nexo_core doctest::doctest)
  add_test(NAME nexo_core COMMAND nexo_tests)
endif()
