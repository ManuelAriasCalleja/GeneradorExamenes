// Recorridos por la interfaz: el editor de preguntas, la práctica, la ventana principal (generar y exportar) y la ayuda.

#include "basedatos.h"
#include "datos_prueba.h"
#include "dialogoanteriores.h"
#include "dialogoayuda.h"
#include "dialogoestilo.h"
#include "dialogoparecidas.h"
#include "dialogopractica.h"
#include "dialogopregunta.h"
#include "dialogotemas.h"
#include "generador.h"
#include "utilidades.h"
#include "ventanaprincipal.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QtTest>
#include <functional>

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

// Banco inventado de ocho temas en 'ruta', que la ventana abre con diez preguntas de los temas 1 a 4 (3, 2, 3 y 2).
bool crearBancoInventado(const QString &ruta)
{
    BaseDatos bd;
    return bd.abrir(ruta) && llenarBaseDatos(bd, 8, bancoInventado()) && bd.guardarUltimoReparto({{1, 3}, {2, 2}, {3, 3}, {4, 2}});
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
        QApplication::setOrganizationName(QStringLiteral("UNED-pruebas"));
        QApplication::setApplicationName(QStringLiteral("GeneradorExamenes-pruebas"));
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

    void editorPreguntaAntesDeDescartar()
    {
        const QVector<Pregunta> banco = bancoInventado(2, 5);
        {
            // Sin cambios, se cierra sin preguntar.
            DialogoPregunta d(temasInventados(2), banco, banco.first());
            d.show();
            d.reject();
            QCOMPARE(d.result(), int(QDialog::Rejected));
            QVERIFY(!d.isVisible());
        }
        DialogoPregunta d(temasInventados(2), banco, banco.first());
        d.show();
        d.findChild<QPlainTextEdit *>()->setPlainText(QStringLiteral("Enunciado cambiado:"));
        QString texto;
        responder(QStringLiteral("Seguir editando"), &texto);
        d.reject();
        QVERIFY(d.isVisible());
        QVERIFY(texto.contains(QStringLiteral("cambios sin guardar")));
        responder(QStringLiteral("Descartar"));
        d.reject();
        QVERIFY(!d.isVisible());
        QCOMPARE(d.result(), int(QDialog::Rejected));
    }

    void editorGuardaYPideLaSiguiente()
    {
        const QVector<Pregunta> banco = bancoInventado(2, 5);
        for (auto despues : {DialogoPregunta::Siguiente, DialogoPregunta::Nueva}) {
            DialogoPregunta d(temasInventados(2), banco, banco.first());
            d.anadirBoton(despues, QStringLiteral("Guardar y otra cosa"));
            d.show();
            for (QPushButton *b : d.findChildren<QPushButton *>())
                if (b->text() == QStringLiteral("Guardar y otra cosa"))
                    b->click();
            QCOMPARE(d.result(), int(QDialog::Accepted));
            QCOMPARE(d.despues(), despues);
        }
        // Con «Guardar», cerrar.
        DialogoPregunta d(temasInventados(2), banco, banco.first());
        d.anadirBoton(DialogoPregunta::Siguiente, QStringLiteral("Guardar y siguiente"));
        d.show();
        for (QPushButton *b : d.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Guardar"))
                b->click();
        QCOMPARE(d.result(), int(QDialog::Accepted));
        QCOMPARE(d.despues(), DialogoPregunta::Cerrar);
    }

    void bancoGuardaYPasaALaSiguiente()
    {
        QTemporaryDir dir;
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
            QVERIFY(llenarBaseDatos(bd, 2, bancoInventado(2, 5)));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        auto *tabla = v.findChild<QTableView *>();
        tabla->setCurrentIndex(tabla->model()->index(0, 0));
        // En el primer editor, «Guardar y siguiente»; en el segundo, «Cancelar».
        QStringList titulos;
        std::function<void()> responderEditor = [&] {
            QWidget *w = QApplication::activeModalWidget();
            if (!w)
                return;
            titulos << w->windowTitle();
            const QString boton = titulos.size() == 1 ? QStringLiteral("Guardar y siguiente") : QStringLiteral("Cancelar");
            if (titulos.size() == 1)
                QTimer::singleShot(100, responderEditor);
            for (QPushButton *b : w->findChildren<QPushButton *>())
                if (b->text() == boton) {
                    b->click();
                    break;
                }
        };
        QTimer::singleShot(100, responderEditor);
        QMetaObject::invokeMethod(&v, "editarPregunta");
        QCOMPARE(titulos, (QStringList{QStringLiteral("Editar pregunta T1-1"), QStringLiteral("Editar pregunta T1-2")}));
        QSettings().clear();
    }

    void estiloGuardaYPasaALaSiguienteConAvisos()
    {
        // Preguntas sin los dos puntos del final. Al editar una se le ponen: sale de la lista, y la siguiente ocupa su
        // sitio.
        QVector<Pregunta> banco = bancoInventado(1, 4);
        for (Pregunta &p : banco)
            p.enunciado.chop(1);
        QVector<int> editadas;
        DialogoEstilo d([&banco]() { return banco; },
                        [&](int id) {
                            editadas << id;
                            for (Pregunta &p : banco)
                                if (p.id == id)
                                    p.enunciado += QLatin1Char(':');
                            return editadas.size() < 3;
                        },
                        QString());
        d.show();
        auto *arbol = d.findChild<QTreeWidget *>();
        arbol->setCurrentItem(arbol->topLevelItem(0));
        for (QPushButton *b : d.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Editar la pregunta…"))
                b->click();
        QCOMPARE(editadas, (QVector<int>{banco.at(0).id, banco.at(1).id, banco.at(2).id}));
        QCOMPARE(arbol->topLevelItemCount(), banco.size() - 3);
    }

    void practicaCorrige()
    {
        ConfigExamen c;
        c.semilla = 11;
        c.nombresModelos = {QStringLiteral("Práctica")};
        c.preguntasPorTema = {{1, 3}, {2, 2}, {3, 3}, {4, 2}};
        c.barajarOpciones = true;
        const Modelo m = Generador::generar(bancoInventado(), c).modelos.first();
        DialogoPractica p(m, DatosConvocatoria(), 45);   // +1 por acierto, −0,3 por fallo
        p.show();
        const QList<QButtonGroup *> grupos = p.findChildren<QButtonGroup *>();
        QCOMPARE(grupos.size(), 10);
        // Hasta pulsar «Empezar» no se puede contestar ni corregir; el reloj arranca con la duración elegida.
        QVERIFY(!grupos.first()->button(0)->isEnabled());
        QPushButton *empezar = nullptr;
        for (QPushButton *b : p.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Empezar"))
                empezar = b;
        QVERIFY(empezar);
        QCOMPARE(p.duracionMinutos(), 45);
        p.findChild<QSpinBox *>()->setValue(50);
        empezar->click();
        QCOMPARE(p.duracionMinutos(), 50);
        QVERIFY(grupos.first()->button(0)->isEnabled());
        QString reloj;
        for (QLabel *l : p.findChildren<QLabel *>())
            if (l->text() == QStringLiteral("50:00"))
                reloj = l->text();
        QCOMPARE(reloj, QStringLiteral("50:00"));
        // Bien las cuatro primeras, mal la quinta; la sexta se marca y se deja después en blanco.
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

    void ventanaAvisaDelJson()
    {
        QTemporaryDir dir;
        const QString rutaBd = dir.filePath(QStringLiteral("banco.db"));
        const QString json = dir.filePath(QStringLiteral("banco.json"));
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(rutaBd));
            QVERIFY(llenarBaseDatos(bd, 2, bancoInventado(2, 6)));
            QVERIFY(bd.exportarJson(json));
            QVERIFY(bd.enlazarJson(json));
            Pregunta p = bd.preguntas().first();
            p.notas = QStringLiteral("cambiada después de exportar");
            QVERIFY(bd.guardarPregunta(p));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(rutaBd));
        QPushButton *aviso = nullptr;
        for (QPushButton *b : v.findChildren<QPushButton *>())
            if (b->text().startsWith(QStringLiteral("El JSON del banco no coincide")))
                aviso = b;
        QVERIFY(aviso);
        QVERIFY(aviso->isVisible());
        QVERIFY2(aviso->toolTip().contains(QStringLiteral("1 pregunta distinta: T1-1")), qPrintable(aviso->toolTip()));

        // Al actualizarlo (confirmando), el JSON vuelve a coincidir y el aviso desaparece.
        QString texto;
        responder(QStringLiteral("Sí"), &texto);
        aviso->click();
        QVERIFY2(texto.contains(QStringLiteral("1 pregunta distinta: T1-1")), qPrintable(texto));
        QVERIFY(!aviso->isVisible());
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(rutaBd));
            QVERIFY(bd.diferenciasConJson(json).isEmpty());
        }

        // Con «Mantener el JSON al día», borrar una pregunta reescribe el JSON.
        QAction *automatico = nullptr;
        for (QAction *a : v.findChildren<QAction *>())
            if (a->text().startsWith(QStringLiteral("&Mantener el JSON")))
                automatico = a;
        QVERIFY(automatico);
        automatico->setChecked(true);
        auto *tabla = v.findChild<QTableView *>();
        QVERIFY(tabla);
        tabla->setCurrentIndex(tabla->model()->index(0, 0));
        responder(QStringLiteral("Sí"));
        QMetaObject::invokeMethod(&v, "eliminarPregunta");
        QVERIFY(!aviso->isVisible());
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(rutaBd));
            QCOMPARE(bd.preguntas().size(), bancoInventado(2, 6).size() - 1);
            QVERIFY(bd.jsonAutomatico());
            QVERIFY2(bd.diferenciasConJson(json).isEmpty(), qPrintable(bd.diferenciasConJson(json).join(QLatin1Char('\n'))));
        }
    }

    void ventanaLlevaLaAsignaturaYLosRepartosDelBanco()
    {
        QTemporaryDir dir;
        const QString primero = dir.filePath(QStringLiteral("primero.db"));
        const QString segundo = dir.filePath(QStringLiteral("segundo.db"));
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(primero));
            QVERIFY(llenarBaseDatos(bd, 4, bancoInventado(4, 6)));
            QVERIFY(bd.guardarAsignatura(QStringLiteral("Asignatura inventada")));
            QVERIFY(bd.guardarRepartos({{QStringLiteral("Primeros"), {{1, 2}, {2, 1}}}}));
            QVERIFY(bd.abrir(segundo));
            QVERIFY(llenarBaseDatos(bd, 3, bancoInventado(3, 6)));
        }
        // Las versiones anteriores guardaban el último reparto en la configuración: pasa al primer banco que se abre.
        QSettings().setValue(QStringLiteral("examen/temas"),
                             QVariantMap{{QStringLiteral("1"), 1}, {QStringLiteral("2"), 2}, {QStringLiteral("3"), 3}, {QStringLiteral("4"), 4}});
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(primero));
        QCOMPARE(v.windowTitle(), QStringLiteral("Generador de exámenes - Asignatura inventada"));
        QLineEdit *asignatura = nullptr;
        for (QLineEdit *l : v.findChildren<QLineEdit *>())
            if (l->placeholderText() == QStringLiteral("Nombre de la asignatura"))
                asignatura = l;
        QVERIFY(asignatura);
        QCOMPARE(asignatura->text(), QStringLiteral("Asignatura inventada"));
        auto casillas = [&v]() {
            QList<QSpinBox *> res;
            for (QGroupBox *g : v.findChildren<QGroupBox *>())
                if (g->title() == QStringLiteral("Preguntas por tema"))
                    res = g->findChildren<QSpinBox *>();
            return res;
        };
        auto valores = [&casillas]() {
            QList<int> res;
            for (QSpinBox *s : casillas())
                res << s->value();
            return res;
        };
        auto boton = [&v](const QString &texto) -> QPushButton * {
            for (QPushButton *b : v.findChildren<QPushButton *>())
                if (b->text() == texto)
                    return b;
            return nullptr;
        };
        QCOMPARE(valores(), (QList<int>{1, 2, 3, 4}));
        QVERIFY(!QSettings().contains(QStringLiteral("examen/temas")));

        // Los botones de reparto son los del banco, más «Ninguna».
        QVERIFY(boton(QStringLiteral("Primeros")));
        boton(QStringLiteral("Primeros"))->click();
        QCOMPARE(valores(), (QList<int>{2, 1, 0, 0}));
        boton(QStringLiteral("Ninguna"))->click();
        QCOMPARE(valores(), (QList<int>{0, 0, 0, 0}));
        QString aviso;
        responder(QStringLiteral("OK"), &aviso);
        boton(QStringLiteral("Guardar reparto…"))->click();   // sin preguntas, no hay nada que guardar
        QVERIFY2(aviso.contains(QStringLiteral("cuántas preguntas")), qPrintable(aviso));
        // Se guarda uno nuevo, con el nombre que se propone.
        boton(QStringLiteral("Primeros"))->click();
        casillas().at(2)->setValue(3);
        responder(QStringLiteral("Aceptar"));
        boton(QStringLiteral("Guardar reparto…"))->click();
        QVERIFY(boton(QStringLiteral("T1–T3")));
        QVERIFY(boton(QStringLiteral("Primeros")));
        boton(QStringLiteral("Ninguna"))->click();
        boton(QStringLiteral("T1–T3"))->click();
        QCOMPARE(valores(), (QList<int>{2, 1, 3, 0}));

        // El nombre de la asignatura, cambiado en la ventana, se guarda en el banco.
        asignatura->selectAll();
        QTest::keyClicks(asignatura, QStringLiteral("Otra asignatura inventada"));
        QTest::keyClick(asignatura, Qt::Key_Return);
        QCOMPARE(v.windowTitle(), QStringLiteral("Generador de exámenes - Otra asignatura inventada"));
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(primero));
            QCOMPARE(bd.asignatura(), QStringLiteral("Otra asignatura inventada"));
            QCOMPARE(bd.repartos().size(), 2);
            QCOMPARE(bd.repartos().at(1).preguntas, (QMap<int, int>{{1, 2}, {2, 1}, {3, 3}}));
        }

        // Cada banco, con su nombre y su último reparto: el otro no tiene ni nombre ni repartos.
        QVERIFY(v.abrirBaseDatos(segundo));
        QCOMPARE(v.windowTitle(), QStringLiteral("Generador de exámenes"));
        QVERIFY(asignatura->text().isEmpty());
        QCOMPARE(valores(), (QList<int>{0, 0, 0}));
        QVERIFY(!boton(QStringLiteral("Primeros")));
        QVERIFY(boton(QStringLiteral("Ninguna")));
        casillas().at(0)->setValue(5);
        QVERIFY(v.abrirBaseDatos(primero));
        QCOMPARE(valores(), (QList<int>{2, 1, 3, 0}));
        QVERIFY(v.abrirBaseDatos(segundo));
        QCOMPARE(valores(), (QList<int>{5, 0, 0}));
        QSettings().clear();
    }

    void temasGuardanLaFormaDeLaReferencia()
    {
        QTemporaryDir dir;
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        QVERIFY(llenarBaseDatos(bd, 3, bancoInventado(3, 2)));
        DialogoTemas d(&bd);
        const QList<QComboBox *> formas = d.findChildren<QComboBox *>();
        QCOMPARE(formas.size(), 3);
        QCOMPARE(formas.at(0)->currentData().toString(), QStringLiteral("vease"));
        QCOMPARE(formas.at(1)->currentData().toString(), QStringLiteral("pag"));
        formas.at(1)->setCurrentIndex(formas.at(1)->findData(QString()));   // cualquiera
        formas.at(2)->setCurrentIndex(formas.at(2)->findData(QStringLiteral("pag")));
        d.accept();
        QStringList guardadas;
        for (const Tema &t : bd.temas())
            guardadas << t.formatoReferencia;
        QCOMPARE(guardadas, (QStringList{QStringLiteral("vease"), QString(), QStringLiteral("pag")}));
    }

    void ventanaProponeLaProximaConvocatoria()
    {
        // Aunque la configuración tenga otra (la de una versión anterior, o una cargada del historial).
        QSettings().setValue(QStringLiteral("examen/nombreConvocatoria"), QStringLiteral("Septiembre"));
        QSettings().setValue(QStringLiteral("examen/anio"), 2001);
        VentanaPrincipal v;
        const DatosConvocatoria proxima = DatosConvocatoria::proxima(QDate::currentDate());
        QComboBox *convocatoria = nullptr;
        for (QComboBox *c : v.findChildren<QComboBox *>())
            if (c->isEditable())
                convocatoria = c;
        QVERIFY(convocatoria);
        QCOMPARE(convocatoria->currentText(), proxima.convocatoria);
        QSpinBox *anio = nullptr;
        for (QSpinBox *s : v.findChildren<QSpinBox *>())
            if (s->minimum() == 2000)
                anio = s;
        QVERIFY(anio);
        QCOMPARE(anio->value(), proxima.anio);
        QSettings().clear();
    }

    void ventanaGeneraYExporta()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        // El botón de generar no está en la zona con desplazamiento: siempre se ve.
        QPushButton *generar = nullptr;
        for (QPushButton *b : v.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Generar exámenes"))
                generar = b;
        QVERIFY(generar);
        for (QWidget *w = generar; w; w = w->parentWidget())
            QVERIFY(!qobject_cast<QScrollArea *>(w));
        generar->click();

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
        // HTML con soluciones: con la casilla del diálogo (sin «Mostrar soluciones»), y «_soluciones» en el nombre.
        QTimer::singleShot(100, [&carpeta] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            d->findChild<QLineEdit *>()->setText(carpeta);
            d->findChild<QCheckBox *>()->setChecked(true);
            for (QPushButton *b : d->findChildren<QPushButton *>())
                if (b->text() == QStringLiteral("Exportar"))
                    b->click();
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "exportarHtml"));
        // Tras exportar, la barra de estado ofrece abrir la carpeta.
        QPushButton *abrir = nullptr;
        for (QPushButton *b : v.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Abrir la carpeta"))
                abrir = b;
        QVERIFY(abrir);
        QVERIFY(abrir->isVisible());
        QCOMPARE(abrir->toolTip(), QDir::toNativeSeparators(carpeta));
        QFile conSoluciones(carpeta + QStringLiteral("/") + QDir(carpeta).entryList({QStringLiteral("*_soluciones_*.html")}).value(0));
        QVERIFY(conSoluciones.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(conSoluciones.readAll()).contains(QStringLiteral("CON SOLUCIONES")));
        // Con «Letra grande», los ficheros llevan «_letra_grande» en el nombre.
        auto *letraGrande = v.findChild<QCheckBox *>(QString(), Qt::FindChildrenRecursively);
        for (QCheckBox *c : v.findChildren<QCheckBox *>())
            if (c->text() == QStringLiteral("Letra grande"))
                letraGrande = c;
        QCOMPARE(letraGrande->text(), QStringLiteral("Letra grande"));
        letraGrande->setChecked(true);
        QTimer::singleShot(100, [&carpeta] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            d->findChild<QLineEdit *>()->setText(carpeta);
            for (QPushButton *b : d->findChildren<QPushButton *>())
                if (b->text() == QStringLiteral("Exportar"))
                    b->click();
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "exportarPdf"));
        // Por omisión, un fichero por examen (Nacional Original y Nacional Reserva) de cada tipo.
        const QStringList ficheros = QDir(carpeta).entryList(QDir::Files);
        QCOMPARE(ficheros.filter(QStringLiteral(".pdf")).size(), 6);
        QCOMPARE(ficheros.filter(QStringLiteral("_letra_grande")).size(), 2);
        QCOMPARE(ficheros.filter(QStringLiteral("_soluciones")).size(), 4);
        QCOMPARE(ficheros.filter(QStringLiteral(".html")).size(), 4);
        QCOMPARE(ficheros.filter(QStringLiteral(".tex")).size(), 2);
        for (const QString &f : ficheros)
            QVERIFY2(QFileInfo(carpeta + QLatin1Char('/') + f).size() > 1000, qPrintable(f));
        QCOMPARE(QSettings().value(QStringLiteral("bd/ruta")).toString(), dir.filePath(QStringLiteral("banco.db")));

        // El banco entero, a Moodle: el fichero se escribe en la ruta del diálogo.
        const QString moodle = carpeta + QStringLiteral("/moodle.xml");
        QTimer::singleShot(100, [&moodle] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            d->findChild<QLineEdit *>()->setText(moodle);
            for (QPushButton *b : d->findChildren<QPushButton *>())
                if (b->text() == QStringLiteral("Exportar"))
                    b->click();
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "exportarMoodle"));
        QFile f(moodle);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QString xml = QString::fromUtf8(f.readAll());
        QCOMPARE(xml.count(QStringLiteral("type=\"multichoice\"")), bancoInventado().size());
        QCOMPARE(xml.count(QStringLiteral("type=\"category\"")), 8);
    }

    void ventanaCambiaUnaPregunta()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QMetaObject::invokeMethod(&v, "generar");
        QTextBrowser *visor = nullptr;
        for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
            if (t->toPlainText().contains(QStringLiteral("Pregunta contestada correctamente")))
                visor = t;
        QVERIFY(visor);
        const QString antes = visor->toPlainText();
        QSpinBox *numero = nullptr;
        for (QSpinBox *s : v.findChildren<QSpinBox *>())
            if (s->toolTip().startsWith(QStringLiteral("Número de la pregunta que se quiere cambiar")))
                numero = s;
        QPushButton *cambiar = nullptr, *deshacer = nullptr;
        for (QPushButton *b : v.findChildren<QPushButton *>()) {
            if (b->text() == QStringLiteral("Cambiar por otra"))
                cambiar = b;
            else if (b->text() == QStringLiteral("Deshacer el cambio"))
                deshacer = b;
        }
        QVERIFY(numero && cambiar && deshacer);
        QVERIFY(cambiar->isEnabled());
        QVERIFY(!deshacer->isEnabled());

        // Con el número y el botón; se deshace con «Deshacer el cambio».
        numero->setValue(3);
        cambiar->click();
        QVERIFY(visor->toPlainText() != antes);
        QVERIFY(deshacer->isEnabled());
        bool avisa = false;
        for (QLabel *l : v.findChildren<QLabel *>())
            avisa = avisa || l->text().contains(QStringLiteral("con una pregunta cambiada a mano"));
        QVERIFY(avisa);
        deshacer->click();
        QCOMPARE(visor->toPlainText(), antes);
        QVERIFY(!deshacer->isEnabled());

        // Con el botón derecho sobre la pregunta 5.
        QTextBlock bloque;
        for (QTextBlock b = visor->document()->begin(); b.isValid() && !bloque.isValid(); b = b.next())
            for (auto it = b.begin(); !it.atEnd(); ++it)
                if (it.fragment().charFormat().anchorNames().contains(QStringLiteral("q5")))
                    bloque = b;
        QVERIFY(bloque.isValid());
        QTextCursor cursor(bloque);
        visor->setTextCursor(cursor);
        visor->ensureCursorVisible();
        const QPoint punto = visor->cursorRect(cursor).center() + QPoint(4, 0);
        QString opcion;
        QTimer::singleShot(100, [&opcion] {
            for (QWidget *w : QApplication::topLevelWidgets()) {
                auto *menu = qobject_cast<QMenu *>(w);
                if (!menu || !menu->isVisible())
                    continue;
                for (QAction *a : menu->actions())
                    if (a->text().startsWith(QStringLiteral("Cambiar la pregunta"))) {
                        opcion = a->text();
                        menu->setActiveAction(a);
                    }
                QTest::keyClick(menu, opcion.isEmpty() ? Qt::Key_Escape : Qt::Key_Return);
            }
        });
        QContextMenuEvent evento(QContextMenuEvent::Mouse, punto, visor->viewport()->mapToGlobal(punto));
        QApplication::sendEvent(visor->viewport(), &evento);
        QVERIFY2(opcion.startsWith(QStringLiteral("Cambiar la pregunta 5 por otra del tema")), qPrintable(opcion));
        QVERIFY(visor->toPlainText() != antes);
        QCOMPARE(numero->value(), 5);
    }

    void registrarExamenesAnteriores()
    {
        QTemporaryDir dir;
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        QVERIFY(llenarBaseDatos(bd, 4, bancoVariado()));
        // Un examen de cinco preguntas del banco, como texto pegado.
        QString texto = QStringLiteral("Examen inventado\n");
        const QVector<Pregunta> banco = bd.preguntas();
        for (int i = 0; i < 5; ++i) {
            texto += QStringLiteral("%1.- %2\n").arg(i + 1).arg(banco.at(i).enunciado);
            for (int k = 0; k < 4; ++k)
                texto += QStringLiteral("%1) %2\n").arg(Pregunta::letra(k)).arg(banco.at(i).opciones.at(k));
        }
        DialogoAnteriores d(&bd, banco, DatosConvocatoria(), {QStringLiteral("Nacional Original")});
        d.show();
        QVERIFY(d.anadirExamen(texto));
        d.findChild<QComboBox *>()->setCurrentText(QStringLiteral("Febrero"));
        d.findChild<QSpinBox *>()->setValue(2026);
        QString pregunta;
        responder(QStringLiteral("Sí"), &pregunta);
        d.accept();
        QCOMPARE(d.result(), int(QDialog::Accepted));
        QVERIFY2(pregunta.contains(QStringLiteral("1 exámenes de Febrero 2026")), qPrintable(pregunta));
        const QVector<ExamenRegistrado> registrados = bd.examenesRegistrados();
        QCOMPARE(registrados.size(), 1);
        QCOMPARE(registrados.first().datos.convocatoriaYAnio(), QStringLiteral("Febrero 2026"));
        QCOMPARE(registrados.first().nombre, QStringLiteral("Examen 1"));
        QCOMPARE(registrados.first().numPreguntas, 5);
        // Sus preguntas cuentan ya como salidas al generar la convocatoria siguiente.
        QCOMPARE(bd.usoReciente(1, 2026, QStringLiteral("Junio"), {}).size(), 5);
    }

    void bancoBuscaSinTildes()
    {
        QTemporaryDir dir;
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
            QVERIFY(llenarBaseDatos(bd, 4, bancoVariado()));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QLineEdit *buscar = nullptr;
        for (QLineEdit *l : v.findChildren<QLineEdit *>())
            if (l->placeholderText().startsWith(QStringLiteral("Buscar texto")))
                buscar = l;
        QVERIFY(buscar);
        auto *tabla = v.findChild<QTableView *>();
        // «direccion» y «DIRECCIÓN» encuentran «dirección» (en una sola pregunta del banco variado), y el código, la suya.
        for (const QString &texto : {QStringLiteral("direccion"), QStringLiteral("DIRECCIÓN"), QStringLiteral("T1-2")}) {
            buscar->setText(texto);
            QCOMPARE(tabla->model()->rowCount(), 1);
        }
        buscar->setText(QStringLiteral("no aparece en ninguna"));
        QCOMPARE(tabla->model()->rowCount(), 0);
        QSettings().clear();
    }

    void ventanaNoPierdeLosExportadosSinRegistrar()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QMetaObject::invokeMethod(&v, "generar");
        QTextBrowser *visor = nullptr;
        for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
            if (t->toPlainText().contains(QStringLiteral("Pregunta contestada correctamente")))
                visor = t;
        QVERIFY(visor);
        QPushButton *registrar = nullptr;
        for (QPushButton *b : v.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Registrar convocatoria…"))
                registrar = b;
        QVERIFY(registrar);
        // Sin exportar, se generan otros sin preguntar.
        QMetaObject::invokeMethod(&v, "generar");
        QVERIFY(registrar->styleSheet().isEmpty());

        // Exportados: el botón de registrar avisa y, al generar otros, se pregunta.
        const QString carpeta = dir.filePath(QStringLiteral("exportados"));
        QTimer::singleShot(100, [&carpeta] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            d->findChild<QLineEdit *>()->setText(carpeta);
            for (QPushButton *b : d->findChildren<QPushButton *>())
                if (b->text() == QStringLiteral("Exportar"))
                    b->click();
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "exportarPdf"));
        QVERIFY(!registrar->styleSheet().isEmpty());
        const QString exportados = visor->toPlainText();
        QString texto;
        responder(QStringLiteral("Cancelar"), &texto);
        QMetaObject::invokeMethod(&v, "generar");
        QVERIFY2(texto.contains(QStringLiteral("no se han registrado")), qPrintable(texto));
        QCOMPARE(visor->toPlainText(), exportados);

        // Al salir, «Registrarlos…» los registra y el programa se cierra.
        responder(QStringLiteral("Registrarlos"), nullptr, QStringLiteral("Sí"));
        v.close();
        QVERIFY(!v.isVisible());
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        QCOMPARE(bd.examenesRegistrados().size(), 2);
        QSettings().clear();
    }

    void ventanaAvisaDeLaSemillaFija()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QLineEdit *semilla = nullptr;
        for (QLineEdit *l : v.findChildren<QLineEdit *>())
            if (l->placeholderText() == QStringLiteral("aleatoria"))
                semilla = l;
        QVERIFY(semilla);
        QTextBrowser *visor = nullptr;
        auto infoDiceFija = [&v] {
            for (QLabel *l : v.findChildren<QLabel *>())
                if (l->text().startsWith(QStringLiteral("Semilla:")))
                    return l->text().contains(QStringLiteral("fija"));
            return false;
        };
        semilla->setText(QStringLiteral("123"));
        QVERIFY(!semilla->styleSheet().isEmpty());
        QMetaObject::invokeMethod(&v, "generar");
        for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
            if (t->toPlainText().contains(QStringLiteral("Pregunta contestada correctamente")))
                visor = t;
        QVERIFY(visor);
        const QString primera = visor->toPlainText();
        QVERIFY(infoDiceFija());
        QMetaObject::invokeMethod(&v, "generar");
        QCOMPARE(visor->toPlainText(), primera);
        // Sin semilla, deja de ser fija.
        semilla->clear();
        QVERIFY(semilla->styleSheet().isEmpty());
        QMetaObject::invokeMethod(&v, "generar");
        QVERIFY(!infoDiceFija());
        QSettings().clear();
    }

    void ayudaTieneLosApartados()
    {
        DialogoAyuda ayuda;
        ayuda.show();
        const QStringList secciones = ayuda.secciones();
        QVERIFY(secciones.size() > 30);
        QVERIFY2(!secciones.contains(QString()), "hay un título sin ancla: el índice no puede llevar a él");
        // Los apartados a los que llevan F1 y el botón «Ayuda» de cada parte del programa.
        for (const char *s : {"generar", "banco", "editor", "parecidas", "temas", "practicar", "estilo", "exportar",
                              "historial", "anteriores", "moodle"})
            QVERIFY2(secciones.contains(QLatin1String(s)), s);

        // Los enlaces del manual llevan a anclas que existen.
        auto *texto = ayuda.findChild<QTextBrowser *>();
        QSet<QString> anclas;
        QStringList enlaces;
        for (QTextBlock b = texto->document()->begin(); b.isValid(); b = b.next()) {
            for (auto it = b.begin(); !it.atEnd(); ++it) {
                const QTextCharFormat f = it.fragment().charFormat();
                for (const QString &a : f.anchorNames())
                    anclas.insert(a);
                if (f.anchorHref().startsWith(QLatin1Char('#')))
                    enlaces << f.anchorHref().mid(1);
            }
        }
        QVERIFY(enlaces.size() > 10);
        for (const QString &e : std::as_const(enlaces))
            QVERIFY2(anclas.contains(e), qPrintable(e));

        // El buscador marca lo que se escribe e Intro pasa a la siguiente vez que aparece.
        auto *buscar = ayuda.findChild<QLineEdit *>();
        QTest::keyClicks(buscar, QStringLiteral("semilla"));
        QCOMPARE(texto->textCursor().selectedText().toLower(), QStringLiteral("semilla"));
        const int antes = texto->textCursor().position();
        QTest::keyClick(buscar, Qt::Key_Return);
        QCOMPARE(texto->textCursor().selectedText().toLower(), QStringLiteral("semilla"));
        QVERIFY(texto->textCursor().position() > antes);
        // Si no está, no se mueve.
        QTest::keyClicks(buscar, QStringLiteral("xyz"));
        QCOMPARE(texto->textCursor().position(), texto->textCursor().anchor());
        // Sin distinguir tildes: «examenes» encuentra «exámenes».
        buscar->clear();
        QTest::keyClicks(buscar, QStringLiteral("examenes"));
        QCOMPARE(texto->textCursor().selectedText().toLower(), QStringLiteral("exámenes"));
    }

    void ayudaPorLaVentanaDesdeLaQueSePide()
    {
        // El botón «Ayuda» del editor la abre por su apartado, hija del editor: si no, el editor (modal) la bloquearía.
        auto *editor = new DialogoPregunta(temasInventados(2), bancoInventado(2, 5), nuevaEn(1, 6));
        editor->show();
        QPushButton *boton = nullptr;
        for (QPushButton *b : editor->findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("Ayuda"))
                boton = b;
        QVERIFY(boton);
        boton->click();
        QPointer<DialogoAyuda> delEditor = editor->findChild<DialogoAyuda *>();
        QVERIFY(delEditor);
        QVERIFY(delEditor->isVisible());
        QCOMPARE(delEditor->seccionActual(), QStringLiteral("editor"));

        // Desde la ventana principal, en la pestaña del banco, por el capítulo del banco. Sigue habiendo una sola
        // ventana de ayuda, ahora hija de la principal.
        VentanaPrincipal v;
        v.show();
        v.findChild<QTabWidget *>()->setCurrentIndex(1);
        QAction *manual = nullptr;
        for (QAction *a : v.findChildren<QAction *>())
            if (a->text() == QStringLiteral("&Manual de uso"))
                manual = a;
        QVERIFY(manual);
        manual->trigger();
        QVERIFY(!delEditor);
        auto *ayuda = v.findChild<DialogoAyuda *>();
        QVERIFY(ayuda);
        QVERIFY(ayuda->isVisible());
        QCOMPARE(ayuda->seccionActual(), QStringLiteral("banco"));
        delete editor;
    }

    // Los diálogos para elegir un banco se abren en la carpeta del banco abierto o, si no hay ninguno, en la del último
    // que se abrió (o la más cercana que siga existiendo, si se ha borrado).
    void carpetaDeLosBancos()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString raiz = QFileInfo(tmp.path()).absoluteFilePath();
        QVERIFY(QDir(raiz).mkpath(QStringLiteral("abierto")));
        QVERIFY(QDir(raiz).mkpath(QStringLiteral("ultimo")));
        const QString abierto = raiz + QStringLiteral("/abierto/banco.db");
        const QString ultimo = raiz + QStringLiteral("/ultimo/banco.db");
        QCOMPARE(carpetaBancos(abierto, ultimo), raiz + QStringLiteral("/abierto"));
        // El último banco ya no está, pero sí su carpeta.
        QCOMPARE(carpetaBancos(QString(), ultimo), raiz + QStringLiteral("/ultimo"));
        // Ni su carpeta: la más cercana que exista.
        QCOMPARE(carpetaBancos(QString(), raiz + QStringLiteral("/borrada/dentro/banco.db")), raiz);
        QCOMPARE(carpetaBancos(QString(), QString()),
                 QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }

    // La configuración de cuando el programa se llamaba GeneradorExamenesFI pasa a la nueva si esta está vacía, con
    // sus tipos (texto, listas y datos binarios, como la geometría de la ventana), sin tocar la antigua. Si la nueva
    // ya tiene algo, no se vuelve a copiar.
    void configuracionAntigua()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QByteArray binario("\x01\x02\x00\xff", 4);
        const QStringList lista{QStringLiteral("Original"), QStringLiteral("Reserva")};
        {
            QSettings escrita(tmp.filePath(QStringLiteral("antigua.ini")), QSettings::IniFormat);
            escrita.setValue(QStringLiteral("bd/ruta"), QStringLiteral("/ruta/inventada/banco.db"));
            escrita.setValue(QStringLiteral("ventana/geometria"), binario);
            escrita.setValue(QStringLiteral("examen/nombres"), lista);
        }
        const QSettings antigua(tmp.filePath(QStringLiteral("antigua.ini")), QSettings::IniFormat);
        QSettings nueva(tmp.filePath(QStringLiteral("nueva.ini")), QSettings::IniFormat);
        QVERIFY(traerConfiguracionAntigua(antigua, nueva));
        {
            const QSettings leida(tmp.filePath(QStringLiteral("nueva.ini")), QSettings::IniFormat);
            QCOMPARE(leida.value(QStringLiteral("bd/ruta")).toString(), QStringLiteral("/ruta/inventada/banco.db"));
            QCOMPARE(leida.value(QStringLiteral("ventana/geometria")).toByteArray(), binario);
            QCOMPARE(leida.value(QStringLiteral("examen/nombres")).toStringList(), lista);
        }
        QCOMPARE(antigua.allKeys().size(), 3);

        nueva.setValue(QStringLiteral("bd/ruta"), QStringLiteral("/otra/ruta/banco.db"));
        QVERIFY(!traerConfiguracionAntigua(antigua, nueva));
        QCOMPARE(nueva.value(QStringLiteral("bd/ruta")).toString(), QStringLiteral("/otra/ruta/banco.db"));

        // Sin configuración antigua no se copia nada.
        const QSettings ninguna(tmp.filePath(QStringLiteral("ninguna.ini")), QSettings::IniFormat);
        QSettings vacia(tmp.filePath(QStringLiteral("vacia.ini")), QSettings::IniFormat);
        QVERIFY(!traerConfiguracionAntigua(ninguna, vacia));
        QVERIFY(vacia.allKeys().isEmpty());
    }
};

QTEST_MAIN(PruebaInterfaz)
#include "prueba_interfaz.moc"
