#pragma once

#include <QDialog>
#include <QHash>
#include <QString>

class BaseDatos;
class QListWidget;
class QPushButton;

// La lista de los verificadores del banco: añadirlos, cambiarles el nombre y quitarlos (los cambios se guardan al
// momento).
class DialogoVerificadores : public QDialog
{
    Q_OBJECT
public:
    explicit DialogoVerificadores(BaseDatos *bd, QWidget *parent = nullptr);

    bool hayCambios() const { return m_cambios; }
    // Los nombres cambiados: antes -> después.
    QHash<QString, QString> renombrados() const { return m_renombrados; }

private:
    void actualizar(const QString &seleccionar = QString());
    QString seleccionado() const;
    void anadir();
    void cambiarNombre();
    void quitar();

    BaseDatos *m_bd;
    QListWidget *m_lista;
    QPushButton *m_cambiarNombre;
    QPushButton *m_quitar;
    bool m_cambios = false;
    QHash<QString, QString> m_renombrados;
};
