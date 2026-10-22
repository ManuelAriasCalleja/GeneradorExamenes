// Recorridos por la interfaz: el editor de preguntas, la práctica y la ventana principal (generar y exportar).

#include "basedatos.h"
#include "datos_prueba.h"
#include "dialogoparecidas.h"
#include "dialogopractica.h"
#include "dialogopregunta.h"
#include "generador.h"
#include "ventanaprincipal.h"

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <QtTest>

namespace {

// Al abrirse el siguiente cuadro modal, pulsa su botón que empieza por 'boton' y devuelve el texto del cuadro en
// 'texto'. Si 'despues' no está vacío, en el cuadro que se abra a continuación pulsa ese otro botón.
void responder(const QString &boton, QString *texto = nullptr, const QString &despues = QString())
{
    QTimer::singleShot(100, [=] {
        QWidget *w = QApplication::activeModalWidget();
        if (!w)
            return;
        if (auto *mb = qobject_cast<QMessageBox *>(w); mb && texto)
            *texto = mb->text();
        for (QAbstractButton *b : w->findChildren<QAbstractButton *>())
            if (b->text().startsWith(boton) && b->isEnabled()) {
                b->click();
                break;
            }
        if (!despues.isEmpty())
            responder(despues, texto);
    });
}

Pregunta nuevaEn(int tema, int numero)
{
    Pregunta p;
    p.temaId = tema;
    p.temaNumero = tema;
    p.numero = numero;
    return p;
}

} // namespace

