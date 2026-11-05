#!/usr/bin/env python3
"""
Convierte la base de datos de preguntas en Word (FI_preguntas_examen_base_de_datos_total_verXX.docx)
al fichero JSON que importa el programa GeneradorExamenes.

Uso:
    python3 importar_docx.py ENTRADA.docx SALIDA.json [--imagenes DIR]

Formato del texto de los enunciados (el mismo que usa el editor del programa):
  - Texto normal; los saltos de línea se respetan.
  - Bloques de código entre líneas ``` (se muestran en letra monoespaciada).
  - Filas de tabla en líneas que empiezan por '|'  (| celda | celda |).
Las imágenes (diagramas) se buscan en DIR con el nombre T<tema>_<código>.png, p. ej. T4_14a.png.
"""
import argparse
import base64
import json
import os
import re
import sys
import zipfile
import xml.etree.ElementTree as ET

W = '{http://schemas.openxmlformats.org/wordprocessingml/2006/main}'
M = '{http://schemas.openxmlformats.org/officeDocument/2006/math}'
MC = '{http://schemas.openxmlformats.org/markup-compatibility/2006}'

SKIP_TAGS = {W + 'drawing', W + 'pict', MC + 'AlternateContent', W + 'delText', W + 'instrText'}


def leer_numeracion(z):
    """Devuelve {numId: {ilvl: (numFmt, lvlText)}}."""
    try:
        root = ET.fromstring(z.read('word/numbering.xml'))
    except KeyError:
        return {}
    abstract = {}
    for a in root.findall(W + 'abstractNum'):
        niveles = {}
        for lvl in a.findall(W + 'lvl'):
            nf = lvl.find(W + 'numFmt')
            lt = lvl.find(W + 'lvlText')
            niveles[lvl.get(W + 'ilvl')] = (nf.get(W + 'val') if nf is not None else None,
                                            lt.get(W + 'val') if lt is not None else None)
        abstract[a.get(W + 'abstractNumId')] = niveles
    nums = {}
    for n in root.findall(W + 'num'):
        aid = n.find(W + 'abstractNumId').get(W + 'val')
        nums[n.get(W + 'numId')] = abstract.get(aid, {})
    return nums


def omml_texto(e):
    """Convierte (de forma muy simple) una fórmula OMML a texto: fracciones como a/b."""
    if e.tag == M + 't':
        return e.text or ''
    if e.tag == M + 'f':
        num = e.find(M + 'num')
        den = e.find(M + 'den')
        return '%s/%s' % (''.join(omml_texto(c) for c in num) if num is not None else '',
                          ''.join(omml_texto(c) for c in den) if den is not None else '')
    return ''.join(omml_texto(c) for c in e)


class Parrafo:
    def __init__(self, texto, num=None, dibujo=False):
        self.texto = texto
        self.num = num          # (numId, ilvl) si es un párrafo numerado
        self.dibujo = dibujo    # contiene formas/dibujos


def texto_parrafo(p):
    """Texto visible del párrafo: tabs como \t, ignora códigos de campo y dibujos."""
    partes = []
    estado = {'campo': 0, 'dibujo': False}

    def visitar(e):
        tag = e.tag
        if tag in (W + 'drawing', W + 'pict') or tag == MC + 'AlternateContent':
            estado['dibujo'] = True
            return
        if tag == W + 'fldChar':
            t = e.get(W + 'fldCharType')
            if t == 'begin':
                estado['campo'] += 1
            elif t in ('separate', 'end') and estado['campo'] > 0:
                estado['campo'] -= 1
            return
        if estado['campo'] > 0 and tag in (M + 'oMath', W + 't', W + 'tab', W + 'br', W + 'cr'):
            return  # código de campo (no visible)
        if tag == M + 'oMath':
            partes.append(omml_texto(e))
            return
        if tag == W + 't':
            partes.append(e.text or '')
        elif tag == W + 'tab':
            partes.append('\t')
        elif tag in (W + 'br', W + 'cr'):
            partes.append('\n')
        elif tag in SKIP_TAGS:
            return
        for c in e:
            visitar(c)

    for c in p:
        if c.tag == W + 'pPr':
            continue
        visitar(c)
    numpr = p.find(W + 'pPr/' + W + 'numPr')
    num = None
    if numpr is not None:
        nid = numpr.find(W + 'numId')
        ilvl = numpr.find(W + 'ilvl')
        num = (nid.get(W + 'val') if nid is not None else '0', ilvl.get(W + 'val') if ilvl is not None else '0')
    return Parrafo(''.join(partes), num, estado['dibujo'])


