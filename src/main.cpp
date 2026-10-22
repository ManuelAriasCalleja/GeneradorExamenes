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
    QApplication::setOrganizationName(QStringLiteral("UNED-FI"));
    QApplication::setApplicationName(QStringLiteral("GeneradorExamenesFI"));
    QApplication::setApplicationDisplayName(QStringLiteral("Generador de exámenes FI"));
#ifdef Q_OS_WIN
    // La configuración (último banco de preguntas abierto y opciones) va en un fichero también en Windows
    // (%APPDATA%\UNED-FI\GeneradorExamenesFI.ini), no en el registro. En Linux ya es un fichero:
    // ~/.config/UNED-FI/GeneradorExamenesFI.conf.
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif
    QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));

    QIcon icono;
    for (int lado : {16, 24, 32, 48, 64, 128, 256})
        icono.addFile(QStringLiteral(":/recursos/icono/icono_%1.png").arg(lado), QSize(lado, lado));
    QApplication::setWindowIcon(icono);

    // Diálogos estándar de Qt en castellano, si están instaladas las traducciones.
    QTranslator traductor;
    if (traductor.load(QLocale(QLocale::Spanish), QStringLiteral("qtbase"), QStringLiteral("_"),
                       QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&traductor);

    QCommandLineParser opciones;
    opciones.setApplicationDescription(QStringLiteral("Generador de exámenes tipo test de Fundamentos de Informática"));
    opciones.addHelpOption();
    opciones.addPositionalArgument(QStringLiteral("basedatos"), QStringLiteral("Fichero SQLite de preguntas (se crea si no existe)"));
    opciones.process(app);

    VentanaPrincipal ventana;
    ventana.show();
    if (!ventana.abrirBancoInicial(opciones.positionalArguments().value(0)))
        return 0;   // no se ha elegido ningún banco de preguntas
    return app.exec();
}
