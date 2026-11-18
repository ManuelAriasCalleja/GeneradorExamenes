#include "zip.h"

#include <QtEndian>
#include <array>

namespace {

void anadir16(QByteArray &a, quint16 v)
{
    char b[2];
    qToLittleEndian(v, b);
    a.append(b, 2);
}

void anadir32(QByteArray &a, quint32 v)
{
    char b[4];
    qToLittleEndian(v, b);
    a.append(b, 4);
}

// Fecha de los ficheros (en formato de MS-DOS): fija, para que el mismo documento dé siempre el mismo ZIP.
constexpr quint16 kHora = 0;
constexpr quint16 kFecha = (2020 - 1980) << 9 | 1 << 5 | 1;   // 1 de enero de 2020

} // namespace

quint32 EscritorZip::crc32(const QByteArray &datos)
{
    static const std::array<quint32, 256> tabla = [] {
        std::array<quint32, 256> t{};
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k)
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    quint32 c = 0xFFFFFFFFu;
    for (const char b : datos)
        c = tabla[(c ^ quint8(b)) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void EscritorZip::anadir(const QString &nombre, const QByteArray &datos, bool comprimir)
{
    const QByteArray nombreUtf8 = nombre.toUtf8();
    QByteArray guardados = datos;
    quint16 metodo = 0;   // tal cual
    if (comprimir && !datos.isEmpty()) {
        // qCompress da los datos en formato zlib, precedidos de su tamaño (4 bytes): sin eso, sin la cabecera de zlib
        // (2 bytes) y sin su suma de comprobación del final (4 bytes), queda el deflate que usa ZIP.
        const QByteArray zlib = qCompress(datos, 9);
        if (zlib.size() > 10 && zlib.size() - 10 < datos.size()) {
            guardados = zlib.mid(6, zlib.size() - 10);
            metodo = 8;   // deflate
        }
    }
    const quint32 crc = crc32(datos);
    const quint32 desplazamiento = quint32(m_ficheros.size());
    const quint16 indicadores = 1 << 11;   // nombre en UTF-8

    anadir32(m_ficheros, 0x04034b50);   // cabecera local
    anadir16(m_ficheros, 20);           // versión necesaria (2.0)
    anadir16(m_ficheros, indicadores);
    anadir16(m_ficheros, metodo);
    anadir16(m_ficheros, kHora);
    anadir16(m_ficheros, kFecha);
    anadir32(m_ficheros, crc);
    anadir32(m_ficheros, quint32(guardados.size()));
    anadir32(m_ficheros, quint32(datos.size()));
    anadir16(m_ficheros, quint16(nombreUtf8.size()));
    anadir16(m_ficheros, 0);            // campo extra
    m_ficheros += nombreUtf8;
    m_ficheros += guardados;

    anadir32(m_directorio, 0x02014b50);   // entrada del directorio central
    anadir16(m_directorio, 20);           // hecho por (2.0, MS-DOS)
    anadir16(m_directorio, 20);           // versión necesaria
    anadir16(m_directorio, indicadores);
    anadir16(m_directorio, metodo);
    anadir16(m_directorio, kHora);
    anadir16(m_directorio, kFecha);
    anadir32(m_directorio, crc);
    anadir32(m_directorio, quint32(guardados.size()));
    anadir32(m_directorio, quint32(datos.size()));
    anadir16(m_directorio, quint16(nombreUtf8.size()));
    anadir16(m_directorio, 0);            // campo extra
    anadir16(m_directorio, 0);            // comentario
    anadir16(m_directorio, 0);            // disco
    anadir16(m_directorio, 0);            // atributos internos
    anadir32(m_directorio, 0);            // atributos externos
    anadir32(m_directorio, desplazamiento);
    m_directorio += nombreUtf8;
    ++m_entradas;
}

QByteArray EscritorZip::terminar() const
{
    QByteArray zip = m_ficheros + m_directorio;
    anadir32(zip, 0x06054b50);   // fin del directorio central
    anadir16(zip, 0);            // este disco
    anadir16(zip, 0);            // disco del directorio
    anadir16(zip, quint16(m_entradas));
    anadir16(zip, quint16(m_entradas));
    anadir32(zip, quint32(m_directorio.size()));
    anadir32(zip, quint32(m_ficheros.size()));
    anadir16(zip, 0);            // comentario
    return zip;
}
