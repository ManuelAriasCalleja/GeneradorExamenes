// Recorridos por la interfaz: el editor de preguntas, la práctica, la ventana principal (generar y exportar) y la ayuda.

#include "basedatos.h"
#include "datos_prueba.h"
#include "dialogoanteriores.h"
#include "dialogoayuda.h"
#include "dialogocomprobacion.h"
#include "dialogohistorial.h"
#include "dialogoestilo.h"
#include "dialogoexportar.h"
#include "dialogomoodle.h"
#include "dialogoparecidas.h"
#include "dialogopractica.h"
#include "dialogopregunta.h"
#include "dialogotemas.h"
#include "generador.h"
#include "idioma.h"
#include "modelopreguntas.h"
#include "utilidades.h"
#include "ventanaprincipal.h"
#include "atencionalverificar.h"
#include "dialogoverificadores.h"
#include "dialogoverificar.h"
#include "estilo.h"
#include "panelverificacion.h"
#include "paleta.h"
#include "pestanabanco.h"
#include "visorampliable.h"

#include <QPalette>
#include <QRegularExpression>
#include <cmath>
#include <QLocale>
#include <QDateTime>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include "grupoplegable.h"
#include <QLineEdit>
#include <QInputDialog>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableView>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTextTable>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QWheelEvent>
#include <QtTest>
#include <algorithm>
#include <functional>
#include <memory>

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

// El control de ese tipo (un botón, una casilla…) que hay en 'w' con ese texto; nullptr si no lo hay.
template <class T>
T *conTexto(const QWidget *w, const QString &texto)
{
    for (T *control : w->findChildren<T *>())
        if (control->text() == texto)
            return control;
    return nullptr;
}

// Pulsa el botón de 'w' con ese texto; false si no lo hay.
bool pulsar(const QWidget *w, const QString &texto)
{
    QPushButton *boton = conTexto<QPushButton>(w, texto);
    if (boton)
        boton->click();
    return boton != nullptr;
}

// Marca (o desmarca) la casilla de 'w' con ese texto; false si no la hay.
bool marcar(const QWidget *w, const QString &texto, bool marcada = true)
{
    QCheckBox *casilla = conTexto<QCheckBox>(w, texto);
    if (casilla)
        casilla->setChecked(marcada);
    return casilla != nullptr;
}

// El texto del botón de Intro (el botón por defecto) de 'd'; vacío si no tiene.
QString botonDeIntro(const QDialog *d)
{
    QStringList textos;
    for (QPushButton *b : d->findChildren<QPushButton *>())
        if (b->isDefault() && b->isVisible())
            textos << b->text();
    return textos.join(QStringLiteral(" + "));
}

// Abre 'd' y comprueba que el botón de Intro es 'esperado', al abrirlo y después de que el foco pase por cada uno de
// los otros botones y vuelva a otro control (Qt le da Intro al botón que tiene el foco y, al salir de él, se lo
// devuelve al botón por defecto, si se entera de cuál es). Devuelve lo que falla; vacío si nada.
QString problemaConElBotonDeIntro(QDialog *d, const QString &esperado)
{
    if (!d->isVisible())
        d->show();
    if (!QTest::qWaitForWindowActive(d))
        return QStringLiteral("«%1» no se activa").arg(d->windowTitle());
    if (botonDeIntro(d) != esperado)
        return QStringLiteral("«%1»: al abrirlo, Intro es «%2» y no «%3»").arg(d->windowTitle(), botonDeIntro(d), esperado);
    QWidget *otro = d;
    for (QWidget *w : d->findChildren<QWidget *>())
        if (!qobject_cast<QAbstractButton *>(w) && (w->focusPolicy() & Qt::TabFocus) && w->isVisible() && w->isEnabled()
            && w->window() == d) {
            otro = w;
            break;
        }
    for (QPushButton *b : d->findChildren<QPushButton *>()) {
        if (!b->isVisible() || !b->isEnabled() || b->isDefault() || b->window() != d)
            continue;
        b->setFocus(Qt::TabFocusReason);
        otro->setFocus(Qt::TabFocusReason);
        if (botonDeIntro(d) != esperado)
            return QStringLiteral("«%1»: después de pasar por «%2», Intro es «%3» y no «%4»")
                .arg(d->windowTitle(), b->text(), botonDeIntro(d), esperado);
    }
    return QString();
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

// Lo que mide cada trozo de texto del documento (el tamaño de su letra en puntos, o en píxeles si lo lleva así) y cada
// imagen (su ancho), y la letra de cada línea vacía (la de su bloque).
QVector<qreal> tamanos(const QTextDocument *doc)
{
    auto letra = [doc](const QTextCharFormat &f) {
        const QFont fuente = f.font();
        if (fuente.pixelSize() > 0)
            return qreal(fuente.pixelSize());
        return fuente.pointSizeF() > 0 ? fuente.pointSizeF() : doc->defaultFont().pointSizeF();
    };
    QVector<qreal> t;
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        if (b.length() == 1)
            t << letra(b.charFormat());
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat f = it.fragment().charFormat();
            t << letra(f);
            if (f.isImageFormat())
                t << f.toImageFormat().width();
        }
    }
    return t;
}