def bloques(z):
    """Recorre el cuerpo del documento produciendo Parrafo o ('tabla', filas)."""
    root = ET.fromstring(z.read('word/document.xml'))
    body = root.find(W + 'body')
    for child in body:
        if child.tag == W + 'p':
            yield texto_parrafo(child)
        elif child.tag == W + 'tbl':
            filas = []
            for tr in child.iter(W + 'tr'):
                celdas = []
                for tc in tr.findall(W + 'tc'):
                    celdas.append(' '.join(texto_parrafo(p).texto.strip() for p in tc.findall(W + 'p')).strip())
                filas.append(celdas)
            yield ('tabla', filas)


RE_TEMA = re.compile(r'^\s*Tema\s+(\d+)\s*:\s*(.+?)\s*$')
RE_CABECERA = re.compile(r'^\s*(\d+)(.{0,60}?)\.-\s*(.*)$', re.S)
RE_RESPUESTA = re.compile(r'^\s*Respuesta\s+correcta\s*:?\s*([A-Da-d])\b[\s.,;:]*(.*)$', re.S)
RE_OPCION_INICIO = re.compile(r'^\s*[a-d]\)')
RE_MARCA = re.compile(r'(?:(?<=^)|(?<=\s))([a-d])\)\s*')
RE_NOTA_ENUNCIADO = re.compile(r'^(ANULADA|MEJORAR\.?|REFINAR|DEFINIR\s+IPC)\s+')


def limpiar(s):
    s = s.replace(' ', ' ').replace('ﬁ', 'fi').replace('ﬂ', 'fl')
    s = re.sub(r'[ \t]+', ' ', s)
    return s.strip()


def analizar_etiqueta(numero, resto):
    """'22aold' -> variante 'a', old; detecta ANULADA, OBSOLETA, MEJORAR, PSEUDORREPETIDA..."""
    info = {'numero': int(numero), 'variante': '', 'old': False, 'estado': 'activa',
            'revisar': False, 'notas': [], 'pseudo_con': None}
    r = resto
    m = re.search(r'pseudo\w*\s*(?:con\s*)?T(?:EMA)?\s*(\d+)\D+?(\d+)', r, re.I)
    if m:
        info['pseudo_con'] = 'T%s-%s' % (m.group(1), m.group(2))
        info['notas'].append('Pseudorrepetida con %s' % info['pseudo_con'])
        r = r[:m.start()] + r[m.end():]
    for palabra, accion in (('anulada', 'anulada'), ('obsoleta', 'obsoleta'), ('mejorar', 'revisar'), ('old', 'old')):
        if re.search(palabra, r, re.I):
            r = re.sub(palabra, '', r, flags=re.I)
            if accion == 'old':
                info['old'] = True
            elif accion == 'revisar':
                info['revisar'] = True
                info['notas'].append('MEJORAR')
            else:
                info['estado'] = accion
    r = r.strip()
    if re.fullmatch(r'[a-z]?', r):
        info['variante'] = r
    else:
        info['notas'].append('Etiqueta no reconocida: ' + resto)
    return info


def separar_opciones(texto):
    """'a) X.\tc) Y.' -> {'a': 'X.', 'c': 'Y.'}"""
    marcas = list(RE_MARCA.finditer(texto))
    res = {}
    for i, m in enumerate(marcas):
        fin = marcas[i + 1].start() if i + 1 < len(marcas) else len(texto)
        res[m.group(1)] = limpiar(texto[m.end():fin])
    return res


def es_texto_final(linea):
    """Última línea del enunciado tras un bloque de código: texto normal que suele acabar en ':'.
    Una sola palabra cuenta como texto si empieza por mayúscula («Declara:»); en minúscula sería
    una etiqueta de código («default:»)."""
    s = linea.strip()
    return (s.endswith(':') and re.match(r'^[A-Za-zÁÉÍÓÚáéíóúñÑ¿]', s) is not None
            and (' ' in s or s[0].isupper()))


