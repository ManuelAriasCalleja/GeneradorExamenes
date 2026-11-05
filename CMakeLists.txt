cmake_minimum_required(VERSION 3.19)

project(GeneradorExamenes VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sin tipo de compilación indicado, optimizada (CMake no optimiza por defecto).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilación" FORCE)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Sql PrintSupport)
find_package(Qt6 6.2 COMPONENTS Test)   # solo para las pruebas automáticas
find_package(Qt6 6.2 COMPONENTS Pdf)    # opcional: leer los PDF de los exámenes anteriores
qt_standard_project_setup()
include(GNUInstallDirs)

# Todo el programa salvo main.cpp, en una biblioteca que comparten el ejecutable y las pruebas.
qt_add_library(nucleo STATIC
    src/pregunta.h src/pregunta.cpp
    src/utilidades.h
    src/idioma.h src/idioma.cpp
    src/comprobadorjson.h src/comprobadorjson.cpp
    src/dialogocomprobacion.h src/dialogocomprobacion.cpp
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
    src/moodle.h src/moodle.cpp
    src/dialogomoodle.h src/dialogomoodle.cpp
    src/dialogoayuda.h src/dialogoayuda.cpp
    src/examenesanteriores.h src/examenesanteriores.cpp
    src/dialogoanteriores.h src/dialogoanteriores.cpp
    src/ventanaprincipal.h src/ventanaprincipal.cpp
)
target_include_directories(nucleo PUBLIC src)
# El manual de uso (Ayuda → Manual de uso, F1), en la biblioteca para que lo tengan también las pruebas.
qt_add_resources(nucleo "ayuda"
    PREFIX "/"
    FILES recursos/ayuda.html recursos/ayuda_en.html recursos/banco.schema.json recursos/ejemplo_banco.json recursos/ejemplo_banco_en.json
)
target_link_libraries(nucleo PUBLIC Qt6::Widgets Qt6::Sql Qt6::PrintSupport)
# Con Qt PDF (6.4 o posterior), «Registrar exámenes anteriores» lee los PDF; sin él, solo el texto pegado.
if(Qt6Pdf_FOUND AND Qt6_VERSION VERSION_GREATER_EQUAL 6.4)
    target_link_libraries(nucleo PUBLIC Qt6::Pdf)
    target_compile_definitions(nucleo PRIVATE LEER_PDF)
else()
    message(STATUS "Sin Qt PDF: los exámenes anteriores no se podrán leer de sus PDF, solo pegando su texto.")
endif()
# Traducciones de la interfaz y de los textos de los exámenes (Qt Linguist; ver src/idioma.h). Los textos del código
# están en castellano; cada idioma más es un traducciones/GeneradorExamenes_<código>.ts. «cmake --build build --target
# update_translations» los pone al día con los textos nuevos, que se traducen con Qt Linguist, y al compilar se
# convierten en .qm dentro del programa (:/traducciones). Sin Qt Linguist el programa compila, pero solo en castellano.
find_package(Qt6 6.2 COMPONENTS LinguistTools)
if(Qt6LinguistTools_FOUND)
    get_target_property(fuentes_nucleo nucleo SOURCES)
    qt_add_translations(nucleo
        TS_FILES traducciones/GeneradorExamenes_en.ts
        RESOURCE_PREFIX "/traducciones"
        SOURCES ${fuentes_nucleo} src/main.cpp
        LUPDATE_OPTIONS -no-obsolete -locations relative)
else()
    message(WARNING "Sin Qt Linguist (en Debian/Ubuntu, qt6-l10n-tools y qt6-tools-dev): el programa solo estará en castellano.")
endif()
# La versión del proyecto se usa en el diálogo «Acerca de», para no tenerla escrita a mano en dos sitios.
target_compile_definitions(nucleo PRIVATE VERSION_APLICACION="${PROJECT_VERSION}")

qt_add_executable(GeneradorExamenes src/main.cpp)
target_link_libraries(GeneradorExamenes PRIVATE nucleo)

# Foto del autor para el diálogo «Acerca de» e icono de la aplicación (PNG generados con recursos/generar_iconos.sh).
qt_add_resources(GeneradorExamenes "recursos"
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
    set_target_properties(GeneradorExamenes PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(GeneradorExamenes PRIVATE recursos/icono.rc)
endif()

# Paquete para Windows: «cmake --build build --target paquete» (en Release) prepara la carpeta build/paquete/
# GeneradorExamenes con el programa y todo lo que necesita (las bibliotecas de Qt, que copia windeployqt, y las del
# compilador) y la comprime en build/GeneradorExamenes-<versión>-windows.zip. Se descomprime en cualquier ordenador
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
        set(copiar_bibliotecas_compilador COMMAND ${CMAKE_COMMAND} -E copy ${bibliotecas_compilador} paquete/GeneradorExamenes)
    else()
        message(WARNING "No se encuentran las bibliotecas del compilador: el paquete de Windows no funcionaría en otro ordenador.")
    endif()
    # Instalación (la de compilar.ps1): el programa, en la carpeta de destino sin subcarpetas, con las bibliotecas
    # de Qt que copia windeployqt y las del compilador, como en el paquete.
    install(TARGETS GeneradorExamenes RUNTIME DESTINATION .)
    if(bibliotecas_compilador)
        install(FILES ${bibliotecas_compilador} DESTINATION .)
    endif()
    if(WINDEPLOYQT)
        install(CODE "
            execute_process(COMMAND \"${WINDEPLOYQT}\" $<IF:$<CONFIG:Debug>,--debug,--release> --no-compiler-runtime
                    --no-opengl-sw --no-system-d3d-compiler --translations es
                    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/$<TARGET_FILE_NAME:GeneradorExamenes>\"
                RESULT_VARIABLE resultado)
            if(NOT resultado EQUAL 0)
                message(FATAL_ERROR \"windeployqt no ha podido copiar las bibliotecas de Qt\")
            endif()
        ")
    endif()

    set(zip GeneradorExamenes-${PROJECT_VERSION}-windows.zip)
    add_custom_target(paquete
        COMMAND ${CMAKE_COMMAND} -E rm -rf paquete ${zip}
        COMMAND ${CMAKE_COMMAND} -E make_directory paquete/GeneradorExamenes
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:GeneradorExamenes> paquete/GeneradorExamenes
        # Sin las bibliotecas de OpenGL por software ni el compilador de Direct3D, que no usa, y con las
        # traducciones al castellano de los diálogos de Qt.
        COMMAND ${WINDEPLOYQT} $<IF:$<CONFIG:Debug>,--debug,--release> --no-compiler-runtime --no-opengl-sw
                --no-system-d3d-compiler --translations es paquete/GeneradorExamenes/$<TARGET_FILE_NAME:GeneradorExamenes>
        ${copiar_bibliotecas_compilador}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/recursos/LEEME_windows.txt paquete/GeneradorExamenes/LEEME.txt
        COMMAND ${CMAKE_COMMAND} -E chdir paquete ${CMAKE_COMMAND} -E tar cf ../${zip} --format=zip GeneradorExamenes
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS GeneradorExamenes
        COMMENT "Preparando ${zip}"
        VERBATIM
    )
endif()

# Instalación en Linux (la de compilar.sh, que instala en ~/.local): el ejecutable en bin/, el lanzador del menú del
# escritorio en share/applications y el icono en el tema hicolor. El lanzador (recursos/GeneradorExamenes.desktop.in,
# que usa también herramientas/instalar_lanzador.sh) se escribe al instalar, para que apunte al ejecutable del prefijo
# de verdad (el de «cmake --install --prefix», si se indica). Su StartupWMClass es la clase de las ventanas del programa
# (su nombre de aplicación en Qt): así el panel agrupa la ventana con el lanzador.
if(UNIX AND NOT APPLE)
    install(TARGETS GeneradorExamenes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    foreach(n 16 24 32 48 64 128 256)
        install(FILES recursos/icono/icono_${n}.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${n}x${n}/apps
                RENAME generador-examenes.png)
    endforeach()
    install(FILES recursos/icono.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
            RENAME generador-examenes.svg)
    install(CODE "
        set(plantilla_lanzador \"${CMAKE_CURRENT_SOURCE_DIR}/recursos/GeneradorExamenes.desktop.in\")
        set(dir_bin \"${CMAKE_INSTALL_BINDIR}\")
        set(dir_datos \"${CMAKE_INSTALL_DATADIR}\")
    ")
    install(CODE [[
        set(RUTA_EJECUTABLE "${CMAKE_INSTALL_PREFIX}/${dir_bin}/GeneradorExamenes")
        set(lanzador "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${dir_datos}/applications/GeneradorExamenes.desktop")
        message(STATUS "Installing: ${lanzador}")
        configure_file("${plantilla_lanzador}" "${lanzador}" @ONLY)
        list(APPEND CMAKE_INSTALL_MANIFEST_FILES "${lanzador}")
    ]])
endif()

# Capturas de pantalla de la presentación (presentacion/), hechas con un banco inventado: «cmake --build build --target
# capturas» las rehace en presentacion/capturas/. No se compila con el resto; necesita Qt PDF (para las páginas de los PDF).
# La presentación no está en git (.gitignore): sin ella, no hay objetivo.
if(TARGET Qt6::Pdf AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/presentacion/capturas.cpp)
    qt_add_executable(hacer_capturas presentacion/capturas.cpp presentacion/banco_ejemplo.h)
    set_target_properties(hacer_capturas PROPERTIES EXCLUDE_FROM_ALL TRUE)
    target_include_directories(hacer_capturas PRIVATE presentacion)
    target_link_libraries(hacer_capturas PRIVATE nucleo Qt6::Pdf)
    add_custom_target(capturas
        COMMAND ${CMAKE_COMMAND} -E env QT_QPA_PLATFORM=offscreen $<TARGET_FILE:hacer_capturas>
                ${CMAKE_CURRENT_SOURCE_DIR}/presentacion/capturas
        DEPENDS hacer_capturas
        COMMENT "Capturas de la presentación"
        VERBATIM
    )
endif()

# Pruebas automáticas (ctest --test-dir build --output-on-failure). Usan un banco inventado; la de los bancos
# reales se salta si no está el enlace «asignatura» al directorio de la asignatura.
option(PRUEBAS "Compilar las pruebas automáticas" ON)
if(PRUEBAS AND Qt6Test_FOUND)
    enable_testing()
    # Sin ventanas (plataforma «offscreen»). En Windows, esa plataforma no usa las fuentes del sistema, sino las de
    # <Qt>/lib/fonts, que no existe: sin fuentes, la maqueta no cabe y los PDF salen sin texto. Se le da la carpeta
    # de las de Windows.
    set(entorno_pruebas "QT_QPA_PLATFORM=offscreen")
    if(WIN32)
        set(fuentes_windows "$ENV{SystemRoot}/Fonts")
        if(NOT DEFINED ENV{SystemRoot})
            set(fuentes_windows "C:/Windows/Fonts")
        endif()
        file(TO_CMAKE_PATH "${fuentes_windows}" fuentes_windows)
        list(APPEND entorno_pruebas "QT_QPA_FONTDIR=${fuentes_windows}")
    endif()
    foreach(prueba generador estilo enunciado basedatos maqueta parecidas moodle anteriores interfaz comprobador idiomas bancos_reales)
        qt_add_executable(prueba_${prueba} tests/prueba_${prueba}.cpp tests/datos_prueba.h)
        target_link_libraries(prueba_${prueba} PRIVATE nucleo Qt6::Test)
        add_test(NAME ${prueba} COMMAND prueba_${prueba})
        set_tests_properties(${prueba} PROPERTIES ENVIRONMENT "${entorno_pruebas}")
    endforeach()
    target_compile_definitions(prueba_bancos_reales PRIVATE RUTA_PROYECTO="${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(prueba_idiomas PRIVATE RUTA_PROYECTO="${CMAKE_CURRENT_SOURCE_DIR}")
endif()
