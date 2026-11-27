#pragma once

#include <QDialog>
#include <QStringList>
#include <QVector>
#include <array>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Exportación de los exámenes generados, de una vez: en uno o varios formatos (PDF, Word, HTML, LaTeX), sus
// enunciados y sus soluciones (cada cosa en su carpeta, que se recuerda en cada banco) y los exámenes que se marquen,
// cada uno en su fichero (con el nombre que se quiera) y, si hay varios, también todos juntos en uno. Quien llama
// escribe los ficheros que dice ficheros().
class DialogoExportar : public QDialog
{
    Q_OBJECT
public:
    enum Formato { Pdf, Word, Html, Latex };   // en el orden del diálogo
    static constexpr int kNumFormatos = 4;
    // Sin punto: «pdf», «docx», «html», «tex».
    static QString extension(Formato formato);

    struct Fichero
    {
        QVector<int> modelos;   // índices de los modelos que contiene
        Formato formato = Pdf;
        bool soluciones = false;
        QString ruta;           // completa, con extensión
    };

    // 'nombres' son los de los exámenes (Nacional Original...) y 'nombreBase' el principio propuesto para el nombre de
    // sus ficheros: «<nombreBase>-<examen>», y el de todos juntos, «<nombreBase>». Los de las soluciones se llaman igual
    // con «-soluciones» al final, así que no se pisan aunque vayan a la misma carpeta.
    DialogoExportar(const QStringList &nombres, const QString &nombreBase, QWidget *parent = nullptr);

    // Lo que se escribe: de cada examen marcado (o de todos juntos), sus enunciados y sus soluciones, lo que esté
    // marcado, en cada formato marcado. Por examen y, de cada uno, como en el diálogo.
    QVector<Fichero> ficheros() const;

public slots:
    void accept() override;

private:
    QString carpeta(bool soluciones) const;
    // El nombre de la fila, como se ha escrito, sin la extensión si se ha puesto la de alguno de los formatos.
    QString nombre(int fila) const;
    void elegirCarpeta(bool soluciones);
    // Las carpetas de lo que no se exporta, sin poder tocarlas, y cuántos ficheros salen.
    void actualizar();

    std::array<QCheckBox *, kNumFormatos> m_formatos{};
    QCheckBox *m_enunciados;
    QCheckBox *m_soluciones;
    QLineEdit *m_carpetaEnunciados;
    QLineEdit *m_carpetaSoluciones;
    QPushButton *m_elegirEnunciados;
    QPushButton *m_elegirSoluciones;
    QTableWidget *m_tabla;
    QLabel *m_resumen;
    int m_numModelos;
};