def es_texto_intermedio(linea):
    """Frase entre dos tablas o bloques («y el resultado queda así:»): varias palabras, acaba en ':'
    y no tiene caracteres propios del código."""
    s = linea.strip()
    return (s.endswith(':') and ' ' in s and re.match(r'^[A-Za-zÁÉÍÓÚáéíóúñÑ¿]', s) is not None
            and not re.search(r'[;{}()=<>"\[\]]', s))


def construir_enunciado(primera, extras):
    """Combina la primera línea con los párrafos/tablas adicionales en el formato del programa."""
    lineas = [primera]
    codigo = []

    def volcar_codigo():
        if codigo:
            lineas.append('```')
            lineas.extend(codigo)
            lineas.append('```')
            codigo.clear()

    for k, e in enumerate(extras):
        if isinstance(e, tuple):  # tabla
            volcar_codigo()
            for fila in e[1]:
                lineas.append('| ' + ' | '.join(c.strip() for c in fila) + ' |')
            continue
        t = e.rstrip()
        if not t.strip():
            continue
        ultima = (k == len(extras) - 1)
        if (ultima and es_texto_final(t)) or (not ultima and not codigo and es_texto_intermedio(t)):
            volcar_codigo()
            lineas.append(limpiar(t))
        else:
            codigo.append(t.strip())
    volcar_codigo()
    return '\n'.join(lineas)


