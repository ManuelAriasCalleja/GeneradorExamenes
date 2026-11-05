#include "comprobadorjson.h"
#include "idioma.h"
#include "utilidades.h"
#include "ventanaprincipal.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QSettings>
#include <cstdio>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

// GeneradorExamenes --comprobar fichero.json…: comprueba los JSON sin abrir ninguna ventana y escribe los problemas
// como los compiladores («fichero:línea:columna: error: …»). Sale con 1 si alguno tiene errores.
int comprobarJson(const QStringList &ficheros)
{
#ifdef Q_OS_WIN
    // En Windows el programa es una aplicación gráfica, sin consola: escribe en la de quien lo ha lanzado.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        std::freopen("CONOUT$", "w", stdout);
        std::freopen("CONOUT$", "w", stderr);
        SetConsoleOutputCP(CP_UTF8);
    }
#endif
    if (ficheros.isEmpty()) {
        std::fputs(qUtf8Printable(QCoreApplication::translate("main", "Indique el JSON que quiere comprobar: GeneradorExamenes --comprobar preguntas.json\n")),
                   stderr);
        return 2;
    }
    int salida = 0;
    for (const QString &ruta : ficheros) {
        const ComprobadorJson::Resultado r = ComprobadorJson::comprobarFichero(ruta);
        std::fputs(qUtf8Printable(ComprobadorJson::informe(r, ruta)), stdout);
        if (!r.valido())
            salida = 1;
    }
    std::fflush(stdout);
    return salida;
}

} // namespace

int main(int argc, char *argv[])
{
    // Para comprobar un JSON no hacen falta ventanas (ni pantalla).
    bool soloComprobar = false;
    for (int i = 1; i < argc; ++i)
        soloComprobar = soloComprobar || qstrcmp(argv[i], "--comprobar") == 0;
    std::unique_ptr<QCoreApplication> app(soloComprobar ? new QCoreApplication(argc, argv) : new QApplication(argc, argv));
    QCoreApplication::setOrganizationName(QStringLiteral("UNED"));
    QCoreApplication::setApplicationName(QStringLiteral("GeneradorExamenes"));
#ifdef Q_OS_WIN
    // La configuración (último banco de preguntas abierto y opciones) va en un fichero también en Windows
    // (%APPDATA%\UNED\GeneradorExamenes.ini), no en el registro. En Linux ya es un fichero:
    // ~/.config/UNED/GeneradorExamenes.conf.
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif
    // Hasta la versión 1.0 el programa se llamaba GeneradorExamenesFI y su configuración estaba en
    // ~/.config/UNED-FI/GeneradorExamenesFI.conf (%APPDATA%\UNED-FI\GeneradorExamenesFI.ini en Windows): si la de
    // ahora está vacía, se copia de allí.
    {
        QSettings nueva;
        traerConfiguracionAntigua(QSettings(QStringLiteral("UNED-FI"), QStringLiteral("GeneradorExamenesFI")), nueva);
    }
    // Idioma de la interfaz (Editar → Idioma; si no se ha elegido, el del sistema) y diálogos estándar de Qt en él.
    Idioma::instalar(Idioma::interfaz());

    QCommandLineParser opciones;
    opciones.setApplicationDescription(QCoreApplication::translate("main", "Generador de exámenes tipo test"));
    opciones.addHelpOption();
    opciones.addOption({QStringLiteral("comprobar"),
                        QCoreApplication::translate("main", "Comprueba los JSON de preguntas indicados, sin abrir el programa: "
                                                            "escribe sus errores y avisos, con su línea, y sale con 1 si hay errores.")});
    opciones.addPositionalArgument(QStringLiteral("basedatos"),
                                   QCoreApplication::translate("main", "Fichero SQLite de preguntas (se crea si no existe); con "
                                                                       "--comprobar, los JSON que se comprueban"));
    opciones.process(*app);
    if (opciones.isSet(QStringLiteral("comprobar")))
        return comprobarJson(opciones.positionalArguments());

    QApplication::setApplicationDisplayName(QCoreApplication::translate("main", "Generador de exámenes"));
    QIcon icono;
    for (int lado : {16, 24, 32, 48, 64, 128, 256})
        icono.addFile(QStringLiteral(":/recursos/icono/icono_%1.png").arg(lado), QSize(lado, lado));
    QApplication::setWindowIcon(icono);

    VentanaPrincipal ventana;
    ventana.show();
    if (!ventana.abrirBancoInicial(opciones.positionalArguments().value(0)))
        return 0;   // no se ha elegido ningún banco de preguntas
    return app->exec();
}
