#include "idioma.h"

#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>
#include <memory>

namespace Idioma {

namespace {

const char *const kClave = "interfaz/idioma";

QString rutaTraduccion(const QString &codigo)
{
    return QStringLiteral(":/traducciones/GeneradorExamenes_%1.qm").arg(codigo);
}

QLocale localeDe(const QString &codigo)
{
    return codigo == QLatin1String(kOrigen) ? QLocale(QLocale::Spanish, QLocale::Spain) : QLocale(codigo);
}

// Los traductores instalados (del programa y de Qt), que tienen que vivir mientras se usen.
std::unique_ptr<QTranslator> &traductorPrograma()
{
    static std::unique_ptr<QTranslator> t;
    return t;
}

std::unique_ptr<QTranslator> &traductorQt()
{
    static std::unique_ptr<QTranslator> t;
    return t;
}

} // namespace

QVector<Disponible> disponibles()
{
    return {{QStringLiteral("es"), QStringLiteral("Español")}, {QStringLiteral("en"), QStringLiteral("English")}};
}

QString nombre(const QString &codigo)
{
    for (const Disponible &d : disponibles())
        if (d.codigo == codigo)
            return d.nombre;
    return codigo;
}

bool disponible(const QString &codigo)
{
    for (const Disponible &d : disponibles())
        if (d.codigo == codigo)
            return true;
    return false;
}

QString elegido()
{
    return QSettings().value(QLatin1String(kClave)).toString();
}

void elegir(const QString &codigo)
{
    if (codigo.isEmpty())
        QSettings().remove(QLatin1String(kClave));
    else
        QSettings().setValue(QLatin1String(kClave), codigo);
}

QString delSistema()
{
    for (const QString &l : QLocale::system().uiLanguages()) {
        // «es-ES», «en-GB», «en»…: vale el idioma, sin el país.
        const QString codigo = l.section(QLatin1Char('-'), 0, 0).section(QLatin1Char('_'), 0, 0).toLower();
        if (disponible(codigo))
            return codigo;
    }
    return QLatin1String(kOrigen);
}

QString interfaz()
{
    const QString e = elegido();
    return disponible(e) ? e : delSistema();
}

void instalar(const QString &codigo)
{
    for (auto *t : {&traductorPrograma(), &traductorQt()}) {
        if (*t)
            QCoreApplication::removeTranslator(t->get());
        t->reset();
    }
    QLocale::setDefault(localeDe(codigo));

    if (codigo != QLatin1String(kOrigen)) {
        auto t = std::make_unique<QTranslator>();
        if (t->load(rutaTraduccion(codigo)) && QCoreApplication::installTranslator(t.get()))
            traductorPrograma() = std::move(t);
    }
    // Las de Qt (botones de los diálogos estándar, menús contextuales): junto al programa en el paquete de Windows
    // (windeployqt puede juntarlas en qt_<idioma>.qm) o donde estén las de Qt. Los textos de Qt están en inglés.
    if (codigo != QLatin1String("en")) {
        auto t = std::make_unique<QTranslator>();
        for (const QString &carpeta : {QCoreApplication::applicationDirPath() + QStringLiteral("/translations"),
                                       QLibraryInfo::path(QLibraryInfo::TranslationsPath)}) {
            if (t->load(localeDe(codigo), QStringLiteral("qtbase"), QStringLiteral("_"), carpeta)
                || t->load(localeDe(codigo), QStringLiteral("qt"), QStringLiteral("_"), carpeta)) {
                if (QCoreApplication::installTranslator(t.get()))
                    traductorQt() = std::move(t);
                break;
            }
        }
    }
}

QString examen(const QString &idioma, const char *texto)
{
    if (idioma.isEmpty() || idioma == QLatin1String(kOrigen))
        return QString::fromUtf8(texto);
    // Un traductor por idioma, sin instalar: no cambia el de la interfaz.
    static QHash<QString, std::shared_ptr<QTranslator>> traductores;
    std::shared_ptr<QTranslator> &t = traductores[idioma];
    if (!t) {
        t = std::make_shared<QTranslator>();
        if (!t->load(rutaTraduccion(idioma)))
            t.reset(new QTranslator);   // sin traducción: los textos originales
    }
    const QString traducido = t->translate("Examen", texto);
    return traducido.isEmpty() ? QString::fromUtf8(texto) : traducido;
}

QString ayuda(const QString &codigo)
{
    const QString propia = QStringLiteral(":/recursos/ayuda_%1.html").arg(codigo);
    if (codigo != QLatin1String(kOrigen) && QFile::exists(propia))
        return QStringLiteral("qrc") + propia;
    return QStringLiteral("qrc:/recursos/ayuda.html");
}

} // namespace Idioma