def importar(ruta_docx, dir_imagenes):
    z = zipfile.ZipFile(ruta_docx)
    numeracion = leer_numeracion(z)
    temas = []
    tema = None
    preg = None
    fase = None          # 'enunciado', 'opciones', 'respuesta'
    extras = []
    contadores = {}
    avisos = []

    def cerrar():
        nonlocal preg
        if preg is None:
            return
        preg['enunciado'] = construir_enunciado(preg.pop('_primera'), extras)
        preg['explicacion'] = '\n'.join(preg.pop('_explicacion')).strip()
        opciones = preg.pop('_opciones')
        faltan = [l for l in 'abcd' if not opciones.get(l)]
        sobran = [l for l in opciones if l not in 'abcd']
        if faltan or sobran:
            avisos.append('T%d-%s: opciones incompletas (faltan %s)' % (tema['numero'], preg['codigo'], faltan))
        preg['opciones'] = [opciones.get(l, '') for l in 'abcd']
        if not preg.get('correcta'):
            avisos.append('T%d-%s: sin respuesta correcta' % (tema['numero'], preg['codigo']))
        img = os.path.join(dir_imagenes or '', 'T%d_%d%s.png' % (tema['numero'], preg['numero'], preg['variante']))
        if preg.pop('_dibujo'):
            if dir_imagenes and os.path.exists(img):
                with open(img, 'rb') as f:
                    preg['imagen'] = base64.b64encode(f.read()).decode('ascii')
            else:
                avisos.append('T%d-%s: tiene un dibujo y no se encontró %s' % (tema['numero'], preg['codigo'], img))
        preg['notas'] = '; '.join(preg['notas'])
        if any(p['codigo'] == preg['codigo'] for p in tema['preguntas']):
            avisos.append('T%d-%s: código repetido' % (tema['numero'], preg['codigo']))
        tema['preguntas'].append(preg)
        preg = None

    for b in bloques(z):
        if isinstance(b, tuple):
            if preg is not None and fase == 'enunciado':
                extras.append(b)
            else:
                avisos.append('Tabla fuera de un enunciado (tras %s)' % (preg or {}).get('codigo'))
            continue
        texto = b.texto
        if b.num and b.num[0] != '0':
            fmt = numeracion.get(b.num[0], {}).get(b.num[1], (None, None))
            if fmt[0] == 'lowerLetter':
                clave = b.num
                n = contadores.get(clave, 0)
                contadores[clave] = n + 1
                texto = '%s) %s' % ('abcdefghij'[n % 10], texto)
        plano = texto.strip()

        m = RE_TEMA.match(plano)
        if m:
            cerrar()
            titulo = m.group(2)
            autor = ''
            ma = re.match(r'^(.*?)\s+([A-ZÁÉÍÓÚÑ][A-Za-zÁÉÍÓÚÑ]{1,4})$', titulo)
            if ma and ma.group(2).upper() == ma.group(2).replace('h', 'H'):
                titulo, autor = ma.group(1), ma.group(2)
            tema = {'numero': int(m.group(1)), 'titulo': titulo.strip(), 'autor': autor, 'preguntas': []}
            temas.append(tema)
            fase = None
            continue
        if tema is None:
            continue

        m = RE_CABECERA.match(texto)
        if m and not RE_OPCION_INICIO.match(texto):
            cerrar()
            info = analizar_etiqueta(m.group(1), m.group(2))
            enun = limpiar(m.group(3))
            mn = RE_NOTA_ENUNCIADO.match(enun)
            while mn:
                nota = mn.group(1).rstrip('.')
                if nota == 'ANULADA':
                    info['estado'] = 'anulada'
                else:
                    info['revisar'] = True
                info['notas'].append(nota)
                enun = enun[mn.end():]
                mn = RE_NOTA_ENUNCIADO.match(enun)
            preg = dict(info)
            preg['codigo'] = '%d%s' % (info['numero'], info['variante'])
            preg['etiqueta_original'] = (m.group(1) + m.group(2)).strip()
            preg['_primera'] = enun
            preg['_opciones'] = {}
            preg['_explicacion'] = []
            preg['_dibujo'] = False
            preg['correcta'] = ''
            preg['referencia'] = ''
            preg['imagen'] = None
            extras = []
            contadores = {}
            fase = 'enunciado'
            continue
        if preg is None:
            if plano:
                avisos.append('Texto suelto ignorado en tema %d: %r' % (tema['numero'], plano[:60]))
            continue

        if b.dibujo and fase == 'enunciado':
            preg['_dibujo'] = True
        if not plano:
            continue

        m = RE_RESPUESTA.match(plano)
        if m:
            preg['correcta'] = m.group(1).upper()
            ref = limpiar(m.group(2))
            ref = re.sub(r'^%s[.,]\s*' % preg['correcta'], '', ref)   # "B. B. Véase..." duplicado
            preg['referencia'] = ref[:1].upper() + ref[1:]
            fase = 'respuesta'
            continue
        if RE_OPCION_INICIO.match(texto) and fase in ('enunciado', 'opciones'):
            for letra, valor in separar_opciones(texto.strip()).items():
                if letra in preg['_opciones']:
                    avisos.append('T%d-%s: opción %s repetida' % (tema['numero'], preg['codigo'], letra))
                preg['_opciones'][letra] = valor
            fase = 'opciones'
            continue
        if fase == 'enunciado':
            extras.append(texto)
        elif fase == 'respuesta':
            preg['_explicacion'].append(limpiar(texto))
        else:
            avisos.append('T%d-%s: línea inesperada %r' % (tema['numero'], preg['codigo'], plano[:60]))
    cerrar()
    return temas, avisos


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('docx')
    ap.add_argument('json')
    ap.add_argument('--imagenes', help='directorio con los diagramas T<tema>_<codigo>.png')
    ap.add_argument('--asignatura', default='Fundamentos de la informática',
                    help='nombre de la asignatura; el programa lo toma si el banco aún no tiene')
    a = ap.parse_args()
    temas, avisos = importar(a.docx, a.imagenes)
    salida = {
        'asignatura': a.asignatura,
        'origen': os.path.basename(a.docx),
        'temas': temas,
    }
    with open(a.json, 'w', encoding='utf-8') as f:
        json.dump(salida, f, ensure_ascii=False, indent=1)
    total = sum(len(t['preguntas']) for t in temas)
    print('%d temas, %d preguntas -> %s' % (len(temas), total, a.json))
    for t in temas:
        activas = sum(1 for p in t['preguntas'] if p['estado'] == 'activa')
        grupos = len({p['numero'] for p in t['preguntas'] if p['estado'] == 'activa'})
        print('  Tema %d (%s, %s): %d preguntas, %d activas, %d números distintos'
              % (t['numero'], t['titulo'], t['autor'], len(t['preguntas']), activas, grupos))
    for av in avisos:
        print('AVISO:', av, file=sys.stderr)
    return 0


if __name__ == '__main__':
    sys.exit(main())
