# Prepara el paquete de macOS (el objetivo «paquete» de CMakeLists.txt): copia la aplicación, le añade las bibliotecas
# de Qt con macdeployqt, la firma «ad hoc» (no es la firma de Apple, pero los Mac con procesador Apple no abren un
# programa sin ninguna) y la mete en un .dmg con el LEEME y un enlace a Aplicaciones.
# Recibe APP, MACDEPLOYQT, TRADUCCIONES_QT, LEEME, PAQUETE y DMG.

function(ejecutar)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE resultado)
    if(NOT resultado EQUAL 0)
        message(FATAL_ERROR "Ha fallado: ${ARGN}")
    endif()
endfunction()

get_filename_component(nombre "${APP}" NAME)
set(copia "${PAQUETE}/${nombre}")
file(REMOVE_RECURSE "${PAQUETE}" "${DMG}")
file(MAKE_DIRECTORY "${PAQUETE}")
file(COPY "${APP}" DESTINATION "${PAQUETE}")

ejecutar("${MACDEPLOYQT}" "${copia}")

# De los controladores de bases de datos, solo el de SQLite: los demás necesitan bibliotecas que no van en el paquete.
file(GLOB controladores "${copia}/Contents/PlugIns/sqldrivers/*")
foreach(f IN LISTS controladores)
    if(NOT f MATCHES "libqsqlite")
        file(REMOVE "${f}")
    endif()
endforeach()

# Las traducciones al castellano de los diálogos de Qt (macdeployqt no las copia).
file(GLOB traducciones "${TRADUCCIONES_QT}/qtbase_es.qm" "${TRADUCCIONES_QT}/qt_es.qm")
file(COPY ${traducciones} DESTINATION "${copia}/Contents/Resources/translations")
file(APPEND "${copia}/Contents/Resources/qt.conf" "\nTranslations = Resources/translations\n")

# Después de cambiar la aplicación: cualquier cambio posterior invalidaría la firma.
ejecutar(codesign --force --deep --sign - "${copia}")
ejecutar(codesign --verify --deep --strict "${copia}")

configure_file("${LEEME}" "${PAQUETE}/LEEME.txt" COPYONLY)
file(CREATE_LINK /Applications "${PAQUETE}/Applications" SYMBOLIC)

# hdiutil falla a veces con «Resource busy» en las máquinas de GitHub: se reintenta.
foreach(intento RANGE 1 3)
    execute_process(COMMAND hdiutil create -volname GeneradorExamenes -srcfolder "${PAQUETE}" -ov -format UDZO "${DMG}"
                    RESULT_VARIABLE resultado)
    if(resultado EQUAL 0)
        return()
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 5)
endforeach()
message(FATAL_ERROR "No se ha podido crear ${DMG}")
