# Escribe SALIDA (un .cpp) con la fecha y la hora de ahora y el commit de git del código de FUENTES, para el diálogo
# «Acerca de». Lo ejecuta la compilación (ver CMakeLists.txt) cada vez que se ha recompilado algo del programa.
#   cmake -DSALIDA=<fichero.cpp> -DFUENTES=<carpeta del proyecto> [-DGIT=<ejecutable de git>] -P compilacion.cmake
string(TIMESTAMP fecha "%Y-%m-%dT%H:%M:%S")

# Sin git, o fuera de un repositorio (el código descargado en un .zip), no hay commit.
set(commit "")
if(GIT)
    execute_process(COMMAND "${GIT}" rev-parse --short HEAD
        WORKING_DIRECTORY "${FUENTES}" OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE resultado)
    if(NOT resultado EQUAL 0 OR NOT commit MATCHES "^[0-9a-f]+$")
        set(commit "")
    endif()
endif()

file(WRITE "${SALIDA}" "// Generado por cmake/compilacion.cmake al compilar: no editar.
extern const char *const kFechaCompilacion;
extern const char *const kCommitCompilacion;
const char *const kFechaCompilacion = \"${fecha}\";
const char *const kCommitCompilacion = \"${commit}\";
")
