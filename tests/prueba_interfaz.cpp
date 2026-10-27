// Recorridos por la interfaz: el editor de preguntas, la práctica, la ventana principal (generar y exportar) y la ayuda.

#include "basedatos.h"
#include "datos_prueba.h"
#include "dialogoanteriores.h"
#include "dialogoayuda.h"
#include "dialogoparecidas.h"
#include "dialogopractica.h"
#include "dialogopregunta.h"
#include "generador.h"
#include "ventanaprincipal.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPlainTextEdit>
#include <QPushButton>
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
        QCOMPARE(ficheros.filter(QStringLiteral("_soluciones")).size(), 2);
        QCOMPARE(ficheros.filter(QStringLiteral(".html")).size(), 2);
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
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
            QVERIFY(llenarBaseDatos(bd, 8, bancoInventado()));
        }
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
};

QTEST_MAIN(PruebaInterfaz)
#include "prueba_interfaz.moc"
