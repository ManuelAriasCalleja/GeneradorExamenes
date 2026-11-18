#pragma once

#include <QByteArray>
#include <QString>

// Fichero ZIP sencillo, para empaquetar los documentos de Word (.docx), que son un ZIP con XML e imágenes: sin cifrado,
// sin ZIP64 (hasta 4 GB) y con los nombres en UTF-8. Las entradas se comprimen con deflate (con la zlib de Qt) o se
// guardan tal cual.
class EscritorZip
{
public:
    // Añade un fichero; 'nombre' es su ruta dentro del ZIP, con «/» («word/document.xml»).
    void anadir(const QString &nombre, const QByteArray &datos, bool comprimir = true);
    // El ZIP con lo añadido.
    QByteArray terminar() const;

    // CRC-32 (el de ZIP y PNG) de los datos.
    static quint32 crc32(const QByteArray &datos);

private:
    QByteArray m_ficheros;     // cabeceras locales y datos
    QByteArray m_directorio;   // directorio central
    int m_entradas = 0;
};
