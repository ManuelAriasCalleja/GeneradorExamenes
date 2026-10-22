cmake_minimum_required(VERSION 3.19)

project(GeneradorExamenesFI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Sql PrintSupport)
qt_standard_project_setup()

qt_add_executable(GeneradorExamenesFI
    src/main.cpp
    src/pregunta.h
    src/utilidades.h
    src/basedatos.h src/basedatos.cpp
    src/generador.h src/generador.cpp
    src/maquetador.h src/maquetador.cpp
    src/modelopreguntas.h src/modelopreguntas.cpp
    src/dialogopregunta.h src/dialogopregunta.cpp
    src/dialogotemas.h src/dialogotemas.cpp
    src/dialogopractica.h src/dialogopractica.cpp
    src/dialogoexportar.h src/dialogoexportar.cpp
    src/dialogohistorial.h src/dialogohistorial.cpp
    src/ventanaprincipal.h src/ventanaprincipal.cpp
)

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

# La versión del proyecto se usa en el diálogo «Acerca de», para no tenerla escrita a mano en dos sitios.
target_compile_definitions(GeneradorExamenesFI PRIVATE
    VERSION_APLICACION="${PROJECT_VERSION}"
)

target_link_libraries(GeneradorExamenesFI PRIVATE Qt6::Widgets Qt6::Sql Qt6::PrintSupport)

# En Windows, aplicación gráfica (sin ventana de consola) y con icono en el .exe. El de las ventanas lo pone main.cpp.
if(WIN32)
    set_target_properties(GeneradorExamenesFI PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(GeneradorExamenesFI PRIVATE recursos/icono.rc)
endif()
