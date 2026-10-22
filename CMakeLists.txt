cmake_minimum_required(VERSION 3.19)

project(GeneradorExamenesFI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sin tipo de compilación indicado, optimizada (CMake no optimiza por defecto).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilación" FORCE)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Sql PrintSupport)
find_package(Qt6 6.2 COMPONENTS Test)   # solo para las pruebas automáticas
qt_standard_project_setup()

# Todo el programa salvo main.cpp, en una biblioteca que comparten el ejecutable y las pruebas.
qt_add_library(nucleo STATIC
    src/pregunta.h src/pregunta.cpp
    src/utilidades.h
    src/basedatos.h src/basedatos.cpp
    src/generador.h src/generador.cpp
    src/enunciado.h src/enunciado.cpp
    src/estilo.h src/estilo.cpp
    src/exportacion.h src/exportacion.cpp
    src/parecidas.h src/parecidas.cpp
    src/maquetador.h src/maquetador.cpp
    src/modelopreguntas.h src/modelopreguntas.cpp
    src/dialogopregunta.h src/dialogopregunta.cpp
    src/dialogotemas.h src/dialogotemas.cpp
    src/dialogopractica.h src/dialogopractica.cpp
    src/dialogoestilo.h src/dialogoestilo.cpp
    src/dialogoparecidas.h src/dialogoparecidas.cpp
    src/dialogoexportar.h src/dialogoexportar.cpp
    src/dialogohistorial.h src/dialogohistorial.cpp
    src/ventanaprincipal.h src/ventanaprincipal.cpp
)
target_include_directories(nucleo PUBLIC src)
target_link_libraries(nucleo PUBLIC Qt6::Widgets Qt6::Sql Qt6::PrintSupport)
# La versión del proyecto se usa en el diálogo «Acerca de», para no tenerla escrita a mano en dos sitios.
target_compile_definitions(nucleo PRIVATE VERSION_APLICACION="${PROJECT_VERSION}")

qt_add_executable(GeneradorExamenesFI src/main.cpp)
target_link_libraries(GeneradorExamenesFI PRIVATE nucleo)

# Foto del autor para el diálogo «Acerca de» e icono de la aplicación (PNG generados con recursos/generar_iconos.sh).
qt_add_resources(GeneradorExamenesFI "recursos"
    PREFIX "/"
    FILES
        recursos/manuel-arias-calleja.jpg
        recursos/icono/icono_16.png
        recursos/icono/icono_24.png
        recursos/icono/icono_32.png
        recursos/icono/icono_48.png
        recursos/icono/icono_64.png
        recursos/icono/icono_128.png
        recursos/icono/icono_256.png
)

# En Windows, aplicación gráfica (sin ventana de consola) y con icono en el .exe. El de las ventanas lo pone main.cpp.
if(WIN32)
    set_target_properties(GeneradorExamenesFI PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(GeneradorExamenesFI PRIVATE recursos/icono.rc)
endif()

# Paquete para Windows: «cmake --build build --target paquete» (en Release) prepara la carpeta build/paquete/
# GeneradorExamenesFI con el programa y todo lo que necesita (las bibliotecas de Qt, que copia windeployqt, y las del
# compilador) y la comprime en build/GeneradorExamenesFI-<versión>-windows.zip. Se descomprime en cualquier ordenador
# con Windows y funciona sin instalar nada.
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
    if(NOT WINDEPLOYQT)
        message(WARNING "No se encuentra windeployqt: no se podrá preparar el paquete de Windows.")
    endif()
    # Bibliotecas del compilador: con MinGW, las de su carpeta; con Visual Studio, las que da CMake.
    if(MINGW)
        get_filename_component(bin_compilador "${CMAKE_CXX_COMPILER}" DIRECTORY)
        file(GLOB bibliotecas_compilador "${bin_compilador}/libgcc_s_*.dll" "${bin_compilador}/libstdc++-6.dll"
                                         "${bin_compilador}/libwinpthread-1.dll")
    else()
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
        include(InstallRequiredSystemLibraries)
        set(bibliotecas_compilador ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
    endif()
    set(copiar_bibliotecas_compilador)
    if(bibliotecas_compilador)
        set(copiar_bibliotecas_compilador COMMAND ${CMAKE_COMMAND} -E copy ${bibliotecas_compilador} paquete/GeneradorExamenesFI)
    else()
        message(WARNING "No se encuentran las bibliotecas del compilador: el paquete de Windows no funcionaría en otro ordenador.")
    endif()
    set(zip GeneradorExamenesFI-${PROJECT_VERSION}-windows.zip)
    add_custom_target(paquete
        COMMAND ${CMAKE_COMMAND} -E rm -rf paquete ${zip}
        COMMAND ${CMAKE_COMMAND} -E make_directory paquete/GeneradorExamenesFI
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GeneradorExamenesFI> paquete/GeneradorExamenesFI
        # Sin las bibliotecas de OpenGL por software ni el compilador de Direct3D, que no usa, y con las
        # traducciones al castellano de los diálogos de Qt.
        COMMAND ${WINDEPLOYQT} $<IF:$<CONFIG:Debug>,--debug,--release> --no-compiler-runtime --no-opengl-sw
                --no-system-d3d-compiler --translations es paquete/GeneradorExamenesFI/$<TARGET_FILE_NAME:GeneradorExamenesFI>
        ${copiar_bibliotecas_compilador}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/recursos/LEEME_windows.txt paquete/GeneradorExamenesFI/LEEME.txt
        COMMAND ${CMAKE_COMMAND} -E chdir paquete ${CMAKE_COMMAND} -E tar cf ../${zip} --format=zip GeneradorExamenesFI
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS GeneradorExamenesFI
        COMMENT "Preparando ${zip}"
        VERBATIM
    )
endif()

# Pruebas automáticas (ctest --test-dir build --output-on-failure). Usan un banco inventado; la de los bancos
# reales se salta si no está el enlace «asignatura» al directorio de la asignatura.
option(PRUEBAS "Compilar las pruebas automáticas" ON)
if(PRUEBAS AND Qt6Test_FOUND)
    enable_testing()
    foreach(prueba generador estilo enunciado basedatos maqueta parecidas interfaz bancos_reales)
        qt_add_executable(prueba_${prueba} tests/prueba_${prueba}.cpp tests/datos_prueba.h)
        target_link_libraries(prueba_${prueba} PRIVATE nucleo Qt6::Test)
        add_test(NAME ${prueba} COMMAND prueba_${prueba})
        set_tests_properties(${prueba} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endforeach()
    target_compile_definitions(prueba_bancos_reales PRIVATE RUTA_PROYECTO="${CMAKE_CURRENT_SOURCE_DIR}")
endif()