// Si cada medida de 'a' es 'escala' veces la de 'b' (con medio punto de margen, por los píxeles redondeados).
bool proporcionales(const QVector<qreal> &a, const QVector<qreal> &b, qreal escala)
{
    if (a.size() != b.size() || a.isEmpty())
        return false;
    for (int i = 0; i < a.size(); ++i)
        if (qAbs(a.at(i) - b.at(i) * escala) > 0.5)
            return false;
    return true;
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
        // La interfaz en castellano, sea cual sea el idioma del sistema (la ayuda, los decimales…), sin las traducciones
        // de Qt: responder() pulsa los botones estándar por su texto («OK»).
        Idioma::instalar(QStringLiteral("es"), false);
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
            pulsar(&d, QStringLiteral("Guardar y otra cosa"));
            QCOMPARE(d.result(), int(QDialog::Accepted));
            QCOMPARE(d.despues(), despues);
        }
        // Con «Guardar», cerrar.
        DialogoPregunta d(temasInventados(2), banco, banco.first());
        d.anadirBoton(DialogoPregunta::Siguiente, QStringLiteral("Guardar y siguiente"));
        d.show();
        pulsar(&d, QStringLiteral("Guardar"));
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
            pulsar(w, boton);
        };
        QTimer::singleShot(100, responderEditor);
        QMetaObject::invokeMethod(v.findChild<PestanaBanco *>(), "editarPregunta");
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
        pulsar(&d, QStringLiteral("Editar la pregunta…"));
        QCOMPARE(editadas, (QVector<int>{banco.at(0).id, banco.at(1).id, banco.at(2).id}));
        QCOMPARE(arbol->topLevelItemCount(), banco.size() - 3);
    }

    // En la práctica, los textos salen como en el examen: las opciones con «&» o «>» no enseñan sus entidades de HTML,
    // el código en línea va sin sus comillas invertidas (también en la explicación) y el enunciado sigue la casilla
    // «Enunciados en cursiva».
    void practicaEnsenaLosTextosComoElExamen()
    {
        Modelo m;
        PreguntaExamen pe;
        pe.pregunta = bancoInventado(1, 1).first();
        pe.pregunta.enunciado = QStringLiteral("Enunciado inventado con `nombre`:");
        pe.pregunta.opciones = QStringList{QStringLiteral("x > 3 && y."), QStringLiteral("El valor de `uno`."), QStringLiteral("Dos."), QStringLiteral("Tres.")};
        pe.pregunta.explicacion = QStringLiteral("Porque `uno` es inventado.");
        m.preguntas << pe;
        DatosConvocatoria datos;
        datos.enunciadoEnCursiva = false;
        DialogoPractica p(m, datos, 45);
        p.show();
        int opciones = 0;
        bool enunciado = false;
        for (const QLabel *l : p.findChildren<QLabel *>()) {
            if (l->text().contains(QStringLiteral("x &gt; 3 &amp;&amp; y."))) {
                QCOMPARE(l->textFormat(), Qt::RichText);
                ++opciones;
            }
            if (l->text().contains(QStringLiteral("El valor de <span"))) {
                QCOMPARE(l->textFormat(), Qt::RichText);
                ++opciones;
            }
            if (l->text().contains(QStringLiteral("Enunciado inventado con"))) {
                enunciado = true;
                QVERIFY(l->text().startsWith(QStringLiteral("<div style=\"font-weight:bold\">")));
                QVERIFY(!l->text().contains(QLatin1Char('`')));
                // Va sobre papel, también con el tema oscuro: el código, las tablas y los dibujos son para blanco.
                QVERIFY(l->autoFillBackground());
                QCOMPARE(l->palette().color(QPalette::Window), QColor(Qt::white));
                QCOMPARE(l->palette().color(QPalette::WindowText), QColor(Qt::black));
            }
        }
        QCOMPARE(opciones, 2);
        QVERIFY(enunciado);
        pulsar(&p, QStringLiteral("Empezar"));
        QTimer::singleShot(100, [] {
            if (QWidget *d = QApplication::activeModalWidget())
                d->close();
        });
        QMetaObject::invokeMethod(&p, "corregir");
        bool explicacion = false;
        for (const QLabel *l : p.findChildren<QLabel *>())
            if (l->text().contains(QStringLiteral("es inventado."))) {
                explicacion = true;
                QVERIFY2(!l->text().contains(QLatin1Char('`')), qPrintable(l->text()));
                QVERIFY(l->text().contains(QStringLiteral("font-style:normal\">uno</span>")));
            }
        QVERIFY(explicacion);
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
        auto *empezar = conTexto<QPushButton>(&p, QStringLiteral("Empezar"));
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
        // El reloj se puede ocultar (y se recuerda): el tiempo corre igual.
        {
            QLabel *etiquetaReloj = nullptr;
            for (QLabel *l : p.findChildren<QLabel *>())
                if (l->text() == QStringLiteral("50:00"))
                    etiquetaReloj = l;
            auto *ocultar = p.findChild<QCheckBox *>(QStringLiteral("ocultarReloj"));
            QVERIFY(ocultar && !ocultar->isChecked());
            QVERIFY(etiquetaReloj->isVisible());
            ocultar->setChecked(true);
            QVERIFY(!etiquetaReloj->isVisible());
            QVERIFY(QSettings().value(QStringLiteral("practica/ocultarReloj")).toBool());
            DialogoPractica otra(m, DatosConvocatoria(), 45);
            QVERIFY(otra.findChild<QCheckBox *>(QStringLiteral("ocultarReloj"))->isChecked());
            ocultar->setChecked(false);
            QVERIFY(etiquetaReloj->isVisible());
        }
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
        QMetaObject::invokeMethod(v.findChild<PestanaBanco *>(), "eliminarPregunta");
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

    void cadaBancoConSusOpciones()
    {
        // Dos asignaturas, cada una con su banco en su carpeta.
        QTemporaryDir dir;
        QDir(dir.path()).mkpath(QStringLiteral("a"));
        QDir(dir.path()).mkpath(QStringLiteral("b"));
        const QString primero = dir.filePath(QStringLiteral("a/banco.db")), segundo = dir.filePath(QStringLiteral("b/banco.db"));
        QVERIFY(crearBancoInventado(primero));
        QVERIFY(crearBancoInventado(segundo));
        auto nativa = [](const QString &ruta) { return QDir::toNativeSeparators(QDir::cleanPath(ruta)); };
        auto carpeta = [] { return QSettings().value(QStringLiteral("exportar/carpeta")).toString(); };

        // La configuración de una versión anterior, que tenía las opciones de todos los bancos: son las del último que
        // se abrió, y se las queda él.
        QSettings().clear();
        QSettings().setValue(QStringLiteral("bd/ruta"), primero);
        QSettings().setValue(QStringLiteral("exportar/carpeta"), nativa(dir.filePath(QStringLiteral("a/exportados"))));
        QSettings().setValue(QStringLiteral("examen/numRecientes"), 4);
        QSettings().setValue(QStringLiteral("examenes/nombres"), QStringList{QStringLiteral("Original"), QStringLiteral("Reserva")});
        QSettings().setValue(QStringLiteral("examenes/marcados"), QStringList{QStringLiteral("Original")});

        auto ventana = std::make_unique<VentanaPrincipal>();
        VentanaPrincipal &v = *ventana;
        v.show();
        auto lista = [&v]() -> QListWidget * {
            for (QListWidget *l : v.findChildren<QListWidget *>())
                if (l->toolTip().startsWith(QStringLiteral("Se genera un examen")))
                    return l;
            return nullptr;
        };
        auto examenes = [&lista]() {
            QStringList res;
            for (int i = 0; i < lista()->count(); ++i)
                res << lista()->item(i)->text() + (lista()->item(i)->checkState() == Qt::Checked ? QStringLiteral(" ✓") : QString());
            return res;
        };
        auto recientes = [&v]() -> QSpinBox * {
            for (QSpinBox *s : v.findChildren<QSpinBox *>())
                if (s->suffix() == QStringLiteral(" convocatorias"))
                    return s;
            return nullptr;
        };
        QVERIFY(v.abrirBancoInicial(QString()));
        QVERIFY(lista() && recientes());
        QCOMPARE(examenes(), (QStringList{QStringLiteral("Original ✓"), QStringLiteral("Reserva")}));
        QCOMPARE(recientes()->value(), 4);
        QCOMPARE(carpeta(), nativa(dir.filePath(QStringLiteral("a/exportados"))));
        recientes()->setValue(9);
        lista()->item(1)->setCheckState(Qt::Checked);

        // Otro banco, aún sin opciones: se queda con las de la ventana, pero las carpetas son la suya.
        QVERIFY(v.abrirBaseDatos(segundo));
        QCOMPARE(examenes(), (QStringList{QStringLiteral("Original ✓"), QStringLiteral("Reserva ✓")}));
        QCOMPARE(recientes()->value(), 9);
        QCOMPARE(carpeta(), nativa(dir.filePath(QStringLiteral("b"))));
        {
            // El primero ya tiene las suyas, con la carpeta relativa a la del banco.
            BaseDatos bd;
            QVERIFY(bd.abrir(primero));
            QCOMPARE(bd.opciones().value(QStringLiteral("examen/numRecientes")).toInt(), 9);
            QCOMPARE(bd.opciones().value(QStringLiteral("exportar/carpeta")).toString(), QStringLiteral("exportados"));
        }
        lista()->clear();
        for (const QString &nombre : {QStringLiteral("Primera semana"), QStringLiteral("Segunda semana")})
            (new QListWidgetItem(nombre, lista()))->setCheckState(nombre.startsWith(QLatin1Char('P')) ? Qt::Checked : Qt::Unchecked);
        recientes()->setValue(2);
        QSettings().setValue(QStringLiteral("exportar/carpeta"), nativa(dir.filePath(QStringLiteral("b/examenes"))));   // como el diálogo

        // Cada banco, con las suyas.
        QVERIFY(v.abrirBaseDatos(primero));
        QCOMPARE(examenes(), (QStringList{QStringLiteral("Original ✓"), QStringLiteral("Reserva ✓")}));
        QCOMPARE(recientes()->value(), 9);
        QCOMPARE(carpeta(), nativa(dir.filePath(QStringLiteral("a/exportados"))));
        QVERIFY(v.abrirBaseDatos(segundo));
        QCOMPARE(examenes(), (QStringList{QStringLiteral("Primera semana ✓"), QStringLiteral("Segunda semana")}));
        QCOMPARE(recientes()->value(), 2);
        QCOMPARE(carpeta(), nativa(dir.filePath(QStringLiteral("b/examenes"))));

        // También al volver a abrir el programa: la configuración tiene las del segundo, y el primero, las suyas.
        v.close();
        ventana.reset();
        VentanaPrincipal w;
        w.show();
        QVERIFY(w.abrirBancoInicial(primero));
        QSpinBox *recientesW = nullptr;
        for (QSpinBox *s : w.findChildren<QSpinBox *>())
            if (s->suffix() == QStringLiteral(" convocatorias"))
                recientesW = s;
        QVERIFY(recientesW);
        QCOMPARE(recientesW->value(), 9);
        QCOMPARE(carpeta(), nativa(dir.filePath(QStringLiteral("a/exportados"))));
        w.close();
        QSettings().clear();
    }

    void listaDeExamenesConAtajos()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(QTest::qWaitForWindowActive(&v));
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QListWidget *lista = nullptr;
        QList<QPushButton *> botones;
        QGroupBox *grupo = nullptr;
        for (QGroupBox *g : v.findChildren<QGroupBox *>())
            if (g->title().startsWith(QStringLiteral("Exámenes:"))) {
                grupo = g;
                lista = g->findChild<QListWidget *>();
                botones = g->findChildren<QPushButton *>();
            }
        QVERIFY(lista);
        QCOMPARE(botones.size(), 4);

        // Cada botón tiene su atajo (Alt y una letra), distinto del de los otros y de los de los menús, y lo dice su
        // descripción emergente. En macOS no existen estos atajos: los botones se pulsan.
        const bool conAtajos = !QKeySequence::mnemonic(QStringLiteral("&a")).isEmpty();
        QSet<QString> atajos;
        for (QAction *menu : v.menuBar()->actions())
            atajos << QKeySequence::mnemonic(menu->text()).toString();
        for (QPushButton *b : botones) {
            if (!conAtajos)
                break;
            const QString atajo = b->shortcut().toString();
            QVERIFY2(atajo.startsWith(QStringLiteral("Alt+")), qPrintable(b->text()));
            QVERIFY2(!atajos.contains(atajo), qPrintable(atajo));
            atajos << atajo;
            QVERIFY2(b->toolTip().contains(b->shortcut().toString(QKeySequence::NativeText)), qPrintable(b->toolTip()));
        }
        auto usar = [&](Qt::Key tecla) {
            if (conAtajos) {
                QTest::keyClick(&v, tecla, Qt::AltModifier);
                return;
            }
            for (QPushButton *b : botones)
                if (b->text().contains(QStringLiteral("&") + QChar(int(tecla)), Qt::CaseInsensitive))
                    QTimer::singleShot(0, b, &QPushButton::click);
        };

        // Y funcionan: Ninguno, Todos, Añadir y Quitar (el botón se pulsa un momento después de la tecla).
        auto marcados = [lista]() {
            int n = 0;
            for (int i = 0; i < lista->count(); ++i)
                n += lista->item(i)->checkState() == Qt::Checked ? 1 : 0;
            return n;
        };
        // El título del recuadro dice cuántos hay marcados de cuántos (con «modelo» en singular si es uno).
        const int examenes = lista->count();
        QVERIFY(examenes > 1 && marcados() > 0);
        auto titulo = [](int marcados, int total) {
            return marcados == 1 ? QStringLiteral("Exámenes: 1 modelo de %1 seleccionado.").arg(total)
                                 : QStringLiteral("Exámenes: %1 modelos de %2 seleccionados.").arg(marcados).arg(total);
        };
        QCOMPARE(grupo->title(), titulo(marcados(), examenes));
        usar(Qt::Key_N);
        QTRY_COMPARE(marcados(), 0);
        QCOMPARE(grupo->title(), titulo(0, examenes));
        lista->item(0)->setCheckState(Qt::Checked);
        QCOMPARE(grupo->title(), titulo(1, examenes));
        usar(Qt::Key_T);
        QTRY_COMPARE(marcados(), examenes);
        QCOMPARE(grupo->title(), titulo(examenes, examenes));
        usar(Qt::Key_D);
        QTRY_COMPARE(lista->count(), examenes + 1);
        QCOMPARE(grupo->title(), titulo(examenes + 1, examenes + 1));
        QTest::keyClick(QApplication::focusWidget(), Qt::Key_Escape);   // deja el nombre que se propone
        QCOMPARE(lista->currentRow(), examenes);
        // La confirmación sale al pulsarse el botón, un momento después de la tecla: se responde cuando aparece.
        QTimer espera;
        QObject::connect(&espera, &QTimer::timeout, [&espera] {
            if (QApplication::activeModalWidget()) {
                espera.stop();
                responder(QStringLiteral("Sí"));
            }
        });
        espera.start(20);
        usar(Qt::Key_Q);
        QTRY_COMPARE(lista->count(), examenes);
        QCOMPARE(grupo->title(), titulo(examenes, examenes));
        v.close();
        QSettings().clear();
    }

    void cuatroFormasDePuntuar()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));   // diez preguntas
        // La configuración de una versión anterior, con la casilla «Automática» sin marcar: es la personalizada.
        QSettings().clear();
        QSettings().setValue(QStringLiteral("examen/puntuacionAuto"), false);
        QSettings().setValue(QStringLiteral("examen/acierto"), 0.4);
        QSettings().setValue(QStringLiteral("examen/fallo"), 0.1);
        auto ventana = std::make_unique<VentanaPrincipal>();
        ventana->show();
        QVERIFY(ventana->abrirBancoInicial(ruta));
        auto controles = [](VentanaPrincipal &v, QMap<QString, QRadioButton *> &formas, QDoubleSpinBox *&acierto, QDoubleSpinBox *&fallo) {
            for (GrupoPlegable *g : v.findChildren<GrupoPlegable *>())
                if (g->objectName() == QStringLiteral("plegablePuntuacion")) {
                    for (QRadioButton *r : g->findChildren<QRadioButton *>())
                        formas.insert(r->text(), r);
                    const QList<QDoubleSpinBox *> puntos = g->findChildren<QDoubleSpinBox *>();
                    acierto = puntos.value(0);
                    fallo = puntos.value(1);
                }
        };
        QMap<QString, QRadioButton *> formas;
        QDoubleSpinBox *acierto = nullptr, *fallo = nullptr;
        controles(*ventana, formas, acierto, fallo);
        QCOMPARE(formas.keys(), (QStringList{QStringLiteral("Cada fallo resta el 30 %"), QStringLiteral("Cada fallo resta el 33,3 %"),
                                             QStringLiteral("Los fallos no restan"), QStringLiteral("Personalizada")}));
        QVERIFY(acierto && fallo);
        QVERIFY(formas.value(QStringLiteral("Personalizada"))->isChecked());
        QVERIFY(acierto->isEnabled() && fallo->isEnabled());
        QCOMPARE(acierto->value(), 0.4);
        QCOMPARE(fallo->value(), 0.1);

        // Las otras tres, sobre 10 puntos (10 preguntas: un punto por acierto), sin poder cambiar los puntos.
        formas.value(QStringLiteral("Cada fallo resta el 30 %"))->click();
        QCOMPARE(acierto->value(), 1.0);
        QCOMPARE(fallo->value(), 0.3);
        QVERIFY(!acierto->isEnabled() && !fallo->isEnabled());
        formas.value(QStringLiteral("Cada fallo resta el 33,3 %"))->click();
        QCOMPARE(fallo->value(), 0.33);
        formas.value(QStringLiteral("Los fallos no restan"))->click();
        QCOMPARE(acierto->value(), 1.0);
        QCOMPARE(fallo->value(), 0.0);

        // Se guarda en el banco (y en la configuración), ya sin la casilla antigua.
        formas.value(QStringLiteral("Cada fallo resta el 33,3 %"))->click();
        ventana->close();
        ventana.reset();
        QVERIFY(!QSettings().contains(QStringLiteral("examen/puntuacionAuto")));
        QCOMPARE(QSettings().value(QStringLiteral("examen/puntuacion")).toString(), QStringLiteral("restaAlAzar"));
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta));
            QCOMPARE(bd.opciones().value(QStringLiteral("examen/puntuacion")).toString(), QStringLiteral("restaAlAzar"));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        formas.clear();
        controles(v, formas, acierto, fallo);
        QVERIFY(formas.value(QStringLiteral("Cada fallo resta el 33,3 %"))->isChecked());
        QCOMPARE(fallo->value(), 0.33);
        v.close();
        QSettings().clear();
    }

    void sinLimiteDePaginas()
    {
        // Preguntas tan largas que un examen de diez no cabe en una página ni con la maqueta más compacta.
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        {
            QVector<Pregunta> banco = bancoInventado();
            for (Pregunta &p : banco)
                p.enunciado = QStringLiteral("Una línea inventada.\n").repeated(12) + QStringLiteral("Pregunta:");
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta) && llenarBaseDatos(bd, 8, banco) && bd.guardarUltimoReparto({{1, 3}, {2, 2}, {3, 3}, {4, 2}}));
        }
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        auto *ajustar = conTexto<QCheckBox>(&v, QStringLiteral("Ajustar a una página por cada 10 preguntas"));
        QVERIFY(ajustar && ajustar->isChecked());
        QMetaObject::invokeMethod(&v, "generar");
        auto texto = [&v](const QString &parte) {
            for (QLabel *l : v.findChildren<QLabel *>())
                if (l->text().contains(parte) && l->isVisible())
                    return l->text();
            return QString();
        };
        QVERIFY(!texto(QStringLiteral("Algún examen no cabe en una página")).isEmpty());
        QVERIFY2(texto(QStringLiteral("Semilla:")).contains(QStringLiteral("Maqueta compacta")), qPrintable(texto(QStringLiteral("Semilla:"))));

        // Sin ajustar, la maqueta normal, ocupe lo que ocupe, y sin avisos.
        ajustar->setChecked(false);
        QVERIFY(texto(QStringLiteral("Algún examen no cabe")).isEmpty());
        QVERIFY(!texto(QStringLiteral("Semilla:")).contains(QStringLiteral("Maqueta compacta")));
        QVERIFY(texto(QStringLiteral("Semilla:")).contains(QStringLiteral("Sin límite de páginas")));
        v.close();
        BaseDatos bd;
        QVERIFY(bd.abrir(ruta));
        QCOMPARE(bd.opciones().value(QStringLiteral("examen/limitarPaginas")), QVariant(false));
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

    // En macOS, Qt pasa al menú de la aplicación lo que tenga estos papeles; por su texto solo los reconocería en inglés.
    void salirYAcercaDeVanAlMenuDeLaAplicacionEnMacOS()
    {
        VentanaPrincipal v;
        QHash<QString, QAction::MenuRole> papeles;
        for (QAction *menu : v.menuBar()->actions())
            for (QAction *a : menu->menu()->actions())
                papeles.insert(a->text(), a->menuRole());
        QCOMPARE(papeles.value(QStringLiteral("&Salir")), QAction::QuitRole);
        QCOMPARE(papeles.value(QStringLiteral("&Acerca de…")), QAction::AboutRole);
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
        auto *generar = conTexto<QPushButton>(&v, QStringLiteral("Generar exámenes"));
        QVERIFY(generar);
        for (QWidget *w = generar; w; w = w->parentWidget())
            QVERIFY(!qobject_cast<QScrollArea *>(w));
        generar->click();

        const QString carpeta = dir.filePath(QStringLiteral("exportados"));
        QDir().mkpath(carpeta);
        for (const char *slot : {"exportarPdf", "exportarSolucionesPdf", "exportarHtml", "exportarLatex", "exportarDocx"}) {
            QTimer::singleShot(100, [&carpeta] {
                QWidget *d = QApplication::activeModalWidget();
                if (!d)
                    return;
                d->findChild<QLineEdit *>()->setText(carpeta);
                pulsar(d, QStringLiteral("Exportar"));
            });
            QVERIFY(QMetaObject::invokeMethod(&v, slot));
        }
        // HTML con soluciones: con la casilla del diálogo (sin «Mostrar soluciones»), y «-soluciones» en el nombre.
        QTimer::singleShot(100, [&carpeta] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            d->findChild<QLineEdit *>()->setText(carpeta);
            d->findChild<QCheckBox *>()->setChecked(true);
            pulsar(d, QStringLiteral("Exportar"));
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "exportarHtml"));
        // Tras exportar, la barra de estado ofrece abrir la carpeta.
        auto *abrir = conTexto<QPushButton>(&v, QStringLiteral("Abrir la carpeta"));
        QVERIFY(abrir);
        QVERIFY(abrir->isVisible());
        QCOMPARE(abrir->toolTip(), QDir::toNativeSeparators(carpeta));
        QFile conSoluciones(carpeta + QStringLiteral("/") + QDir(carpeta).entryList({QStringLiteral("*-soluciones.html")}).value(0));
        QVERIFY(conSoluciones.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(conSoluciones.readAll()).contains(QStringLiteral("CON SOLUCIONES")));
        // Con «Letra grande», los ficheros llevan «-letra-grande» en el nombre.
        auto *letraGrande = v.findChild<QCheckBox *>(QString(), Qt::FindChildrenRecursively);
        letraGrande = conTexto<QCheckBox>(&v, QStringLiteral("Letra grande"));
        QCOMPARE(letraGrande->text(), QStringLiteral("Letra grande"));
        letraGrande->setChecked(true);
        QTimer::singleShot(100, [&carpeta] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            d->findChild<QLineEdit *>()->setText(carpeta);
            pulsar(d, QStringLiteral("Exportar"));
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "exportarPdf"));
        // Por omisión, un fichero por examen (Nacional Original y Nacional Reserva) de cada tipo.
        const QStringList ficheros = QDir(carpeta).entryList(QDir::Files);
        QCOMPARE(ficheros.filter(QStringLiteral(".pdf")).size(), 6);
        QCOMPARE(ficheros.filter(QStringLiteral("-letra-grande-")).size(), 2);
        QCOMPARE(ficheros.filter(QStringLiteral("-soluciones.")).size(), 4);
        // El de con soluciones se llama como el de sin ellas con «-soluciones» al final, para que estén juntos en la
        // carpeta sin pisarse, y las palabras van separadas por guiones.
        for (const QString &f : ficheros.filter(QStringLiteral("-soluciones.")))
            QVERIFY2(ficheros.contains(QString(f).remove(QStringLiteral("-soluciones"))), qPrintable(f));
        QVERIFY2(ficheros.filter(QStringLiteral("_")).isEmpty(), qPrintable(ficheros.join(QStringLiteral(", "))));
        QCOMPARE(ficheros.filter(QStringLiteral(".html")).size(), 4);
        QCOMPARE(ficheros.filter(QStringLiteral(".tex")).size(), 2);
        QCOMPARE(ficheros.filter(QStringLiteral(".docx")).size(), 2);
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
            pulsar(d, QStringLiteral("Exportar"));
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
            if (t->toPlainText().contains(QStringLiteral("Respuesta correcta: +")))
                visor = t;
        QVERIFY(visor);
        const QString antes = visor->toPlainText();
        QSpinBox *numero = nullptr;
        for (QSpinBox *s : v.findChildren<QSpinBox *>())
            if (s->toolTip().startsWith(QStringLiteral("Número de la pregunta que se quiere editar o cambiar")))
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
        bool editarla = false;
        QTimer::singleShot(100, [&opcion, &editarla] {
            for (QWidget *w : QApplication::topLevelWidgets()) {
                auto *menu = qobject_cast<QMenu *>(w);
                if (!menu || !menu->isVisible())
                    continue;
                for (QAction *a : menu->actions())
                    editarla = editarla || a->text().startsWith(QStringLiteral("Editar la pregunta 5 (T"));
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
        QVERIFY(editarla);
        QVERIFY(visor->toPlainText() != antes);
        QCOMPARE(numero->value(), 5);
    }

    void parteIzquierdaSinCortes()
    {
        // Con la letra grande (la del escritorio puede serlo), lo de la izquierda se ve entero sin redimensionar ni
        // desplazarse en horizontal.
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        QSettings().clear();
        const QFont antes = QApplication::font();
        QApplication::setFont(QFont(antes.family(), 14));
        {
            VentanaPrincipal v;
            v.resize(1000, 700);
            v.show();
            QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
            auto *zona = v.findChild<QScrollArea *>();
            QVERIFY(zona && zona->widget());
            // Los botones de los repartos se crean al abrir el banco y se muestran un momento después.
            const QList<QPushButton *> botones = zona->widget()->findChildren<QPushButton *>();
            QVERIFY(botones.size() >= 6);   // Añadir, Quitar, Todos, Ninguno, los repartos y Guardar reparto…
            QTRY_VERIFY(std::none_of(botones.begin(), botones.end(), [](QPushButton *b) { return b->isHidden(); }));
            QTRY_VERIFY(zona->viewport()->width() >= zona->widget()->minimumSizeHint().width());
            QVERIFY(!zona->horizontalScrollBar()->isVisible());
            for (QPushButton *b : botones) {
                const QRect r(b->mapTo(zona->viewport(), QPoint(0, 0)), b->size());
                QVERIFY2(r.left() >= 0 && r.right() < zona->viewport()->width(), qPrintable(b->text()));
            }
        }
        QApplication::setFont(antes);
        QSettings().clear();
    }

    void ventanaEditaUnaPreguntaDelExamen()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        QMetaObject::invokeMethod(&v, "generar");
        auto *visor = v.findChild<VisorAmpliable *>(QStringLiteral("visorExamen"));
        QSpinBox *numero = nullptr;
        for (QSpinBox *s : v.findChildren<QSpinBox *>())
            if (s->toolTip().startsWith(QStringLiteral("Número de la pregunta que se quiere editar o cambiar")))
                numero = s;
        QPushButton *editar = nullptr, *cambiar = nullptr, *deshacer = nullptr;
        for (QPushButton *b : v.findChildren<QPushButton *>()) {
            if (b->text() == QStringLiteral("Editar…"))
                editar = b;
            else if (b->text() == QStringLiteral("Cambiar por otra"))
                cambiar = b;
            else if (b->text() == QStringLiteral("Deshacer el cambio"))
                deshacer = b;
        }
        QVERIFY(visor && numero && editar && cambiar && deshacer);
        QVERIFY(editar->isEnabled());

        // Se cambia la pregunta 3 por otra, para ver después que al deshacerlo no vuelve la 2 sin corregir.
        numero->setValue(3);
        cambiar->click();
        QVERIFY(deshacer->isEnabled());

        // Se edita la 2: se guarda en el banco y se ve en el examen.
        numero->setValue(2);
        const QString enunciado = QStringLiteral("Enunciado inventado y corregido desde el examen:");
        QString codigo;
        QTimer::singleShot(100, [&] {
            auto *d = qobject_cast<DialogoPregunta *>(QApplication::activeModalWidget());
            if (!d)
                return;
            codigo = d->pregunta().codigoCompleto();
            d->findChild<QPlainTextEdit *>()->setPlainText(enunciado);
            responder(QStringLiteral("Guardar así"));   // por si avisa de alguna pauta
            pulsar(d, QStringLiteral("Guardar"));
        });
        editar->click();
        QVERIFY(!codigo.isEmpty());
        QVERIFY(visor->toPlainText().contains(enunciado));
        QCOMPARE(numero->value(), 2);
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta));
            QString enBanco;
            for (const Pregunta &p : bd.preguntas())
                if (p.codigoCompleto() == codigo)
                    enBanco = p.enunciado;
            QCOMPARE(enBanco, enunciado);
        }
        deshacer->click();
        QVERIFY(!deshacer->isEnabled());
        QVERIFY(visor->toPlainText().contains(enunciado));

        // Si se cancela, no cambia nada.
        const QString antes = visor->toPlainText();
        QTimer::singleShot(100, [] {
            if (auto *d = qobject_cast<DialogoPregunta *>(QApplication::activeModalWidget()))
                d->reject();
        });
        editar->click();
        QCOMPARE(visor->toPlainText(), antes);
        QSettings().clear();
    }

    void panelDeVerificacion()
    {
        // Pocos clics: al marcar «Verificada por humanos» se marcan los de la última; al marcar a alguien, también la
        // casilla; al desmarcarla, nadie, y si se vuelve a marcar, los que había.
        QTemporaryDir dir;
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        for (const char *nombre : {"Persona Uno", "Persona Dos", "Persona Tres"})
            QVERIFY(bd.anadirVerificador(QString::fromUtf8(nombre)));
        QVERIFY(bd.guardarUltimosVerificadores({QStringLiteral("Persona Dos"), QStringLiteral("Persona Tres")}));
        PanelVerificacion panel(&bd);
        QMap<QString, QCheckBox *> casillas;
        for (QCheckBox *c : panel.findChildren<QCheckBox *>())
            casillas.insert(c->text(), c);
        QCOMPARE(casillas.size(), 4);
        QCheckBox *verificada = casillas.value(QStringLiteral("Verificada por humanos"));
        QVERIFY(verificada);
        Pregunta p;
        panel.ponerPregunta(p);
        QVERIFY(panel.sinCambios());
        verificada->setChecked(true);
        QCOMPARE(panel.verificadores(), (QStringList{QStringLiteral("Persona Dos"), QStringLiteral("Persona Tres")}));
        QVERIFY(!panel.sinCambios());
        casillas.value(QStringLiteral("Persona Tres"))->setChecked(false);
        verificada->setChecked(false);
        QVERIFY(panel.verificadores().isEmpty());
        verificada->setChecked(true);
        QCOMPARE(panel.verificadores(), QStringList{QStringLiteral("Persona Dos")});   // los de antes de desmarcarla
        // Al marcar a alguien en una sin verificar, queda verificada (solo por esa persona).
        panel.ponerPregunta(p);
        casillas.value(QStringLiteral("Persona Uno"))->setChecked(true);
        QVERIFY(verificada->isChecked());
        QCOMPARE(panel.verificadores(), QStringList{QStringLiteral("Persona Uno")});
        panel.recordar();
        QCOMPARE(bd.ultimosVerificadores(), QStringList{QStringLiteral("Persona Uno")});
        // Una verificada muestra quién la verificó, y sin cambios no hay nada que guardar.
        p.verificada = true;
        p.verificadores = {QStringLiteral("Persona Tres")};
        panel.ponerPregunta(p);
        QVERIFY(panel.sinCambios());
        QCOMPARE(panel.verificadores(), QStringList{QStringLiteral("Persona Tres")});
        panel.marcarVerificada();   // ya lo está: no cambia
        QVERIFY(panel.sinCambios());
    }

    void dialogoDeVerificadores()
    {
        QTemporaryDir dir;
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        DialogoVerificadores d(&bd);
        d.show();
        auto boton = [&d](const QString &texto) {
            for (QPushButton *b : d.findChildren<QPushButton *>())
                if (b->text() == texto)
                    return b;
            return static_cast<QPushButton *>(nullptr);
        };
        auto escribir = [](const QString &texto) {
            QTimer::singleShot(100, [texto] {
                if (auto *e = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
                    e->setTextValue(texto);
                    e->accept();
                }
            });
        };
        auto *lista = d.findChild<QListWidget *>();
        escribir(QStringLiteral("Persona Nueva"));
        boton(QStringLiteral("Añadir…"))->click();
        QCOMPARE(bd.verificadores(), QStringList{QStringLiteral("Persona Nueva")});
        QCOMPARE(lista->count(), 1);
        QVERIFY(d.hayCambios());
        escribir(QStringLiteral("Persona Renombrada"));
        boton(QStringLiteral("Cambiar el nombre…"))->click();
        QCOMPARE(bd.verificadores(), QStringList{QStringLiteral("Persona Renombrada")});
        QCOMPARE(d.renombrados().value(QStringLiteral("Persona Nueva")), QStringLiteral("Persona Renombrada"));
        responder(QStringLiteral("Sí"));
        boton(QStringLiteral("Quitar"))->click();
        QVERIFY(bd.verificadores().isEmpty());
        QCOMPARE(lista->count(), 0);
        QVERIFY(!boton(QStringLiteral("Quitar"))->isEnabled());
    }

    void ventanaVerificaPreguntas()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta));
            QVERIFY(bd.anadirVerificador(QStringLiteral("Persona Uno")));
            QVERIFY(bd.anadirVerificador(QStringLiteral("Otra Persona")));
        }
        QSettings().clear();
        // Sin los avisos de atención (los prueba verificarAvisaDeLaPrisaYDeLasPausas): aquí se pulsa sin esperar.
        QSettings().setValue(QStringLiteral("verificar/avisos"), false);
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        auto verificadas = [&ruta]() {
            BaseDatos bd;
            QStringList res;
            if (bd.abrir(ruta))
                for (const Pregunta &p : bd.preguntas())
                    if (p.verificada)
                        res << p.codigoCompleto() + QLatin1Char(':') + p.verificadores.join(QLatin1Char(','));
            return res;
        };
        auto pulsar = [](QWidget *w, const QString &texto) {
            for (QPushButton *b : w->findChildren<QPushButton *>())
                if (b->text() == texto && b->isEnabled() && b->isVisible()) {
                    b->click();
                    return true;
                }
            return false;
        };
        auto *tabla = v.findChild<QTableView *>();
        tabla->setCurrentIndex(tabla->model()->index(0, 0));

        // Junto a los filtros, cuántas preguntas hay en la lista y cuántas de ellas están verificadas.
        auto *cuenta = v.findChild<QLabel *>(QStringLiteral("cuentaBanco"));
        QVERIFY(cuenta);
        auto cuentaEsperada = [&] {
            const int n = tabla->model()->rowCount();
            int si = 0;
            for (int f = 0; f < n; ++f)
                si += tabla->model()->index(f, ModeloPreguntas::ColVerificada).data().toString().startsWith(QChar(0x2713)) ? 1 : 0;
            return QStringLiteral("%1 preguntas: %2 verificadas, %3 sin verificar").arg(n).arg(si).arg(n - si);
        };
        QVERIFY(tabla->model()->rowCount() > 3);
        QCOMPARE(cuenta->text(), cuentaEsperada());
        const QString cuentaAlEmpezar = cuenta->text();
        QString cuentaConElDialogo, progresoAlAbrir, progresoDespues;
        auto *barra = v.findChild<QProgressBar *>(QStringLiteral("barraBanco"));
        QVERIFY(barra);
        QCOMPARE(barra->maximum(), tabla->model()->rowCount());
        QCOMPARE(barra->value(), 0);
        int barraDelDialogo = -1;

        // Desde la lista: la primera vez hay que decir quién; después, «Verificada, siguiente» basta (Intro).
        QString aviso, posicion, enLaLista;
        bool ampliada = false;
        QTimer::singleShot(100, [&] {
            auto *d = qobject_cast<DialogoVerificar *>(QApplication::activeModalWidget());
            if (!d)
                return;
            auto *progreso = d->findChild<QLabel *>(QStringLiteral("progresoVerificar"));
            progresoAlAbrir = progreso->text();
            // La ficha se amplía (Ctrl++), y las preguntas siguientes salen igual de grandes.
            auto *ficha = d->findChild<VisorAmpliable *>();
            if (ficha)
                ficha->ampliar();
            pulsar(d, QStringLiteral("&Verificada, siguiente"));
            for (QLabel *l : d->findChildren<QLabel *>())
                if (l->isVisible() && l->text().startsWith(QStringLiteral("Marque")))
                    aviso = l->text();
            marcar(d, QStringLiteral("Persona Uno"));
            pulsar(d, QStringLiteral("&Verificada, siguiente"));
            // Se ve ya en la lista, con el diálogo abierto.
            enLaLista = tabla->model()->index(0, ModeloPreguntas::ColVerificada).data().toString();
            cuentaConElDialogo = cuenta->text();
            progresoDespues = progreso->text();
            barraDelDialogo = d->findChild<QProgressBar *>()->value();
            QTest::keyClick(d, Qt::Key_Return);
            for (QLabel *l : d->findChildren<QLabel *>())
                if (l->text().startsWith(QStringLiteral("Pregunta ")))
                    posicion = l->text();
            if (ficha) {
                const QVector<qreal> grande = tamanos(ficha->document());
                const qreal escala = ficha->escala();
                ficha->tamanoNormal();
                ampliada = escala == 1.1 && !grande.isEmpty() && proporcionales(grande, tamanos(ficha->document()), 1.1);
            }
            d->reject();
        });
        QVERIFY(QMetaObject::invokeMethod(v.findChild<PestanaBanco *>(), "verificarPregunta"));
        QVERIFY(ampliada);
        QCOMPARE(enLaLista, QStringLiteral("✓ Persona Uno"));
        QVERIFY(cuentaConElDialogo != cuentaAlEmpezar);
        // El avance, en el diálogo (sobre las preguntas que se recorren) y en la barra del banco.
        QCOMPARE(progresoAlAbrir, QStringLiteral("Verificadas: 0 de %1").arg(tabla->model()->rowCount()));
        QCOMPARE(progresoDespues, QStringLiteral("Verificadas: 1 de %1").arg(tabla->model()->rowCount()));
        QCOMPARE(barraDelDialogo, 1);
        QCOMPARE(barra->value(), 2);
        QVERIFY2(cuentaConElDialogo.contains(QStringLiteral(": 1 verificadas,")), qPrintable(cuentaConElDialogo));
        QCOMPARE(cuenta->text(), cuentaEsperada());
        QVERIFY2(cuenta->text().contains(QStringLiteral(": 2 verificadas,")), qPrintable(cuenta->text()));
        QCOMPARE(aviso, QStringLiteral("Marque quién la ha verificado."));
        QVERIFY2(posicion.startsWith(QStringLiteral("Pregunta 3 de ")), qPrintable(posicion));
        QCOMPARE(verificadas(), (QStringList{QStringLiteral("T1-1:Persona Uno"), QStringLiteral("T1-2:Persona Uno")}));
        // En la lista se ve, y la selección se queda en la última que se veía.
        QCOMPARE(tabla->model()->index(0, ModeloPreguntas::ColVerificada).data().toString(), QStringLiteral("✓ Persona Uno"));
        QCOMPARE(tabla->currentIndex().row(), 2);

        // En el editor: al marcar «Verificada por humanos», los de la última.
        QTimer::singleShot(100, [&] {
            auto *d = qobject_cast<DialogoPregunta *>(QApplication::activeModalWidget());
            if (!d)
                return;
            marcar(d, QStringLiteral("Verificada por humanos"));
            responder(QStringLiteral("Guardar así"));
            pulsar(d, QStringLiteral("Guardar"));
        });
        QVERIFY(QMetaObject::invokeMethod(v.findChild<PestanaBanco *>(), "editarPregunta"));
        QCOMPARE(verificadas().size(), 3);
        QVERIFY(verificadas().contains(QStringLiteral("T1-3:Persona Uno")));

        // Todas las del tema 1, de una vez (las de la lista con su filtro), por las dos personas.
        QComboBox *filtroTema = nullptr;
        for (QComboBox *c : v.findChildren<QComboBox *>())
            if (c->itemText(0) == QStringLiteral("Todos los temas"))
                filtroTema = c;
        QVERIFY(filtroTema);
        filtroTema->setCurrentIndex(1);
        const int delTema1 = tabla->model()->rowCount();
        QTimer::singleShot(100, [&] {
            QWidget *d = QApplication::activeModalWidget();
            if (!d)
                return;
            for (QCheckBox *c : d->findChildren<QCheckBox *>())
                c->setChecked(true);
            pulsar(d, QStringLiteral("Marcar como verificadas"));
        });
        QVERIFY(QMetaObject::invokeMethod(v.findChild<PestanaBanco *>(), "verificarLista"));
        QCOMPARE(verificadas().size(), delTema1);
        QVERIFY(verificadas().contains(QStringLiteral("T1-1:Otra Persona,Persona Uno")));
        filtroTema->setCurrentIndex(0);

        // Al generar solo con las verificadas, solo salen las del tema 1, y la vista lo marca (no los ficheros).
        auto *soloVerificadas = conTexto<QCheckBox>(&v, QStringLiteral("Solo preguntas verificadas por humanos"));
        QVERIFY(soloVerificadas);
        QVERIFY(!soloVerificadas->isChecked());
        soloVerificadas->setChecked(true);
        QMetaObject::invokeMethod(&v, "generar");
        QTextBrowser *visor = nullptr;
        for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
            if (t->toPlainText().contains(QStringLiteral("Respuesta correcta: +")))
                visor = t;
        QVERIFY(visor);
        QVERIFY(visor->toPlainText().contains(QStringLiteral("verificada: Otra Persona, Persona Uno")));
        QVERIFY(!visor->toPlainText().contains(QStringLiteral("sin verificar")));
        QCOMPARE(visor->toPlainText().count(QStringLiteral("verificada: ")), 3);   // las 3 del tema 1
        soloVerificadas->setChecked(false);
        QMetaObject::invokeMethod(&v, "generar");
        QVERIFY(visor->toPlainText().contains(QStringLiteral("sin verificar")));

        // Desde el examen: se recorren sus preguntas y la vista se pone al día.
        int alEmpezar = 0, conElDialogoAbierto = -1;
        bool deUnaEnUna = true;
        QTimer::singleShot(100, [&] {
            auto *d = qobject_cast<DialogoVerificar *>(QApplication::activeModalWidget());
            if (!d)
                return;
            // Cada una se ve ya en el examen, con el diálogo abierto: una menos sin verificar cada vez, o las mismas
            // si ya lo estaba.
            int sinVerificar = int(visor->toPlainText().count(QStringLiteral("sin verificar")));
            alEmpezar = sinVerificar;
            for (int i = 0; i < 20 && pulsar(d, QStringLiteral("&Verificada, siguiente")); ++i) {
                const int ahora = int(visor->toPlainText().count(QStringLiteral("sin verificar")));
                deUnaEnUna = deUnaEnUna && (ahora == sinVerificar || ahora == sinVerificar - 1);
                sinVerificar = ahora;
            }
            pulsar(d, QStringLiteral("&Verificada"));
            conElDialogoAbierto = int(visor->toPlainText().count(QStringLiteral("sin verificar")));
            d->reject();
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "verificarPreguntaExamen"));
        QVERIFY2(alEmpezar > 1, qPrintable(QString::number(alEmpezar)));
        QVERIFY(deUnaEnUna);
        QCOMPARE(conElDialogoAbierto, 0);
        QVERIFY2(!visor->toPlainText().contains(QStringLiteral("sin verificar")), qPrintable(visor->toPlainText()));
        QSettings().clear();
    }

    // Un examen del historial se ve con la verificación de ahora, aunque se verificara después de registrarlo; salvo la
    // pregunta que se ha corregido desde entonces, porque lo verificado es otro texto.
    void historialConLaVerificacionDelBanco()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        const QString anadido = QStringLiteral(" Texto añadido después de registrar.");
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta));
            QVERIFY(bd.anadirVerificador(QStringLiteral("Persona Uno")));
            const QVector<Pregunta> banco = bd.preguntas();
            Modelo modelo;
            modelo.nombre = QStringLiteral("Uno");
            for (int i = 0; i < 3; ++i) {
                PreguntaExamen pe;
                pe.pregunta = banco.at(i);
                modelo.preguntas << pe;
            }
            DatosConvocatoria datos;
            datos.convocatoria = QStringLiteral("Junio");
            datos.anio = 2026;
            QVERIFY(bd.registrarExamenes({modelo}, datos, 1));
            QVERIFY(bd.guardarVerificacion(banco.at(0).id, true, {QStringLiteral("Persona Uno")}));
            Pregunta corregida = banco.at(1);
            corregida.enunciado += anadido;
            corregida.verificada = true;
            corregida.verificadores = {QStringLiteral("Persona Uno")};
            QVERIFY(bd.guardarPregunta(corregida));
        }
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        QTimer::singleShot(100, [] {
            auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!d)
                return;
            auto *arbol = d->findChild<QTreeWidget *>();
            arbol->setCurrentItem(arbol->topLevelItem(0));
            d->accept();
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "historialConvocatorias"));
        QTextBrowser *visor = nullptr;
        for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
            if (t->toPlainText().contains(QStringLiteral("Respuesta correcta: +")))
                visor = t;
        QVERIFY(visor);
        const QString examen = visor->toPlainText();
        QCOMPARE(examen.count(QStringLiteral("verificada: Persona Uno")), 1);
        QCOMPARE(examen.count(QStringLiteral("sin verificar")), 2);
        QVERIFY(!examen.contains(anadido));   // sigue tal como salió
        QSettings().clear();
    }

    // En el historial, «Cargar» es el botón por defecto (el de Intro) al abrirlo y después de eliminar algo, que pide
    // confirmación; y Cargar, Cerrar y Ayuda tienen cada uno su atajo (Alt y una letra), salvo en macOS.
    void historialConAtajosYCargarPorDefecto()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        BaseDatos bd;
        QVERIFY(bd.abrir(ruta));
        const QVector<Pregunta> banco = bd.preguntas();
        DatosConvocatoria datos;
        datos.anio = 2026;
        for (const QString &convocatoria : {QStringLiteral("Junio"), QStringLiteral("Septiembre")}) {
            Modelo modelo;
            modelo.nombre = QStringLiteral("Uno");
            PreguntaExamen pe;
            pe.pregunta = banco.at(0);
            modelo.preguntas << pe;
            datos.convocatoria = convocatoria;
            QVERIFY(bd.registrarExamenes({modelo}, datos, 1));
        }
        DialogoHistorial d(&bd);
        d.show();
        QVERIFY(QTest::qWaitForWindowActive(&d));
        auto *arbol = d.findChild<QTreeWidget *>();
        QPushButton *cargar = nullptr, *cerrar = nullptr, *ayuda = nullptr, *eliminar = nullptr;
        for (QPushButton *b : d.findChildren<QPushButton *>()) {
            const QString texto = b->text().remove(QLatin1Char('&'));
            if (texto == QStringLiteral("Cargar"))
                cargar = b;
            else if (texto == QStringLiteral("Cerrar"))
                cerrar = b;
            else if (texto == QStringLiteral("Ayuda"))
                ayuda = b;
            else if (texto == QStringLiteral("Eliminar del historial…"))
                eliminar = b;
        }
        QVERIFY(arbol && cargar && cerrar && ayuda && eliminar);
        QVERIFY(cargar->isDefault());
        QVERIFY(!eliminar->isDefault());

        // Sin nada elegido a la vista, ni Intro carga ni «Eliminar del historial…» borra, aunque el árbol, al recibir el
        // foco, haya hecho actual la primera convocatoria: dicen que se elija una.
        QVERIFY(arbol->selectedItems().isEmpty());
        for (QPushButton *b : {cargar, eliminar}) {
            QString aviso;
            responder(QStringLiteral("OK"), &aviso);
            if (b == cargar)
                QTest::keyClick(arbol, Qt::Key_Return);
            else
                b->click();
            QCOMPARE(aviso, QStringLiteral("Elija una convocatoria o un examen."));
            QVERIFY(d.isVisible());
            QCOMPARE(bd.examenesRegistrados().size(), 2);
        }
        QCOMPARE(d.focusWidget(), arbol);

        if (!QKeySequence::mnemonic(QStringLiteral("&a")).isEmpty()) {
            QSet<QString> atajos;
            for (QPushButton *b : {cargar, cerrar, ayuda}) {
                const QString atajo = b->shortcut().toString();
                QVERIFY2(atajo.startsWith(QStringLiteral("Alt+")), qPrintable(b->text()));
                QVERIFY2(!atajos.contains(atajo), qPrintable(atajo));
                atajos << atajo;
            }
        }

        // Eliminar pide confirmación: con «No» no se borra; con «Sí», sí. Y después Cargar vuelve a ser el botón por
        // defecto, aunque el foco se lo llevara «Eliminar del historial…» al pulsarlo.
        for (const QString &respuesta : {QStringLiteral("No"), QStringLiteral("Sí")}) {
            arbol->setCurrentItem(arbol->topLevelItem(0));
            eliminar->setFocus(Qt::MouseFocusReason);
            QString confirmacion;
            responder(respuesta, &confirmacion);
            eliminar->click();
            QVERIFY2(confirmacion.startsWith(QStringLiteral("¿Eliminar del historial")), qPrintable(confirmacion));
            QCOMPARE(bd.examenesRegistrados().size(), respuesta == QStringLiteral("No") ? 2 : 1);
            QTRY_VERIFY(cargar->isDefault());
            QVERIFY(!eliminar->isDefault());
        }

        // Intro carga la convocatoria elegida.
        QCOMPARE(arbol->topLevelItemCount(), 1);
        arbol->setCurrentItem(arbol->topLevelItem(0));
        QCOMPARE(d.focusWidget(), arbol);
        QTest::keyClick(arbol, Qt::Key_Return);
        QCOMPARE(d.result(), int(QDialog::Accepted));
        QCOMPARE(d.examenesElegidos().size(), 1);
    }

    // Al generar, la ventana tiene en cuenta cómo salieron las preguntas en el historial: las que ya salieron llevan las
    // opciones barajadas, aunque no esté marcado «Barajar las opciones», con la correcta en otra letra.
    void ventanaBarajaLasQueYaSalieron()
    {
        QSettings().clear();
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        {
            // Un examen anterior con todas las preguntas de los temas 1 a 4, con las opciones como en el banco.
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta));
            Modelo anterior;
            anterior.nombre = QStringLiteral("Anterior");
            for (const Pregunta &p : bd.preguntas())
                if (p.temaNumero <= 4) {
                    PreguntaExamen pe;
                    pe.pregunta = p;
                    anterior.preguntas << pe;
                }
            DatosConvocatoria d;
            d.convocatoria = QStringLiteral("Junio");
            d.anio = 2020;
            QVERIFY(bd.registrarExamenes({anterior}, d, 1));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        QVERIFY(marcar(&v, QStringLiteral("Barajar las opciones (la correcta, repartida entre a)–d))"), false));
        QVERIFY(pulsar(&v, QStringLiteral("Generar exámenes")));
        responder(QStringLiteral("Sí"));
        QVERIFY(QMetaObject::invokeMethod(&v, "registrarConvocatoria"));
        v.close();

        BaseDatos bd;
        QVERIFY(bd.abrir(ruta));
        int nuevas = 0;
        for (const ExamenRegistrado &e : bd.examenesRegistrados()) {
            if (e.nombre == QStringLiteral("Anterior"))
                continue;
            for (const PreguntaExamen &pe : bd.modeloRegistrado(e.id).preguntas) {
                ++nuevas;
                if (!Estilo::tieneOpcionesDependientes(pe.pregunta))
                    QVERIFY2(pe.correcta() != pe.pregunta.correcta, qPrintable(e.nombre + QLatin1Char(' ') + pe.pregunta.codigoCompleto()));
            }
        }
        QVERIFY(nuevas >= 10);
        QSettings().clear();
    }

    // Después de quitar un examen (Registrar exámenes anteriores) o un tema (Temas), el foco vuelve a la lista: si se
    // quedara en el botón, Intro quitaría otro en vez de pulsar el botón por defecto.
    void quitarDevuelveIntroAlBotonPorDefecto()
    {
        QTemporaryDir dir;
        BaseDatos bd;
        QVERIFY(bd.abrir(dir.filePath(QStringLiteral("banco.db"))));
        QVERIFY(llenarBaseDatos(bd, 4, bancoVariado()));
        const QVector<Pregunta> banco = bd.preguntas();
        {
            QString texto = QStringLiteral("Examen inventado\n");
            for (int i = 0; i < 3; ++i) {
                texto += QStringLiteral("%1.- %2\n").arg(i + 1).arg(banco.at(i).enunciado);
                for (int k = 0; k < 4; ++k)
                    texto += QStringLiteral("%1) %2\n").arg(Pregunta::letra(k)).arg(banco.at(i).opciones.at(k));
            }
            DialogoAnteriores d(&bd, banco, DatosConvocatoria(), {});
            d.show();
            QVERIFY(QTest::qWaitForWindowActive(&d));
            QVERIFY(d.anadirExamen(texto));
            QVERIFY(d.anadirExamen(texto));
            QTableWidget *lista = nullptr;   // la de los exámenes (la otra es la de las preguntas del elegido)
            for (QTableWidget *t : d.findChildren<QTableWidget *>())
                if (t->rowCount() == 2)
                    lista = t;
            QVERIFY(lista);
            lista->selectRow(0);
            QPushButton *quitar = conTexto<QPushButton>(&d, QStringLiteral("Quitar"));
            QVERIFY(quitar);
            quitar->setFocus(Qt::MouseFocusReason);
            QCOMPARE(botonDeIntro(&d), QStringLiteral("Quitar"));   // mientras tiene el foco, Intro es él
            quitar->click();
            QCOMPARE(lista->rowCount(), 1);
            QCOMPARE(d.focusWidget(), lista);
            QCOMPARE(botonDeIntro(&d), QStringLiteral("Registrar"));
        }
        {
            DialogoTemas d(&bd);
            d.show();
            QVERIFY(QTest::qWaitForWindowActive(&d));
            auto *tabla = d.findChild<QTableWidget *>();
            const int temas = tabla->rowCount();
            QVERIFY(pulsar(&d, QStringLiteral("Añadir tema")));
            QCOMPARE(tabla->rowCount(), temas + 1);
            tabla->setCurrentCell(temas, 0);   // el nuevo, sin preguntas: se quita sin preguntar
            QPushButton *eliminar = conTexto<QPushButton>(&d, QStringLiteral("Eliminar tema"));
            QVERIFY(eliminar);
            eliminar->setFocus(Qt::MouseFocusReason);
            QCOMPARE(botonDeIntro(&d), QStringLiteral("Eliminar tema"));
            eliminar->click();
            QCOMPARE(tabla->rowCount(), temas);
            QCOMPARE(d.focusWidget(), tabla);
            QCOMPARE(botonDeIntro(&d), QStringLiteral("Guardar"));
        }
    }

    // En cada diálogo, Intro pulsa el botón que debe, al abrirlo y después de que el foco pase por los otros botones.
    // Si el botón por defecto se marca antes de meter los botones en el diálogo, Qt no se entera y le da Intro al
    // primer botón que encuentra (en Exportar, «Elegir…»; en Temas, «Añadir tema»).
    void botonDeIntroEnLosDialogos()
    {
        QSettings().clear();
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        BaseDatos bd;
        QVERIFY(bd.abrir(ruta));
        QVERIFY(bd.anadirVerificador(QStringLiteral("Persona Uno")));
        const QVector<Pregunta> banco = bd.preguntas();
        const QVector<Tema> temas = bd.temas();
        DatosConvocatoria datos;
        datos.asignatura = QStringLiteral("Asignatura inventada");
        datos.convocatoria = QStringLiteral("Junio");
        datos.anio = 2026;
        Modelo modelo;
        modelo.nombre = QStringLiteral("Uno");
        for (int i = 0; i < 3; ++i) {
            PreguntaExamen pe;
            pe.pregunta = banco.at(i);
            modelo.preguntas << pe;
        }
        QVERIFY(bd.registrarExamenes({modelo}, datos, 1));
        const QString json = dir.filePath(QStringLiteral("nuevas.json"));
        {
            QFile f(json);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("{\"temas\": [{\"numero\": 1, \"titulo\": \"Inventado\", \"preguntas\": [\n"
                    "{\"numero\": 1, \"enunciado\": \"Inventado:\", \"opciones\": [\"Uno.\", \"Dos.\", \"Tres.\", "
                    "\"Cuatro.\"], \"correcta\": \"c\"}]}]}");
        }
        QVector<Parecidas::Parecida> parecidas;
        parecidas.append({banco.at(1), 0.9});
        QCOMPARE(banco.at(1).temaNumero, banco.at(0).temaNumero);   // «Guardar como variante» está activo

        // Se apuntan los que fallan, para verlos todos de una vez.
        QStringList problemas;
        auto comprobar = [&problemas](QDialog &d, const QString &esperado) {
            const QString problema = problemaConElBotonDeIntro(&d, esperado);
            if (!problema.isEmpty())
                problemas << problema;
            d.hide();
        };
        {
            DialogoHistorial d(&bd);
            comprobar(d, QStringLiteral("&Cargar"));
        }
        {
            DialogoAnteriores d(&bd, banco, datos, {});
            comprobar(d, QStringLiteral("Registrar"));
        }
        {
            DialogoExportar d(QStringLiteral("Exportar"), {QStringLiteral("Uno"), QStringLiteral("Dos")},
                              QStringLiteral("examen"), QStringLiteral("pdf"), true);
            comprobar(d, QStringLiteral("Exportar"));
        }
        {
            DialogoMoodle d(banco, datos.asignatura);
            comprobar(d, QStringLiteral("Exportar"));
        }
        {
            DialogoPregunta d(temas, banco, banco.at(0));
            comprobar(d, QStringLiteral("Guardar"));
        }
        {
            DialogoTemas d(&bd);
            comprobar(d, QStringLiteral("Guardar"));
        }
        {
            DialogoComprobacion d(json, {}, DialogoComprobacion::Informe);
            comprobar(d, QStringLiteral("Cerrar"));
        }
        {
            DialogoComprobacion d(json, {}, DialogoComprobacion::AntesDeImportar);
            comprobar(d, QStringLiteral("Importar"));
        }
        {
            DialogoEstilo d([&banco] { return banco; }, [](int) { return false; }, QStringLiteral("banco"), temas);
            comprobar(d, QStringLiteral("Editar la pregunta…"));
        }
        {
            DialogoParecidas d(banco.at(0), parecidas, temas);
            comprobar(d, QStringLiteral("Seguir editando"));
        }
        {
            DialogoPractica d(modelo, datos, 60);
            comprobar(d, QStringLiteral("Empezar"));
        }
        {
            DialogoVerificar d(&bd, banco.mid(0, 3), 0, temas);
            comprobar(d, QStringLiteral("&Verificada, siguiente"));
        }
        {
            DialogoVerificadores d(&bd);
            comprobar(d, QStringLiteral("Cerrar"));
        }
        {
            // La ayuda no tiene: Intro en el buscador pasa a la siguiente vez que aparece lo buscado y no la cierra.
            DialogoAyuda d;
            comprobar(d, QString());
        }

        // El de marcar como verificadas las preguntas de la lista del banco, que se abre desde la pestaña.
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        // Se mira cada poco hasta que se abre, y el temporizador muere con la prueba: si no llegara a abrirse, no
        // quedaría nada pendiente que tocara otra prueba.
        QString problema = QStringLiteral("«Marcar como verificadas» no se ha abierto");
        QTimer espera;
        QObject::connect(&espera, &QTimer::timeout, &v, [&espera, &problema] {
            auto *d = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!d)
                return;
            espera.stop();
            problema = problemaConElBotonDeIntro(d, QStringLiteral("Marcar como verificadas"));
            d->reject();
        });
        espera.start(20);
        QVERIFY(QMetaObject::invokeMethod(v.findChild<PestanaBanco *>(), "verificarLista"));
        espera.stop();
        if (!problema.isEmpty())
            problemas << problema;
        v.close();
        QVERIFY2(problemas.isEmpty(), qPrintable(problemas.join(QLatin1Char('\n'))));
        QSettings().clear();
    }

    // Al verificar: si se marca una pregunta sin tiempo para leerla, se avisa y hay que pulsar otra vez; y cada cierto
    // tiempo seguido verificando, se propone una pausa. Con la casilla desmarcada, ni lo uno ni lo otro.
    void verificarAvisaDeLaPrisaYDeLasPausas()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        BaseDatos bd;
        QVERIFY(bd.abrir(ruta));
        QVERIFY(bd.anadirVerificador(QStringLiteral("Persona Uno")));
        QSettings().clear();

        // El tiempo mínimo: según las palabras, entre 2 y 20 segundos.
        Pregunta inventada;
        QCOMPARE(AtencionAlVerificar::tiempoParaLeer(inventada), 2000);
        inventada.enunciado = QStringLiteral("palabra ").repeated(60);
        inventada.opciones = QStringList(4, QStringLiteral("dos palabras"));
        inventada.explicacion = QStringLiteral("otras  dos\npalabras  más");
        QCOMPARE(AtencionAlVerificar::tiempoParaLeer(inventada), 72 * 150);
        inventada.enunciado = QStringLiteral("palabra ").repeated(1000);
        QCOMPARE(AtencionAlVerificar::tiempoParaLeer(inventada), 20000);

        // La cuenta, sin el diálogo: de la prisa se avisa una vez por pregunta, y la pausa se propone cuando se ha
        // verificado alguna y ha pasado el tiempo; después se empieza a contar de nuevo.
        AtencionAlVerificar atencion;
        atencion.empezar();
        atencion.preguntaMostrada();
        QVERIFY(atencion.demasiadoDeprisa(0, inventada));
        QVERIFY(!atencion.demasiadoDeprisa(0, inventada));
        QVERIFY(atencion.demasiadoDeprisa(1, inventada));
        atencion.ponerPausaCada(0);
        int minutos = -1;
        int verificadas = -1;
        QVERIFY(!atencion.tocaPausa(&minutos, &verificadas));
        atencion.contarVerificada();
        atencion.contarVerificada();
        QVERIFY(atencion.tocaPausa(&minutos, &verificadas));
        QCOMPARE(minutos, 0);
        QCOMPARE(verificadas, 2);
        QVERIFY(!atencion.tocaPausa(&minutos, &verificadas));

        const QVector<Pregunta> preguntas = bd.preguntas();
        QVERIFY(preguntas.size() > 4);
        DialogoVerificar d(&bd, preguntas, 0, bd.temas());
        d.show();
        auto *aviso = d.findChild<QLabel *>(QStringLiteral("avisoVerificar"));
        auto *casilla = d.findChild<QCheckBox *>(QStringLiteral("avisosVerificar"));
        QVERIFY(aviso && casilla && casilla->isChecked());
        auto *verificar = conTexto<QPushButton>(&d, QStringLiteral("&Verificada, siguiente"));
        QVERIFY(verificar);
        marcar(&d, QStringLiteral("Persona Uno"));

        // Demasiado deprisa: no la marca ni pasa a la siguiente hasta que se pulsa otra vez.
        verificar->click();
        QVERIFY(aviso->isVisible());
        QVERIFY2(aviso->text().startsWith(QStringLiteral("Apenas ha habido tiempo")), qPrintable(aviso->text()));
        QCOMPARE(d.actual(), 0);
        QVERIFY(!bd.preguntas().first().verificada);
        verificar->click();
        QCOMPARE(d.actual(), 1);
        QVERIFY(!aviso->isVisible());
        QVERIFY(bd.preguntas().first().verificada);

        // La pausa, al pasar a la siguiente cuando ha pasado el tiempo.
        d.ponerPausaCada(0);
        verificar->click();
        QCOMPARE(d.actual(), 1);
        verificar->click();
        QCOMPARE(d.actual(), 2);
        QVERIFY(aviso->isVisible());
        QVERIFY2(aviso->text().contains(QStringLiteral("(preguntas verificadas: 2)")), qPrintable(aviso->text()));
        QVERIFY2(aviso->text().contains(QStringLiteral("pausa")), qPrintable(aviso->text()));

        // Sin avisos (se recuerda): a la primera, y sin proponer pausas.
        casilla->setChecked(false);
        QCOMPARE(QSettings().value(QStringLiteral("verificar/avisos")).toBool(), false);
        verificar->click();
        QCOMPARE(d.actual(), 3);
        QVERIFY(!aviso->isVisible());
        d.reject();
        DialogoVerificar otro(&bd, preguntas, 0, bd.temas());
        QVERIFY(!otro.findChild<QCheckBox *>(QStringLiteral("avisosVerificar"))->isChecked());
        QSettings().clear();
    }

    // Al verificar, «Resolverla antes de ver la solución»: la ficha de las que están sin verificar sale sin la
    // respuesta ni la explicación, Intro enseña la solución y se compara con la respuesta que se haya dado.
    void verificarSinVerLaSolucion()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        BaseDatos bd;
        QVERIFY(bd.abrir(ruta));
        QSettings().clear();
        QSettings().setValue(QStringLiteral("verificar/avisos"), false);
        const QVector<Pregunta> preguntas = bd.preguntas();
        QVERIFY(preguntas.size() > 4);

        // La ficha sin solución no dice la respuesta ni lo que la delata.
        Pregunta inventada = preguntas.first();
        inventada.explicacion = QStringLiteral("Explicación inventada.");
        inventada.notas = QStringLiteral("Nota inventada.");
        const QString sin = Maquetador::fichaPreguntaHtml(inventada, QString(), false);
        const QString con = Maquetador::fichaPreguntaHtml(inventada, QString());
        for (const QString &texto : {QStringLiteral("Respuesta correcta"), QStringLiteral("Explicación inventada"), QStringLiteral("Nota inventada")}) {
            QVERIFY2(con.contains(texto), qPrintable(texto));
            QVERIFY2(!sin.contains(texto), qPrintable(texto));
        }
        QVERIFY(sin.contains(QStringLiteral("opción inventada")));
        QVERIFY(sin.size() < con.size());

        DialogoVerificar d(&bd, preguntas, 0, bd.temas());
        d.show();
        auto *ficha = d.findChild<VisorAmpliable *>();
        auto *resolver = d.findChild<QCheckBox *>(QStringLiteral("resolverVerificar"));
        auto *aviso = d.findChild<QLabel *>(QStringLiteral("avisoVerificar"));
        auto *panel = d.findChild<PanelVerificacion *>();
        QVERIFY(ficha && resolver && aviso && panel);
        const QList<QRadioButton *> letras = d.findChildren<QRadioButton *>();
        QCOMPARE(letras.size(), 4);
        auto boton = [&d]() {
            for (QPushButton *b : d.findChildren<QPushButton *>())
                if (b->isDefault())
                    return b;
            return static_cast<QPushButton *>(nullptr);
        };
        QVERIFY(boton());

        // Por defecto, como siempre: con la solución y sin las casillas de la respuesta.
        QVERIFY(!resolver->isChecked());
        QVERIFY(ficha->toPlainText().contains(QStringLiteral("Respuesta correcta")));
        QVERIFY(!letras.first()->isVisible());
        QCOMPARE(boton()->text(), QStringLiteral("&Verificada, siguiente"));

        // Al marcarla, se oculta ya en la que se ve, y no se puede dar por verificada sin verla.
        resolver->setChecked(true);
        QVERIFY(QSettings().value(QStringLiteral("verificar/resolver")).toBool());
        QVERIFY(!ficha->toPlainText().contains(QStringLiteral("Respuesta correcta")));
        QVERIFY(letras.first()->isVisible());
        QVERIFY(!panel->isEnabled());
        QCOMPARE(boton()->text(), QStringLiteral("&Mostrar la solución"));

        // Con una respuesta distinta de la del banco: se dice al mostrarla, y no se ha marcado nada.
        const int correcta = preguntas.at(0).correcta;
        letras.at((correcta + 1) % 4)->setChecked(true);
        boton()->click();
        QCOMPARE(d.actual(), 0);
        QVERIFY(ficha->toPlainText().contains(QStringLiteral("Respuesta correcta")));
        QVERIFY(aviso->isVisible());
        QVERIFY2(aviso->text().contains(QStringLiteral("revísela con cuidado")), qPrintable(aviso->text()));
        QVERIFY(panel->isEnabled());
        QVERIFY(!letras.first()->isVisible());
        QVERIFY(!bd.preguntas().first().verificada);
        QCOMPARE(boton()->text(), QStringLiteral("&Verificada, siguiente"));

        // Ya a la vista, se verifica y se pasa a la siguiente, que sale oculta y sin respuesta marcada.
        boton()->click();
        QCOMPARE(d.actual(), 1);
        QVERIFY(bd.preguntas().first().verificada);
        QVERIFY(!ficha->toPlainText().contains(QStringLiteral("Respuesta correcta")));
        for (QRadioButton *r : letras)
            QVERIFY(!r->isChecked());
        letras.at(preguntas.at(1).correcta)->setChecked(true);
        boton()->click();
        QVERIFY2(aviso->text().contains(QStringLiteral("coincide con la del banco")), qPrintable(aviso->text()));

        // Sin responder, solo la enseña. Y las que ya están verificadas se ven enteras.
        pulsar(&d, QStringLiteral("&Siguiente"));
        QCOMPARE(d.actual(), 2);
        boton()->click();
        QVERIFY(ficha->toPlainText().contains(QStringLiteral("Respuesta correcta")));
        QVERIFY(!aviso->isVisible());
        for (QPushButton *b : d.findChildren<QPushButton *>())
            if (b->text() == QStringLiteral("&Anterior"))
                b->click(), b->click();
        QCOMPARE(d.actual(), 0);
        QVERIFY(ficha->toPlainText().contains(QStringLiteral("Respuesta correcta")));
        QVERIFY(panel->isEnabled());
        d.reject();
        QSettings().clear();
    }

    // «Puntuación» y «Opciones» vienen plegadas, con lo elegido en la cabecera; se despliegan al pulsarla, y cómo
    // están se recuerda.
    void ventanaPliegaLoQueNoCambia()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        QSettings().clear();
        {
            VentanaPrincipal v;
            v.resize(1200, 800);
            v.show();
            QVERIFY(v.abrirBancoInicial(ruta));
            auto *puntuacion = v.findChild<GrupoPlegable *>(QStringLiteral("plegablePuntuacion"));
            auto *opciones = v.findChild<GrupoPlegable *>(QStringLiteral("plegableOpciones"));
            QVERIFY(puntuacion && opciones);
            auto *cabecera = opciones->findChild<QToolButton *>();
            QVERIFY(cabecera);
            QMap<QString, QAbstractButton *> botones;
            for (GrupoPlegable *g : {puntuacion, opciones})
                for (QAbstractButton *b : g->findChildren<QAbstractButton *>())
                    botones.insert(b->text(), b);
            auto *semilla = opciones->findChild<QLineEdit *>();
            QVERIFY(semilla);

            // De entrada, plegadas: no se ven los controles, sí el resumen.
            QVERIFY(puntuacion->plegado() && opciones->plegado());
            QVERIFY(!botones.value(QStringLiteral("Los fallos no restan"))->isVisible());
            QVERIFY(!semilla->isVisible());
            QCOMPARE(puntuacion->resumen(), QStringLiteral("Cada fallo resta el 30 %"));
            QCOMPARE(opciones->resumen(), QStringLiteral("las habituales"));
            QCOMPARE(cabecera->toolTip(), QStringLiteral("Opciones: las habituales"));

            // El resumen sigue a lo que se elige: solo lo que se aparta de lo habitual, y la semilla.
            botones.value(QStringLiteral("Los fallos no restan"))->setChecked(true);
            QCOMPARE(puntuacion->resumen(), QStringLiteral("Los fallos no restan"));
            botones.value(QStringLiteral("Personalizada"))->setChecked(true);
            QVERIFY2(puntuacion->resumen().startsWith(QStringLiteral("acierto +")), qPrintable(puntuacion->resumen()));
            botones.value(QStringLiteral("Solo preguntas verificadas por humanos"))->setChecked(true);
            botones.value(QStringLiteral("No repetir preguntas entre exámenes"))->setChecked(false);
            semilla->setText(QStringLiteral("42"));
            QCOMPARE(opciones->resumen(), QStringLiteral("se repiten entre exámenes, solo verificadas, semilla 42"));

            // Al pulsar la cabecera se despliega (y el título queda sin resumen); otra vez, se pliega.
            cabecera->click();
            QVERIFY(!opciones->plegado());
            QVERIFY(semilla->isVisible());
            QCOMPARE(cabecera->toolTip(), QStringLiteral("Opciones"));
            QVERIFY(puntuacion->plegado());
        }
        // Se recuerda cómo estaba cada una.
        VentanaPrincipal otra;
        QVERIFY(!otra.findChild<GrupoPlegable *>(QStringLiteral("plegableOpciones"))->plegado());
        QVERIFY(otra.findChild<GrupoPlegable *>(QStringLiteral("plegablePuntuacion"))->plegado());
        QSettings().clear();
    }

    // Los filtros de tema y de estado del banco que elige el usuario se recuerdan de una vez a otra; el texto, no.
    void ventanaRecuerdaLosFiltrosDelBanco()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        QSettings().clear();
        auto combos = [](VentanaPrincipal &v, QComboBox *&tema, QComboBox *&estado) {
            tema = estado = nullptr;
            for (QComboBox *c : v.findChildren<QComboBox *>()) {
                if (c->itemText(0) == QStringLiteral("Todos los temas"))
                    tema = c;
                else if (c->findText(QStringLiteral("Sin verificar")) >= 0)
                    estado = c;
            }
        };
        int total = 0, filtradas = 0;
        {
            VentanaPrincipal v;
            QVERIFY(v.abrirBancoInicial(ruta));
            QComboBox *tema, *estado;
            combos(v, tema, estado);
            QVERIFY(tema && estado);
            QCOMPARE(tema->currentIndex(), 0);
            QCOMPARE(estado->currentIndex(), 0);
            auto *tabla = v.findChild<QTableView *>();
            total = tabla->model()->rowCount();
            // Como al elegirlos con el ratón o el teclado.
            tema->setCurrentIndex(2);
            emit tema->activated(2);
            estado->setCurrentIndex(estado->findText(QStringLiteral("Sin verificar")));
            emit estado->activated(estado->currentIndex());
            filtradas = tabla->model()->rowCount();
            QVERIFY(filtradas > 0 && filtradas < total);
            for (QLineEdit *l : v.findChildren<QLineEdit *>())
                if (l->placeholderText().startsWith(QStringLiteral("Buscar texto")))
                    l->setText(QStringLiteral("inventada"));
        }
        {
            VentanaPrincipal v;
            QVERIFY(v.abrirBancoInicial(ruta));
            QComboBox *tema, *estado;
            combos(v, tema, estado);
            QCOMPARE(tema->currentIndex(), 2);
            QCOMPARE(estado->currentText(), QStringLiteral("Sin verificar"));
            QCOMPARE(v.findChild<QTableView *>()->model()->rowCount(), filtradas);
            for (QLineEdit *l : v.findChildren<QLineEdit *>())
                if (l->placeholderText().startsWith(QStringLiteral("Buscar texto")))
                    QVERIFY(l->text().isEmpty());
            // Lo que cambia el programa (no el usuario) no se guarda.
            tema->setCurrentIndex(0);
        }
        VentanaPrincipal v;
        QVERIFY(v.abrirBancoInicial(ruta));
        QComboBox *tema, *estado;
        combos(v, tema, estado);
        QCOMPARE(tema->currentIndex(), 2);
        QSettings().clear();
    }

    // Lo que pide una decisión se queda en la franja de avisos, junto al examen, hasta que se resuelve: la semilla
    // fija (con un enlace para quitarla) y las preguntas del examen que han dejado de estar activas.
    void avisosQueSeQuedanJuntoAlExamen()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        auto *avisos = v.findChild<QLabel *>(QStringLiteral("avisosExamen"));
        auto *semilla = v.findChild<GrupoPlegable *>(QStringLiteral("plegableOpciones"))->findChild<QLineEdit *>();
        QVERIFY(avisos && semilla);
        // Sin lo de «no hay ninguna convocatoria registrada», para ver solo los de aquí.
        marcar(&v, QStringLiteral("Evitar las preguntas de las últimas"), false);

        // (Puede haber otros avisos de la generación, según cómo salgan los exámenes.)
        auto dice = [avisos](const QString &texto) { return avisos->isVisible() && avisos->text().contains(texto); };
        QMetaObject::invokeMethod(&v, "generar");
        QVERIFY2(!dice(QStringLiteral("semilla")), qPrintable(avisos->text()));

        // Con la semilla escrita: el aviso no se va solo, y su enlace la quita.
        semilla->setText(QStringLiteral("42"));
        QVERIFY(!dice(QStringLiteral("semilla")));   // los que se ven no se generaron con ella
        QMetaObject::invokeMethod(&v, "generar");
        QVERIFY(avisos->isVisible());
        QVERIFY2(avisos->text().contains(QStringLiteral("Generados con la semilla fija 42")), qPrintable(avisos->text()));
        QVERIFY(avisos->text().contains(QStringLiteral("<a href=\"quitar-semilla\">Quitar la semilla</a>")));
        QTest::qWait(50);
        QVERIFY(avisos->isVisible());
        emit avisos->linkActivated(QStringLiteral("quitar-semilla"));
        QVERIFY(semilla->text().isEmpty());
        QVERIFY(!dice(QStringLiteral("semilla")));
        QVERIFY(!dice(QStringLiteral("ya no está activa")));

        // Una pregunta del examen que se anula al editarla: se dice cuál, y sigue dicho hasta que se cambie.
        QTimer::singleShot(100, [&] {
            auto *d = qobject_cast<DialogoPregunta *>(QApplication::activeModalWidget());
            if (!d)
                return;
            for (QComboBox *c : d->findChildren<QComboBox *>())
                if (c->findText(QStringLiteral("Anulada")) >= 0)
                    c->setCurrentIndex(c->findText(QStringLiteral("Anulada")));
            responder(QStringLiteral("Guardar así"));
            pulsar(d, QStringLiteral("Guardar"));
        });
        QVERIFY(QMetaObject::invokeMethod(&v, "editarPreguntaExamen"));
        QVERIFY(avisos->isVisible());
        QVERIFY2(avisos->text().contains(QStringLiteral("pregunta 1 (")), qPrintable(avisos->text()));
        QVERIFY2(avisos->text().contains(QStringLiteral("ya no está activa")), qPrintable(avisos->text()));
        QVERIFY(!avisos->text().contains(QStringLiteral("semilla")));
        QSettings().clear();
    }

    // La vista previa del editor de preguntas se amplía como las demás fichas, y su tamaño se recuerda.
    void editorAmpliaLaVistaPrevia()
    {
        QSettings().clear();
        {
            DialogoPregunta d(temasInventados(2), bancoInventado(2, 5), nuevaEn(1, 6));
            auto *vista = d.findChild<VisorAmpliable *>();
            QVERIFY(vista);
            QCOMPARE(vista->escala(), 1.0);
            const QVector<qreal> normal = tamanos(vista->document());
            vista->ampliar();
            QCOMPARE(vista->escala(), 1.1);
            QVERIFY(proporcionales(tamanos(vista->document()), normal, 1.1));
        }
        DialogoPregunta d(temasInventados(2), bancoInventado(2, 5), nuevaEn(1, 6));
        QCOMPARE(d.findChild<VisorAmpliable *>()->escala(), 1.1);
        QSettings().clear();
    }

    void visorEscalaLaLetraYLasImagenes()
    {
        QTextDocument doc;
        doc.setDefaultFont(QFont(QStringLiteral("Sans"), 10));
        QImage imagen(20, 10, QImage::Format_RGB32);
        imagen.fill(Qt::red);
        doc.addResource(QTextDocument::ImageResource, QUrl(QStringLiteral("img:1")), imagen);
        doc.setHtml(QStringLiteral("<h3>Título</h3><p>Sin tamaño, <span style=\"font-size:16pt\">en puntos</span> y "
                                   "<span style=\"font-size:20px\">en píxeles</span>.</p><p><img src=\"img:1\" width=\"40\"></p>"
                                   "<table><tr><td width=\"30\">Celda</td><td></td></tr></table><pre>uno\n\ndos</pre>"));
        const QVector<qreal> normal = tamanos(&doc);
        const QString texto = doc.toPlainText();
        QVERIFY(normal.contains(16) && normal.contains(20) && normal.contains(40));

        QTextTable *tabla = nullptr;
        for (QTextFrame *f : doc.rootFrame()->childFrames())
            tabla = tabla ? tabla : qobject_cast<QTextTable *>(f);
        QVERIFY(tabla);
        auto anchoColumna = [tabla] { return tabla->format().columnWidthConstraints().value(0).rawValue(); };
        QCOMPARE(anchoColumna(), 30.0);

        VisorAmpliable::escalarDocumento(&doc, 2);
        QVERIFY(proporcionales(tamanos(&doc), normal, 2));
        QCOMPARE(anchoColumna(), 60.0);
        // Siempre respecto al original: escalar dos veces seguidas con lo mismo no acumula.
        VisorAmpliable::escalarDocumento(&doc, 2);
        QVERIFY(proporcionales(tamanos(&doc), normal, 2));
        VisorAmpliable::escalarDocumento(&doc, 0.5);
        QVERIFY(proporcionales(tamanos(&doc), normal, 0.5));
        QCOMPARE(anchoColumna(), 15.0);
        VisorAmpliable::escalarDocumento(&doc, 1);
        QCOMPARE(tamanos(&doc), normal);
        QCOMPARE(anchoColumna(), 30.0);
        // Solo cambian los tamaños: el texto y la tabla siguen como estaban.
        QCOMPARE(doc.toPlainText(), texto);
        QCOMPARE(tabla->columns(), 2);
        QCOMPARE(tabla->cellAt(0, 0).firstCursorPosition().block().text(), QStringLiteral("Celda"));
    }

    void ventanaAmpliaLaVistaDelExamen()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(QTest::qWaitForWindowActive(&v));
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QMetaObject::invokeMethod(&v, "generar");
        auto *visor = v.findChild<VisorAmpliable *>(QStringLiteral("visorExamen"));
        QVERIFY(visor);
        QCOMPARE(visor->escala(), 1.0);
        const QVector<qreal> normal = tamanos(visor->document());
        const QString texto = visor->toPlainText();
        QVERIFY(texto.contains(QStringLiteral("Respuesta correcta: +")));

        // Ctrl + rueda sobre el examen: un paso de la rueda, o dos medios pasos de una rueda fina o de un panel táctil.
        auto rueda = [visor](int giro) {
            const QPoint centro = visor->viewport()->rect().center();
            QWheelEvent e(centro, visor->viewport()->mapToGlobal(centro), QPoint(), QPoint(0, giro), Qt::NoButton,
                          Qt::ControlModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(visor->viewport(), &e);
        };
        rueda(120);
        QCOMPARE(visor->escala(), 1.1);
        QVERIFY(proporcionales(tamanos(visor->document()), normal, 1.1));
        rueda(60);
        QCOMPARE(visor->escala(), 1.1);
        rueda(60);
        QCOMPARE(visor->escala(), 1.25);
        QCOMPARE(visor->toPlainText(), texto);
        QCOMPARE(QSettings().value(QStringLiteral("vista/escala")).toDouble(), 1.25);

        // Con el teclado, aunque el foco esté en otro sitio de la pestaña.
        auto *generar = conTexto<QPushButton>(&v, QStringLiteral("Generar exámenes"));
        QVERIFY(generar);
        generar->setFocus();
        QTest::keyClick(generar, Qt::Key_Minus, Qt::ControlModifier);
        QCOMPARE(visor->escala(), 1.1);
        QTest::keyClick(generar, Qt::Key_Plus, Qt::ControlModifier);
        QCOMPARE(visor->escala(), 1.25);
        QTest::keyClick(generar, Qt::Key_0, Qt::ControlModifier);
        QCOMPARE(visor->escala(), 1.0);
        QCOMPARE(tamanos(visor->document()), normal);
        rueda(-120);
        QCOMPARE(visor->escala(), 0.9);

        // Lo que estaba arriba de la vista sigue arriba.
        visor->scrollToAnchor(QStringLiteral("q5"));
        const int arriba = visor->cursorForPosition(QPoint(0, 0)).blockNumber();
        QVERIFY(arriba > 0);
        visor->ampliar();
        QCOMPARE(visor->cursorForPosition(QPoint(0, 0)).blockNumber(), arriba);

        // El examen que se pone después en la vista (aquí, con soluciones) sale con la misma escala.
        visor->tamanoNormal();
        auto *soluciones = conTexto<QCheckBox>(&v, QStringLiteral("Mostrar soluciones"));
        QVERIFY(soluciones);
        soluciones->setChecked(true);
        const QVector<qreal> conSoluciones = tamanos(visor->document());
        visor->ponerEscala(1.5);
        soluciones->setChecked(false);
        soluciones->setChecked(true);
        QVERIFY(proporcionales(tamanos(visor->document()), conSoluciones, 1.5));

        // Las acciones están también en el menú del botón derecho, con sus teclas escritas (las atiende la ventana,
        // que amplía la zona en la que está el puntero), y la ventana recuerda la escala.
        const QString ctrl = QKeySequence(Qt::CTRL | Qt::Key_A).toString(QKeySequence::NativeText).chopped(1);   // «Ctrl+», «⌘» en macOS
        for (QAction *a : visor->accionesEscala())
            QVERIFY2(a->shortcuts().isEmpty() && a->text().contains(QStringLiteral("\t") + ctrl), qPrintable(a->text()));
        VentanaPrincipal otra;
        QCOMPARE(otra.findChild<VisorAmpliable *>(QStringLiteral("visorExamen"))->escala(), 1.5);
        QSettings().clear();
    }

    // El panel izquierdo de «Generar examen» y el banco de preguntas (la lista y la ficha) se amplían también, cada
    // uno con su tamaño, con Ctrl + rueda sobre ellos o con las teclas cuando el puntero está encima.
    void ventanaAmpliaLosPaneles()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(QTest::qWaitForWindowActive(&v));
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QMetaObject::invokeMethod(&v, "generar");
        auto *visor = v.findChild<VisorAmpliable *>(QStringLiteral("visorExamen"));
        auto *ficha = v.findChild<VisorAmpliable *>(QStringLiteral("fichaBanco"));
        auto *tabla = v.findChild<QTableView *>();
        auto *pestanas = v.findChild<QTabWidget *>();
        auto *generar = conTexto<QPushButton>(&v, QStringLiteral("Generar exámenes"));
        QVERIFY(visor && ficha && tabla && pestanas && generar);
        // Una casilla numérica del panel izquierdo (el botón está en él, debajo de las opciones).
        QSpinBox *numero = nullptr;
        for (QSpinBox *s : generar->parentWidget()->findChildren<QSpinBox *>())
            if (!numero && s->isVisible() && s->isEnabled() && s->value() < s->maximum())
                numero = s;
        QVERIFY(numero);
        auto rueda = [](QWidget *w, int giro) {
            const QPoint centro = w->rect().center();
            QWheelEvent e(centro, w->mapToGlobal(centro), QPoint(), QPoint(0, giro), Qt::NoButton, Qt::ControlModifier,
                          Qt::NoScrollPhase, false);
            QApplication::sendEvent(w, &e);
        };
        auto letra = [](const QWidget *w) { return w->font().pointSizeF(); };
        const qreal normal = letra(generar);
        const qreal normalTabla = letra(tabla);
        const QVector<qreal> examen = tamanos(visor->document());
        QVERIFY(normal > 0 && normalTabla > 0);

        // Sobre un control del panel: crece la letra de todo el panel, y la casilla no cambia de valor.
        const int valor = numero->value();
        rueda(numero, 120);
        QCOMPARE(numero->value(), valor);
        QCOMPARE(letra(numero), normal * 1.1);
        QCOMPARE(letra(generar), normal * 1.1);
        QVERIFY(generar->font().bold());
        // También las etiquetas con color propio («de 20», junto a cada tema), las que hay y las que se ponen después.
        auto deTantas = [&]() -> QLabel * {
            for (QLabel *l : generar->parentWidget()->findChildren<QLabel *>())
                if (l->text().startsWith(QStringLiteral("de ")))
                    return l;
            return nullptr;
        };
        QVERIFY(deTantas());
        QCOMPARE(letra(deTantas()), normal * 1.1);
        QVERIFY(v.abrirBaseDatos(dir.filePath(QStringLiteral("banco.db"))));
        QVERIFY(deTantas());
        QCOMPARE(letra(deTantas()), normal * 1.1);
        QMetaObject::invokeMethod(&v, "generar");
        // Ni el examen ni el banco cambian.
        QCOMPARE(visor->escala(), 1.0);
        QCOMPARE(tamanos(visor->document()), examen);
        QCOMPARE(letra(tabla), normalTabla);
        QCOMPARE(QSettings().value(QStringLiteral("vista/escalaPanel")).toDouble(), 1.1);
        // Sin Ctrl, la rueda hace lo de siempre.
        rueda(generar, -120);
        QCOMPARE(letra(generar), normal);

        // Las teclas, sobre la zona en la que está el puntero.
        QCursor::setPos(generar->mapToGlobal(generar->rect().center()));
        if (QApplication::widgetAt(QCursor::pos()) == generar) {
            QTest::keyClick(&v, Qt::Key_Plus, Qt::ControlModifier);
            QCOMPARE(letra(generar), normal * 1.1);
            QCOMPARE(visor->escala(), 1.0);
            QTest::keyClick(&v, Qt::Key_0, Qt::ControlModifier);
            QCOMPARE(letra(generar), normal);
            QCursor::setPos(visor->mapToGlobal(visor->rect().center()));
            QTest::keyClick(&v, Qt::Key_Plus, Qt::ControlModifier);
            QCOMPARE(visor->escala(), 1.1);
            QCOMPARE(letra(generar), normal);
            QTest::keyClick(&v, Qt::Key_0, Qt::ControlModifier);
        }

        // El banco: la lista (la letra, el alto de las filas y el ancho de las columnas) y la ficha, a la vez.
        pestanas->setCurrentIndex(1);
        tabla->setCurrentIndex(tabla->model()->index(0, 0));
        const QVector<qreal> fichaNormal = tamanos(ficha->document());
        const int ancho = tabla->columnWidth(ModeloPreguntas::ColCodigo);
        const int alto = tabla->rowHeight(0);
        QVERIFY(!fichaNormal.isEmpty());
        rueda(tabla->viewport(), 120);
        rueda(tabla->viewport(), 120);
        QCOMPARE(letra(tabla), normalTabla * 1.25);
        QCOMPARE(tabla->columnWidth(ModeloPreguntas::ColCodigo), qRound(ancho * 1.25));
        QVERIFY2(tabla->rowHeight(0) > alto, qPrintable(QStringLiteral("%1, %2").arg(tabla->rowHeight(0)).arg(alto)));
        QCOMPARE(ficha->escala(), 1.25);
        QVERIFY(proporcionales(tamanos(ficha->document()), fichaNormal, 1.25));
        // La ficha de otra pregunta sale igual de grande, y sobre la ficha la rueda amplía también la lista.
        tabla->setCurrentIndex(tabla->model()->index(1, 0));
        QCOMPARE(ficha->escala(), 1.25);
        rueda(ficha->viewport(), -120);
        QCOMPARE(ficha->escala(), 1.1);
        QCOMPARE(letra(tabla), normalTabla * 1.1);
        QCOMPARE(letra(generar), normal);
        QCOMPARE(visor->escala(), 1.0);
        // Con el puntero fuera de las zonas, las teclas amplían lo de la pestaña que se ve.
        QCursor::setPos(v.mapToGlobal(QPoint(2, 2)));
        QTest::keyClick(&v, Qt::Key_0, Qt::ControlModifier);
        QCOMPARE(letra(tabla), normalTabla);
        QCOMPARE(tabla->columnWidth(ModeloPreguntas::ColCodigo), ancho);
        rueda(tabla->viewport(), 120);

        // Cada tamaño se recuerda.
        rueda(generar, 120);
        rueda(generar, 120);
        VentanaPrincipal otra;
        auto *otroGenerar = conTexto<QPushButton>(&otra, QStringLiteral("Generar exámenes"));
        QVERIFY(otroGenerar);
        QCOMPARE(letra(otroGenerar), normal * 1.25);
        QCOMPARE(letra(otra.findChild<QTableView *>()), normalTabla * 1.1);
        QCOMPARE(otra.findChild<VisorAmpliable *>(QStringLiteral("fichaBanco"))->escala(), 1.1);
        QCOMPARE(otra.findChild<QTableView *>()->columnWidth(ModeloPreguntas::ColCodigo), qRound(ancho * 1.1));
        QCOMPARE(otra.findChild<VisorAmpliable *>(QStringLiteral("visorExamen"))->escala(), 1.0);
        QSettings().clear();
    }

    // La vista del examen acaba con lo que sale al pie de las páginas, y cambia con las casillas de «Pie de página».
    void vistaConElPieDePagina()
    {
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        QSettings().clear();
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        QMetaObject::invokeMethod(&v, "generar");
        auto *visor = v.findChild<VisorAmpliable *>(QStringLiteral("visorExamen"));
        QVERIFY(visor);
        const QRegularExpression pie(QStringLiteral("Nacional Original · \\S+ \\d{4}: (\\d+) / \\1\\s*$"));
        QVERIFY2(pie.match(visor->toPlainText()).hasMatch(), qPrintable(visor->toPlainText().right(120)));
        QCheckBox *convocatoriaEnPie = nullptr;
        for (QCheckBox *c : v.findChildren<QCheckBox *>())
            if (c->text() == QStringLiteral("Convocatoria y año") && c->toolTip().contains(QStringLiteral("pie de cada")))
                convocatoriaEnPie = c;
        QVERIFY(convocatoriaEnPie);
        convocatoriaEnPie->setChecked(false);
        const QRegularExpression sinConvocatoria(QStringLiteral("\nNacional Original: (\\d+) / \\1\\s*$"));
        QVERIFY2(sinConvocatoria.match(visor->toPlainText()).hasMatch(), qPrintable(visor->toPlainText().right(120)));
        QSettings().clear();
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
            if (t->toPlainText().contains(QStringLiteral("Respuesta correcta: +")))
                visor = t;
        QVERIFY(visor);
        auto *registrar = conTexto<QPushButton>(&v, QStringLiteral("Registrar convocatoria…"));
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
            pulsar(d, QStringLiteral("Exportar"));
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
        QVERIFY(semilla->styleSheet().isEmpty());   // sin color: lo dicen el resumen de las opciones y el aviso
        QMetaObject::invokeMethod(&v, "generar");
        for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
            if (t->toPlainText().contains(QStringLiteral("Respuesta correcta: +")))
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

    void ventanaEligeCabeceraYPie()
    {
        // Las casillas de la cabecera y del pie, marcadas por omisión y guardadas en el banco; la vista sigue a las de
        // la cabecera. La semilla no va en el examen.
        QTemporaryDir dir;
        QVERIFY(crearBancoInventado(dir.filePath(QStringLiteral("banco.db"))));
        QSettings().clear();
        const QString asignatura = QStringLiteral("Asignatura de la cabecera");
        {
            VentanaPrincipal v;
            v.show();
            QVERIFY(v.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
            QLineEdit *campo = nullptr;
            for (QLineEdit *l : v.findChildren<QLineEdit *>())
                if (l->placeholderText() == QStringLiteral("Nombre de la asignatura"))
                    campo = l;
            QVERIFY(campo);
            campo->setText(asignatura);
            QMap<QString, QCheckBox *> casillas;
            for (QCheckBox *c : v.findChildren<QCheckBox *>())
                if (c->toolTip().startsWith(QStringLiteral("Pone ")))
                    casillas.insert(c->toolTip().section(QLatin1Char(','), 0, 0), c);
            QCOMPARE(casillas.size(), 4);
            for (QCheckBox *c : std::as_const(casillas))
                QVERIFY(c->isChecked());
            QMetaObject::invokeMethod(&v, "generar");
            QTextBrowser *visor = nullptr;
            for (QTextBrowser *t : v.findChildren<QTextBrowser *>())
                if (t->toPlainText().contains(QStringLiteral("Respuesta correcta: +")))
                    visor = t;
            QVERIFY(visor);
            QVERIFY(visor->toPlainText().contains(asignatura));
            QVERIFY(!visor->toPlainText().contains(QStringLiteral("Semilla")));
            // Arriba, en la cabecera, y abajo, en el pie (al final de la vista): cada una con su casilla.
            QCOMPARE(visor->toPlainText().count(asignatura), 2);
            casillas.value(QStringLiteral("Pone el nombre de la asignatura arriba"))->setChecked(false);
            QCOMPARE(visor->toPlainText().count(asignatura), 1);
            QVERIFY(!visor->toPlainText().startsWith(asignatura));
            casillas.value(QStringLiteral("Pone el nombre de la asignatura abajo"))->setChecked(false);
            QVERIFY(!visor->toPlainText().contains(asignatura));
            casillas.value(QStringLiteral("Pone el nombre de la asignatura abajo"))->setChecked(true);
            QVERIFY(v.close());   // guarda la configuración
        }
        // Al volver a abrir el banco, la casilla sigue sin marcar.
        VentanaPrincipal otra;
        QVERIFY(otra.abrirBancoInicial(dir.filePath(QStringLiteral("banco.db"))));
        for (QCheckBox *c : otra.findChildren<QCheckBox *>())
            if (c->toolTip().startsWith(QStringLiteral("Pone ")))
                QCOMPARE(c->isChecked(), !c->toolTip().startsWith(QStringLiteral("Pone el nombre de la asignatura arriba")));
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
                              "historial", "anteriores", "moodle", "comprobarjson", "formatojson", "idioma", "verificar",
                              "verificadores"})
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
        QVERIFY(ayuda.isVisible());   // Intro no cierra la ayuda
        // Si no está, no se mueve.
        auto *noEsta = ayuda.findChild<QLabel *>(QStringLiteral("noEsta"));
        QVERIFY(noEsta && !noEsta->isVisible());
        QTest::keyClicks(buscar, QStringLiteral("xyz"));
        QCOMPARE(texto->textCursor().position(), texto->textCursor().anchor());
        // Y se dice con palabras, no solo con el color del campo.
        QVERIFY(noEsta->isVisible());
        QCOMPARE(noEsta->text(), QStringLiteral("No se encuentra"));
        // Sin distinguir tildes: «examenes» encuentra «exámenes».
        buscar->clear();
        QTest::keyClicks(buscar, QStringLiteral("examenes"));
        QVERIFY(!noEsta->isVisible());
        QCOMPARE(texto->textCursor().selectedText().toLower(), QStringLiteral("exámenes"));
    }

    void ayudaPorLaVentanaDesdeLaQueSePide()
    {
        // El botón «Ayuda» del editor la abre por su apartado, hija del editor: si no, el editor (modal) la bloquearía.
        auto *editor = new DialogoPregunta(temasInventados(2), bancoInventado(2, 5), nuevaEn(1, 6));
        editor->show();
        auto *boton = conTexto<QPushButton>(editor, QStringLiteral("Ayuda"));
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
        auto *manual = conTexto<QAction>(&v, QStringLiteral("&Manual de uso"));
        QVERIFY(manual);
        manual->trigger();
        QVERIFY(!delEditor);
        auto *ayuda = v.findChild<DialogoAyuda *>();
        QVERIFY(ayuda);
        QVERIFY(ayuda->isVisible());
        QCOMPARE(ayuda->seccionActual(), QStringLiteral("banco"));
        delete editor;
    }

    // El idioma de los exámenes se elige en la ventana y se guarda en el banco; al cambiarlo, la ventana propone las
    // convocatorias y las instrucciones en él (si no se han cambiado las instrucciones).
    void idiomaDeLosExamenes()
    {
        QSettings().clear();
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        QComboBox *idioma = nullptr;
        QComboBox *convocatoria = nullptr;
        for (QComboBox *c : v.findChildren<QComboBox *>()) {
            if (c->findData(QStringLiteral("en")) >= 0)
                idioma = c;
            if (c->findText(QStringLiteral("Junio")) >= 0)
                convocatoria = c;
        }
        QVERIFY(idioma && convocatoria);
        QPlainTextEdit *instrucciones = nullptr;
        for (QPlainTextEdit *e : v.findChildren<QPlainTextEdit *>())
            if (e->toPlainText() == DatosConvocatoria::instruccionesPorDefecto(QStringLiteral("es")))
                instrucciones = e;
        QVERIFY(instrucciones);
        QCOMPARE(idioma->currentData().toString(), QStringLiteral("es"));
        convocatoria->setCurrentText(QStringLiteral("Junio"));

        idioma->setCurrentIndex(idioma->findData(QStringLiteral("en")));
        QCOMPARE(convocatoria->currentText(), QStringLiteral("June"));
        QVERIFY(convocatoria->findText(QStringLiteral("February")) >= 0);
        QCOMPARE(instrucciones->toPlainText(), DatosConvocatoria::instruccionesPorDefecto(QStringLiteral("en")));
        {
            BaseDatos bd;
            QVERIFY(bd.abrir(ruta));
            QCOMPARE(bd.idioma(), QStringLiteral("en"));
        }
        // Unas instrucciones cambiadas a mano y una convocatoria que no es un mes se quedan como están.
        instrucciones->setPlainText(QStringLiteral("Instrucciones inventadas."));
        convocatoria->setCurrentText(QStringLiteral("Extraordinaria"));
        idioma->setCurrentIndex(idioma->findData(QStringLiteral("es")));
        QCOMPARE(instrucciones->toPlainText(), QStringLiteral("Instrucciones inventadas."));
        QCOMPARE(convocatoria->currentText(), QStringLiteral("Extraordinaria"));
        QVERIFY(convocatoria->findText(QStringLiteral("Febrero")) >= 0);

        // Al volver a abrir el banco, su idioma.
        idioma->setCurrentIndex(idioma->findData(QStringLiteral("en")));
        VentanaPrincipal otra;
        otra.show();
        QVERIFY(otra.abrirBancoInicial(ruta));
        for (QComboBox *c : otra.findChildren<QComboBox *>())
            if (c->findData(QStringLiteral("en")) >= 0)
                QCOMPARE(c->currentData().toString(), QStringLiteral("en"));
    }

    // Editar → Idioma: el del sistema o uno de los del programa; se guarda y se aplica al volver a abrirlo.
    void menuDeIdioma()
    {
        QSettings().clear();
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        QVERIFY(crearBancoInventado(ruta));
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        QMap<QString, QAction *> acciones;
        for (QAction *a : v.findChildren<QAction *>())
            if (a->isCheckable() && a->actionGroup() && a->data().typeId() == QMetaType::QString)
                acciones.insert(a->data().toString(), a);
        QCOMPARE(acciones.keys(), (QStringList{QString(), QStringLiteral("en"), QStringLiteral("es")}));
        QVERIFY(acciones.value(QString())->isChecked());   // nada elegido: el del sistema
        QVERIFY(acciones.value(QString())->text().startsWith(QStringLiteral("El del sistema")));
        QCOMPARE(acciones.value(QStringLiteral("en"))->text(), QStringLiteral("English"));

        // Si cambia el idioma, avisa de que se aplica al volver a abrir el programa («Más tarde»: no lo reinicia).
        const QString otro = Idioma::interfaz() == QLatin1String("en") ? QStringLiteral("es") : QStringLiteral("en");
        QString aviso;
        responder(QStringLiteral("Más tarde"), &aviso);
        acciones.value(otro)->trigger();
        QCOMPARE(Idioma::elegido(), otro);
        QVERIFY2(aviso.contains(QStringLiteral("la próxima vez que se abra")), qPrintable(aviso));
        QVERIFY(v.isVisible());
        Idioma::elegir(QString());
    }

    // Antes de importar, un JSON con errores no se puede importar; corregido el fichero, «Volver a comprobar» lo deja.
    void comprobacionAntesDeImportar()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("nuevas.json"));
        auto escribir = [&ruta](const QByteArray &correcta) {
            QFile f(ruta);
            if (!f.open(QIODevice::WriteOnly))
                return false;
            f.write("{\"temas\": [{\"numero\": 1, \"titulo\": \"Inventado\", \"preguntas\": [\n"
                    "{\"numero\": 1, \"enunciado\": \"Inventado:\", \"opciones\": [\"Uno.\", \"Dos.\", \"Tres.\", \"Cuatro.\"], "
                    "\"correcta\": \"" + correcta + "\"}]}]}");
            return true;
        };
        QVERIFY(escribir("E"));
        DialogoComprobacion d(ruta, {}, DialogoComprobacion::AntesDeImportar);
        QPushButton *importar = nullptr;
        QPushButton *volver = nullptr;
        for (QPushButton *b : d.findChildren<QPushButton *>()) {
            if (b->text().startsWith(QStringLiteral("Importar")))
                importar = b;
            if (b->text() == QStringLiteral("Volver a comprobar"))
                volver = b;
        }
        QVERIFY(importar && volver);
        QVERIFY(!importar->isEnabled());
        auto *lista = d.findChild<QTreeWidget *>();
        QCOMPARE(lista->topLevelItemCount(), 1);
        QCOMPARE(lista->topLevelItem(0)->text(1), QStringLiteral("2"));
        QCOMPARE(lista->topLevelItem(0)->text(2), QStringLiteral("tema 1, pregunta 1"));

        QVERIFY(escribir("c"));
        volver->click();
        QVERIFY(importar->isEnabled());
        QCOMPARE(importar->text(), QStringLiteral("Importar"));
        QCOMPARE(lista->topLevelItemCount(), 0);
    }

    // «Acerca de» habla del programa, que vale para cualquier asignatura: dice la versión, el banco abierto y la
    // configuración, pero no la asignatura del banco (esa va en el título de la ventana).
    void acercaDeNoDiceLaAsignatura()
    {
        QTemporaryDir dir;
        const QString ruta = dir.filePath(QStringLiteral("banco.db"));
        {
            BaseDatos bd;
            QVERIFY(crearBancoInventado(ruta));
            QVERIFY(bd.abrir(ruta));
            QVERIFY(bd.guardarAsignatura(QStringLiteral("Asignatura inventada")));
        }
        VentanaPrincipal v;
        v.show();
        QVERIFY(v.abrirBancoInicial(ruta));
        auto *acerca = conTexto<QAction>(&v, QStringLiteral("&Acerca de…"));
        QVERIFY(acerca);
        QString texto;
        responder(QStringLiteral("OK"), &texto);
        acerca->trigger();
        QVERIFY2(texto.contains(QStringLiteral("<h3>Generador de exámenes</h3>")), qPrintable(texto));
        QVERIFY2(texto.contains(QStringLiteral("<p>Versión ")), qPrintable(texto));
        QVERIFY2(texto.contains(QDir::toNativeSeparators(ruta).toHtmlEscaped()), qPrintable(texto));
        QVERIFY2(!texto.contains(QStringLiteral("Asignatura inventada")), qPrintable(texto));
        QVERIFY2(!texto.contains(QStringLiteral("Asignatura:")), qPrintable(texto));
    }

    // «Acerca de» dice también cuándo se compiló el programa y de qué commit, que el ejecutable pone en la aplicación
    // (las pruebas no los tienen: solo la versión).
    void acercaDeDiceLaCompilacion()
    {
        const QString version = VentanaPrincipal::versionYCompilacion();
        QVERIFY2(!version.isEmpty() && !version.contains(QLatin1Char('(')), qPrintable(version));
        const QDateTime fecha(QDate(2020, 2, 3), QTime(4, 5));
        const QString cuando = QLocale().toString(fecha, QLocale::ShortFormat);
        qApp->setProperty("fechaCompilacion", fecha);
        QCOMPARE(VentanaPrincipal::versionYCompilacion(), QStringLiteral("%1 (compilado el %2)").arg(version, cuando));
        qApp->setProperty("commitCompilacion", QStringLiteral("abc1234"));
        QCOMPARE(VentanaPrincipal::versionYCompilacion(),
                 QStringLiteral("%1 (compilado el %2, commit abc1234)").arg(version, cuando));

        VentanaPrincipal v;
        v.show();
        QString texto;
        responder(QStringLiteral("OK"), &texto);
        QVERIFY(QMetaObject::invokeMethod(&v, "acercaDe"));
        QVERIFY2(texto.contains(QStringLiteral("<p>Versión %1 (compilado el ").arg(version)), qPrintable(texto));
        QVERIFY2(texto.contains(QStringLiteral("commit abc1234")), qPrintable(texto));
        for (const char *propiedad : {"fechaCompilacion", "commitCompilacion"})
            qApp->setProperty(propiedad, QVariant());
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

    // Los colores salen de la paleta, y se leen: cada tinta de la interfaz, sobre el fondo claro y sobre el oscuro, y
    // las de los recuadros y las del papel sobre el suyo, con el contraste que se pide para un texto (4,5:1).
    void losColoresDeLaPaletaSeLeen()
    {
        // Contraste entre dos colores, de 1 a 21 (WCAG 2).
        auto luminancia = [](const QColor &c) {
            auto canal = [](qreal v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
            return 0.2126 * canal(c.redF()) + 0.7152 * canal(c.greenF()) + 0.0722 * canal(c.blueF());
        };
        auto contraste = [&](const QColor &a, const QColor &b) {
            const qreal la = luminancia(a), lb = luminancia(b);
            return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
        };
        const QList<QColor> claros{QColor(Qt::white), QColor(0xef, 0xef, 0xef)};
        const QList<QColor> oscuros{QColor(0x1e, 0x1e, 0x1e), QColor(0x3c, 0x3c, 0x3c)};
        for (Paleta::Tinta t : {Paleta::Secundario, Paleta::Inactiva, Paleta::Atencion, Paleta::Informacion, Paleta::Verificada,
                                Paleta::Correcta, Paleta::Error}) {
            for (const QColor &fondo : claros)
                QVERIFY2(contraste(Paleta::tinta(t, false), fondo) >= 4.5, qPrintable(Paleta::tinta(t, false).name()));
            for (const QColor &fondo : oscuros)
                QVERIFY2(contraste(Paleta::tinta(t, true), fondo) >= 4.5, qPrintable(Paleta::tinta(t, true).name()));
        }
        // Lo que no cuenta va más apagado que lo secundario, sin dejar de leerse.
        QVERIFY(contraste(Paleta::tinta(Paleta::Inactiva, false), Qt::white) < contraste(Paleta::tinta(Paleta::Secundario, false), Qt::white));
        QVERIFY(contraste(Paleta::tinta(Paleta::Inactiva, true), oscuros.first()) < contraste(Paleta::tinta(Paleta::Secundario, true), oscuros.first()));
        // Las preguntas anuladas de la lista del banco: en ese gris, y tachado solo el código, para que el enunciado
        // se lea.
        {
            ModeloPreguntas modelo;
            Pregunta anulada;
            anulada.estado = QStringLiteral("anulada");
            QVERIFY(!anulada.activa());
            modelo.establecer({anulada, Pregunta()});
            for (int c = 0; c < modelo.columnCount(); ++c) {
                QCOMPARE(modelo.data(modelo.index(0, c), Qt::ForegroundRole).value<QBrush>().color(), Paleta::tinta(Paleta::Inactiva));
                QCOMPARE(modelo.data(modelo.index(0, c), Qt::FontRole).value<QFont>().strikeOut(), c == ModeloPreguntas::ColCodigo);
                QVERIFY(!modelo.data(modelo.index(1, c), Qt::FontRole).isValid());
            }
        }
        // Los recuadros: «background: #…; color: #…».
        static const QRegularExpression colores(QStringLiteral("background: (#[0-9a-f]{6}); color: (#[0-9a-f]{6})"));
        for (const QString &estilo : {Paleta::fondoAtencion(), Paleta::fondoError(), Paleta::recuadroAtencion(), Paleta::recuadroInformacion()}) {
            const QRegularExpressionMatch m = colores.match(estilo);
            QVERIFY2(m.hasMatch(), qPrintable(estilo));
            QVERIFY2(contraste(QColor(m.captured(1)), QColor(m.captured(2))) >= 4.5, qPrintable(estilo));
        }
        QVERIFY(contraste(Paleta::colorFondoAtencion(), Paleta::colorTintaAtencion()) >= 4.5);
        // El papel.
        for (const char *c : {Paleta::kPapelCorrecta, Paleta::kPapelVerificada, Paleta::kPapelSinVerificar, Paleta::kPapelSecundario})
            QVERIFY2(contraste(QColor(QLatin1String(c)), Qt::white) >= 4.5, c);
        QVERIFY(contraste(QColor(Qt::black), QColor(QLatin1String(Paleta::kPapelFondoCodigo))) >= 4.5);
    }

    // Con el tema oscuro, las tintas de la interfaz pasan a sus tonos claros, y los exámenes y las fichas siguen
    // viéndose sobre papel blanco con tinta negra, que es con lo que cuentan sus colores.
    void conElTemaOscuro()
    {
        const QPalette antes = QApplication::palette();
        QVERIFY(!Paleta::oscura());
        QCOMPARE(Paleta::tinta(Paleta::Secundario), Paleta::tinta(Paleta::Secundario, false));
        QPalette oscura = antes;
        oscura.setColor(QPalette::Window, QColor(0x2b, 0x2b, 0x2b));
        oscura.setColor(QPalette::Base, QColor(0x1e, 0x1e, 0x1e));
        oscura.setColor(QPalette::WindowText, Qt::white);
        oscura.setColor(QPalette::Text, Qt::white);
        QApplication::setPalette(oscura);
        QVERIFY(Paleta::oscura());
        QCOMPARE(Paleta::tinta(Paleta::Secundario), Paleta::tinta(Paleta::Secundario, true));
        QVERIFY(Paleta::css(Paleta::Verificada).contains(Paleta::tinta(Paleta::Verificada, true).name()));
        {
            VisorAmpliable visor;
            QCOMPARE(visor.palette().color(QPalette::Base), QColor(Qt::white));
            QCOMPARE(visor.palette().color(QPalette::Text), QColor(Qt::black));
            // La lista del banco sí sigue el tema: su azul de «verificada» es el del fondo oscuro.
            ModeloPreguntas modelo;
            Pregunta verificada;
            verificada.verificada = true;
            verificada.enunciado = QStringLiteral("Enunciado inventado con `nombre`:");
            verificada.opciones = QStringList{QStringLiteral("El valor de `uno`."), QStringLiteral("Dos."), QStringLiteral("Tres."), QStringLiteral("Cuatro.")};
            modelo.establecer({verificada});
            // La lista y su texto emergente son texto normal: el código en línea va sin sus comillas invertidas.
            const QModelIndex celda = modelo.index(0, ModeloPreguntas::ColEnunciado);
            QCOMPARE(modelo.data(celda, Qt::DisplayRole).toString(), QStringLiteral("Enunciado inventado con nombre:"));
            QVERIFY(modelo.data(celda, Qt::ToolTipRole).toString().contains(QStringLiteral("a) El valor de uno.")));
            QVERIFY(!modelo.data(celda, Qt::ToolTipRole).toString().contains(QLatin1Char('`')));
            QCOMPARE(modelo.data(modelo.index(0, ModeloPreguntas::ColVerificada), Qt::ForegroundRole).value<QBrush>().color(),
                     Paleta::tinta(Paleta::Verificada, true));
        }
        QApplication::setPalette(antes);
        QVERIFY(!Paleta::oscura());
    }
};

QTEST_MAIN(PruebaInterfaz)
#include "prueba_interfaz.moc"
