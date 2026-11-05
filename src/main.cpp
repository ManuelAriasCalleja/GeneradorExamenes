#include "utilidades.h"
#include "ventanaprincipal.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("UNED"));
    QApplication::setApplicationName(QStringLiteral("GeneradorExamenes"));
    QApplication::setApplicationDisplayName(QStringLiteral("Generador de exámenes"));
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
    QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));

    QIcon icono;
    for (int lado : {16, 24, 32, 48, 64, 128, 256})
        icono.addFile(QStringLiteral(":/recursos/icono/icono_%1.png").arg(lado), QSize(lado, lado));
    QApplication::setWindowIcon(icono);

    // Diálogos estándar de Qt en castellano, si están instaladas las traducciones: junto al programa en el paquete
    // de Windows (windeployqt puede juntarlas en qt_es.qm) o donde estén las de Qt.
    QTranslator traductor;
    for (const QString &carpeta : {QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
                                   QLibraryInfo::path(QLibraryInfo::TranslationsPath)}) {
        if (traductor.load(QLocale(QLocale::Spanish), QStringLiteral("qtbase"), QStringLiteral("_"), carpeta)
            || traductor.load(QLocale(QLocale::Spanish), QStringLiteral("qt"), QStringLiteral("_"), carpeta)) {
            app.installTranslator(&traductor);
            break;
        }
    }

    QCommandLineParser opciones;
    opciones.setApplicationDescription(QStringLiteral("Generador de exámenes tipo test"));
    opciones.addHelpOption();
    opciones.addPositionalArgument(QStringLiteral("basedatos"), QStringLiteral("Fichero SQLite de preguntas (se crea si no existe)"));
    opciones.process(app);

    VentanaPrincipal ventana;
    ventana.show();
    if (!ventana.abrirBancoInicial(opciones.positionalArguments().value(0)))
        return 0;   // no se ha elegido ningún banco de preguntas
    return app.exec();
}
