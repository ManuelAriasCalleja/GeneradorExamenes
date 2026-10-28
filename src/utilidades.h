#pragma once

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
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
