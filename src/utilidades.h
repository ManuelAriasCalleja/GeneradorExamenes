#pragma once

#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QTextDocument>

// Recuadros de la interfaz: avisos (amarillo) e información (azul).
constexpr const char *kEstiloAviso = "background: #fff3cd; color: #664d03; border: 1px solid #ffe69c; padding: 6px";
constexpr const char *kEstiloInformacion = "background: #e7f1fb; color: #0b3c6b; border: 1px solid #b6d4f2; padding: 6px";

// Pone un documento en un visor. El documento inicial lo borra el propio visor al sustituirlo; los puestos con esta
// función (creados como hijos del visor) se borran aquí al poner el siguiente.
inline void ponerDocumento(QTextBrowser *visor, QTextDocument *doc)
{
    QTextDocument *anterior = visor->document();
    const bool propio = anterior && anterior->parent() == visor;
    visor->setDocument(doc);
    if (propio)
        anterior->deleteLater();
}

// Carpeta en la que se abren los diálogos para elegir un banco de preguntas: la del banco abierto o, si no hay ninguno
// (al empezar, o si el último se ha movido), la del último banco que se abrió, o la más cercana a ella que siga
// existiendo; si no se ha abierto ninguno nunca, la de los documentos.
inline QString carpetaBancos(const QString &rutaAbierta, const QString &rutaUltimo)
{
    if (!rutaAbierta.isEmpty())
        return QFileInfo(rutaAbierta).absolutePath();
    if (!rutaUltimo.isEmpty()) {
        QString ruta = QFileInfo(rutaUltimo).absolutePath();
        while (!QFileInfo(ruta).isDir() && ruta != QFileInfo(ruta).absolutePath())
            ruta = QFileInfo(ruta).absolutePath();
        if (QFileInfo(ruta).isDir())
            return ruta;
    }
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

// Copia en 'nueva' toda la configuración de 'antigua' si 'nueva' está vacía, y devuelve si ha copiado algo. La
// antigua no se toca. Sirve para que la configuración de cuando el programa se llamaba GeneradorExamenesFI (hasta la
// versión 1.0) pase a la de ahora la primera vez que se abre.
inline bool traerConfiguracionAntigua(const QSettings &antigua, QSettings &nueva)
{
    if (!nueva.allKeys().isEmpty())
        return false;
    const QStringList claves = antigua.allKeys();
    for (const QString &clave : claves)
        nueva.setValue(clave, antigua.value(clave));
    nueva.sync();
    return !claves.isEmpty();
}

// Nombre de fichero sin tildes, espacios ni símbolos: «2026 Junio», «Unión Europea» → «2026_Junio», «Union_Europea».
inline QString limpiarNombreFichero(QString s)
{
    s = s.normalized(QString::NormalizationForm_D);
    s.remove(QRegularExpression(QStringLiteral("\\p{Mn}")));
    s.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_\\-]+")), QStringLiteral("_"));
    s.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    return s.isEmpty() ? QStringLiteral("examen") : s;
}

// El texto en minúsculas y sin tildes, para buscar sin distinguirlas: «Exámenes» → «examenes». Cada letra se cambia
// por otra sola, así que lo encontrado está en las mismas posiciones que en el original.
inline QString paraBuscar(QString texto)
{
    for (QChar &c : texto) {
        if (c.decompositionTag() == QChar::Canonical)
            c = c.decomposition().at(0);
        c = c.toLower();
    }
    return texto;
}

// Pregunta Sí/No con los botones en castellano (aunque no estén instaladas las traducciones de Qt).
inline bool confirmar(QWidget *padre, const QString &titulo, const QString &texto)
{
    QMessageBox caja(QMessageBox::Question, titulo, texto, QMessageBox::NoButton, padre);
    QPushButton *si = caja.addButton(QStringLiteral("Sí"), QMessageBox::YesRole);
    caja.addButton(QStringLiteral("No"), QMessageBox::NoRole);
    caja.exec();
    return caja.clickedButton() == si;
}

// Pide una línea de texto (en 'texto', el que se propone), con los botones en castellano. Devuelve false si se cancela.
inline bool pedirTexto(QWidget *padre, const QString &titulo, const QString &etiqueta, QString *texto)
{
    QInputDialog dialogo(padre);
    dialogo.setWindowTitle(titulo);
    dialogo.setLabelText(etiqueta);
    dialogo.setTextValue(*texto);
    dialogo.setOkButtonText(QStringLiteral("Aceptar"));
    dialogo.setCancelButtonText(QStringLiteral("Cancelar"));
    if (dialogo.exec() != QDialog::Accepted)
        return false;
    *texto = dialogo.textValue().trimmed();
    return true;
}
