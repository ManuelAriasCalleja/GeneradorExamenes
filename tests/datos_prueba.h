#pragma once

// Datos inventados para las pruebas: en git no puede haber preguntas de ningún banco (ver CLAUDE.md).

#include "basedatos.h"
#include "pregunta.h"

#include <QBuffer>
#include <QImage>
#include <QMetaType>

Q_DECLARE_METATYPE(Pregunta)

// Banco de 'temas' temas con 'grupos' números cada uno, que cumple las pautas de estilo. Los múltiplos de 4 tienen
// tres variantes (a, b, c); los de 3 están marcados «old»; la T3-5 es pseudorrepetida con la T1-7, y la T2-6 tiene
// una opción «Todas las anteriores.». El id de cada tema es su número.
inline QVector<Pregunta> bancoInventado(int temas = 8, int grupos = 20)
{
    QVector<Pregunta> banco;
    int id = 1;
    for (int t = 1; t <= temas; ++t) {
        for (int n = 1; n <= grupos; ++n) {
            const QStringList variantes = n % 4 == 0 ? QStringList{"a", "b", "c"} : QStringList{QString()};
            for (const QString &v : variantes) {
                Pregunta p;
                p.id = id++;
                p.temaId = t;
                p.temaNumero = t;
                p.numero = n;
                p.variante = v;
                p.enunciado = QStringLiteral("Enunciado inventado de la pregunta T%1-%2%3:").arg(t).arg(n).arg(v);
                p.opciones = {QStringLiteral("Primera opción inventada."), QStringLiteral("Segunda opción inventada."),
                              QStringLiteral("Tercera opción inventada."), QStringLiteral("Cuarta opción inventada.")};
                p.correcta = (p.id * 7) % 4;
                const bool vease = t == 1 || t == 3 || t == 6 || t == 8;
                p.referencia = vease ? QStringLiteral("Véase la página %1 del libro de teoría.").arg(10 * t + n)
                                     : QStringLiteral("Pág. %1 libro (subsección %2.1.1).").arg(10 * t + n).arg(t);
                p.old = n % 3 == 0;
                if (t == 3 && n == 5)
                    p.pseudoCon = QStringLiteral("T1-7");
                if (t == 2 && n == 6)
                    p.opciones[3] = QStringLiteral("Todas las anteriores.");
                banco << p;
            }
        }
    }
    return banco;
}

// Banco pequeño de preguntas distintas entre sí (escritas para las pruebas), para buscar parecidas: las del banco
// inventado se parecen todas. Temas 1 a 4; el id de cada tema es su número.
inline QVector<Pregunta> bancoVariado()
{
    struct Datos
    {
        int tema, numero;
        const char *enunciado, *a, *b, *c, *d;
    };
    static const Datos datos[] = {
        {1, 1, "La memoria caché de un microprocesador sirve para:", "Guardar copias de los datos de uso frecuente.",
         "Ampliar la capacidad del disco duro.", "Refrigerar los núcleos del procesador.", "Conectar la impresora a la red."},
        {1, 2, "En un ordenador, las líneas de dirección del bus llevan:", "Posiciones de memoria.", "Instrucciones ya ejecutadas.",
         "La tensión de alimentación.", "Los colores de la pantalla."},
        {1, 3, "El reloj del sistema marca:", "El ritmo de las operaciones del procesador.", "La hora de encendido del equipo.",
         "El tiempo que dura la batería.", "La velocidad del ventilador."},
        {2, 1, "Con relación a los escáneres de sobremesa, su resolución se mide en:", "Puntos por pulgada.",
         "Hercios por segundo.", "Bits por color.", "Páginas por minuto."},
        {2, 2, "Un teclado con distribución QWERTY recibe ese nombre por:", "Las letras de su fila superior.",
         "El apellido de su inventor.", "El tipo de conector que usa.", "El idioma para el que se diseñó."},
        {3, 1, "Con relación a la gestión de procesos, uno que está en espera:", "Aguarda a que ocurra un suceso.",
         "Ocupa siempre el procesador.", "Ya ha terminado su ejecución.", "No tiene memoria asignada."},
        {3, 2, "Un sistema de ficheros jerárquico organiza los archivos en:", "Un árbol de directorios.", "Una sola lista plana.",
         "Varias particiones sin nombre.", "Sectores numerados al azar."},
        {4, 1, "En una hoja de cálculo, una referencia absoluta a una celda:", "No cambia al copiar la fórmula.",
         "Cambia de fila al copiarla hacia abajo.", "Solo puede apuntar a otra hoja.", "Se borra al guardar el libro."},
        {4, 2, "En un modelo relacional, la clave primaria de una tabla:", "Identifica cada fila sin repetirse.",
         "Cifra el contenido de la tabla.", "Ordena las columnas por tamaño.", "Permite valores repetidos y nulos."},
    };
    QVector<Pregunta> banco;
    int id = 1;
    for (const Datos &d : datos) {
        Pregunta p;
        p.id = id++;
        p.temaId = d.tema;
        p.temaNumero = d.tema;
        p.numero = d.numero;
        p.enunciado = QString::fromUtf8(d.enunciado);
        p.opciones = {QString::fromUtf8(d.a), QString::fromUtf8(d.b), QString::fromUtf8(d.c), QString::fromUtf8(d.d)};
        p.correcta = 0;
        p.referencia = d.tema % 2 ? QStringLiteral("Véase la página %1 del libro de teoría.").arg(10 * d.tema + d.numero)
                                  : QStringLiteral("Pág. %1 libro (subsección %2.1.1).").arg(10 * d.tema + d.numero).arg(d.tema);
        banco << p;
    }
    return banco;
}

inline QVector<Tema> temasInventados(int temas = 8)
{
    QVector<Tema> res;
    for (int t = 1; t <= temas; ++t) {
        Tema tema;
        tema.id = t;
        tema.numero = t;
        tema.titulo = QStringLiteral("Tema inventado %1").arg(t);
        res << tema;
    }
    return res;
}

// Llena una base de datos recién creada con 'temas' temas inventados y las preguntas dadas. Los temas se crean
// nuevos, con los id 1, 2, 3..., que son los que llevan las preguntas inventadas.
inline bool llenarBaseDatos(BaseDatos &bd, int temas, const QVector<Pregunta> &preguntas)
{
    for (Tema t : temasInventados(temas)) {
        t.id = 0;
        if (!bd.guardarTema(t) || t.id != t.numero)
            return false;
    }
    for (Pregunta p : preguntas) {
        p.id = 0;
        if (!bd.guardarPregunta(p))
            return false;
    }
    return true;
}

// Una imagen pequeña en PNG, para las preguntas con dibujo.
inline QByteArray imagenInventada()
{
    QImage img(40, 20, QImage::Format_RGB32);
    img.fill(Qt::blue);
    QByteArray datos;
    QBuffer buffer(&datos);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return datos;
}