class PruebaInterfaz : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // La configuración de las pruebas, aparte de la del programa.
        QStandardPaths::setTestModeEnabled(true);
        QApplication::setOrganizationName(QStringLiteral("UNED-FI-pruebas"));
        QApplication::setApplicationName(QStringLiteral("GeneradorExamenesFI-pruebas"));
        QSettings().clear();
    }

    void cleanupTestCase()
    {
        QSettings().clear();
    }

    void editorNumeraAlCambiarDeTema()
    {
        QVector<Pregunta> banco = bancoInventado(5, 20);
        banco.erase(std::remove_if(banco.begin(), banco.end(), [](const Pregunta &p) { return p.temaNumero == 5 && p.numero > 12; }),
                    banco.end());
        DialogoPregunta d(temasInventados(5), banco, nuevaEn(1, 21));
        d.findChild<QComboBox *>()->setCurrentIndex(4);
        QCOMPARE(d.findChild<QSpinBox *>()->value(), 13);

        // Una variante nueva («Duplicar como variante») conserva su número.
        Pregunta variante = banco.first();
        variante.id = 0;
        variante.variante = QStringLiteral("b");
        DialogoPregunta dv(temasInventados(5), banco, variante);
        dv.findChild<QComboBox *>()->setCurrentIndex(4);
        QCOMPARE(dv.findChild<QSpinBox *>()->value(), banco.first().numero);
    }

    void editorNoGuardaSinEnunciado()
    {
        DialogoPregunta d(temasInventados(2), bancoInventado(2, 5), nuevaEn(1, 6));
        d.show();
        QString texto;
        responder(QStringLiteral("OK"), &texto);
        d.accept();
        QVERIFY(d.result() != QDialog::Accepted);
        QCOMPARE(texto, QStringLiteral("El enunciado está vacío."));
    }

    void editorAvisaDeLasPautas()
    {
        Pregunta p = bancoVariado().at(2);   // T1-3
        p.id = 0;
        p.numero = 9;
        p.enunciado = QStringLiteral("¿Qué mide un contador de programa inventado?");   // sin los dos puntos
        p.opciones = {QStringLiteral("Una cosa."), QStringLiteral("Otra cosa."), QStringLiteral("Nada."), QStringLiteral("Todo.")};
        {
            DialogoPregunta d(temasInventados(4), bancoVariado(), p);
            d.show();
            QString texto;
            responder(QStringLiteral("Seguir editando"), &texto);
            d.accept();
            QVERIFY(d.result() != QDialog::Accepted);
            QVERIFY(texto.contains(QStringLiteral("no sigue algunas pautas de estilo")));
        }
        {
            DialogoPregunta d(temasInventados(4), bancoVariado(), p);
            d.show();
            responder(QStringLiteral("Guardar así"));
            d.accept();
            QCOMPARE(d.result(), int(QDialog::Accepted));
            QCOMPARE(d.pregunta().enunciado, p.enunciado);
        }
    }

    void editorGuardaComoVariante()
    {
        QVector<Pregunta> banco = bancoVariado();
        banco[0].pseudoCon = QStringLiteral("T3-2");   // la T1-1, enlazada con otra
        Pregunta nueva = banco.first();
        nueva.id = 0;
        nueva.numero = 9;
        nueva.pseudoCon.clear();
        nueva.enunciado = QStringLiteral("La memoria caché de un procesador sirve para:");
        DialogoPregunta d(temasInventados(4), banco, nueva);
        d.show();
        responder(QStringLiteral("Guardar como variante"));
        d.accept();
        QCOMPARE(d.result(), int(QDialog::Accepted));
        QCOMPARE(d.pregunta().codigoCompleto(), QStringLiteral("T1-1b"));
        QCOMPARE(d.pregunta().pseudoCon, QStringLiteral("T3-2"));   // hereda el enlace de su grupo
    }

    void editorSeguirEditandoNoGuarda()
    {
        Pregunta nueva = bancoVariado().first();
        nueva.id = 0;
        nueva.numero = 9;
        DialogoPregunta d(temasInventados(4), bancoVariado(), nueva);
        d.show();
        responder(QStringLiteral("Seguir editando"));
        d.accept();
        QVERIFY(d.result() != QDialog::Accepted);
    }

    void practicaCorrige()
    {
        ConfigExamen c;
        c.semilla = 11;
        c.nombresModelos = {QStringLiteral("Práctica")};
        c.preguntasPorTema = {{1, 3}, {2, 2}, {3, 3}, {4, 2}};
        c.barajarOpciones = true;
        const Modelo m = Generador::generar(bancoInventado(), c).modelos.first();
        DialogoPractica p(m, DatosConvocatoria());   // +1 por acierto, −0,3 por fallo
        p.show();
        // Bien las cuatro primeras, mal la quinta; la sexta se marca y se deja después en blanco.
        const QList<QButtonGroup *> grupos = p.findChildren<QButtonGroup *>();
        QCOMPARE(grupos.size(), 10);
        for (int i = 0; i < 6; ++i) {
            const int correcta = m.preguntas.at(i).correcta();
            grupos.at(i)->button(i == 4 ? (correcta + 1) % 4 : correcta)->setChecked(true);
        }
        p.findChildren<QToolButton *>().at(5)->click();
        QString texto;
        responder(QStringLiteral("OK"), &texto);
        QMetaObject::invokeMethod(&p, "corregir");
        QVERIFY2(texto.contains(QStringLiteral("Aciertos: 4")) && texto.contains(QStringLiteral("Fallos: 1"))
                     && texto.contains(QStringLiteral("En blanco: 5")),
                 qPrintable(texto));
        QString nota;
        for (QLabel *l : p.findChildren<QLabel *>())
            if (l->text().startsWith(QStringLiteral("Nota:")))
                nota = l->text();
        QCOMPARE(nota, QStringLiteral("Nota: 3,70 / 10,00"));
    }

    void ventanaGeneraYExporta()
    {
        QTemporaryDir dir;
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
            QVERIFY(llenarBaseDatos(bd, 8, bancoInventado()));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QMetaObject::invokeMethod(&v, "generar");

        const QString carpeta = dir.filePath(QStringLiteral("exportados"));
        QDir().mkpath(carpeta);
        for (const char *slot : {"exportarPdf", "exportarSolucionesPdf", "exportarHtml", "exportarLatex"}) {
            QTimer::singleShot(100, [&carpeta] {
                QWidget *d = QApplication::activeModalWidget();
                if (!d)
                    return;
                d->findChild<QLineEdit *>()->setText(carpeta);
                for (QPushButton *b : d->findChildren<QPushButton *>())
                    if (b->text() == QStringLiteral("Exportar"))
                        b->click();
            });
            QVERIFY(QMetaObject::invokeMethod(&v, slot));
        }
        // Por omisión, un fichero por examen (Nacional Original y Nacional Reserva) de cada tipo.
        const QStringList ficheros = QDir(carpeta).entryList(QDir::Files);
        QCOMPARE(ficheros.filter(QStringLiteral(".pdf")).size(), 4);
        QCOMPARE(ficheros.filter(QStringLiteral("_soluciones")).size(), 2);
        QCOMPARE(ficheros.filter(QStringLiteral(".html")).size(), 2);
        QCOMPARE(ficheros.filter(QStringLiteral(".tex")).size(), 2);
        for (const QString &f : ficheros)
            QVERIFY2(QFileInfo(carpeta + QLatin1Char('/') + f).size() > 1000, qPrintable(f));
        QCOMPARE(QSettings().value(QStringLiteral("bd/ruta")).toString(), dir.filePath(QStringLiteral("banco.db")));
    }
};

QTEST_MAIN(PruebaInterfaz)
#include "prueba_interfaz.moc"
