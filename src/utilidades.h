#pragma once

#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>

// Nombre de fichero sin tildes, espacios ni símbolos: «2026 Junio», «Unión Europea» → «2026_Junio», «Union_Europea».
inline QString limpiarNombreFichero(QString s)
{
    s = s.normalized(QString::NormalizationForm_D);
    s.remove(QRegularExpression(QStringLiteral("\\p{Mn}")));
    s.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_\\-]+")), QStringLiteral("_"));
    s.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    return s.isEmpty() ? QStringLiteral("examen") : s;
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
