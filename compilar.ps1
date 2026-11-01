<#
.SYNOPSIS
Compila el Generador de exámenes FI en Windows, pasa las pruebas automáticas y lo instala para el usuario actual.

.DESCRIPTION
Busca Qt 6 (en C:\Qt o en la carpeta Qt del usuario, si no se indica con -Qt), compila en Release en build\ y, si
las pruebas pasan, lo instala en %LOCALAPPDATA%\Programs\GeneradorExamenesFI con las bibliotecas de Qt que necesita
(las copia windeployqt) y crea sus accesos directos en el menú Inicio y en el escritorio. No hace falta ser
administrador.

Con un Qt para MinGW se usan el compilador, CMake y Ninja que instala el instalador de Qt (C:\Qt\Tools); con uno para
Visual Studio (msvc), el Visual Studio instalado.

Si PowerShell no deja ejecutar scripts, abra compilar.bat, que lo llama con los mismos parámetros.

.PARAMETER Qt
Carpeta del Qt que se usa, la que contiene bin\ y lib\ (por ejemplo, C:\Qt\6.8.3\mingw_64).

.PARAMETER Destino
Carpeta donde se instala el programa.

.PARAMETER SinPruebas
No compila ni pasa las pruebas automáticas.

.PARAMETER SinInstalar
Solo compila (y pasa las pruebas).

.PARAMETER SinEscritorio
No pone el acceso directo en el escritorio (si ya había uno, lo actualiza).

.PARAMETER Desinstalar
Quita el programa instalado y sus accesos directos. La configuración y los bancos de preguntas no se tocan.

.EXAMPLE
.\compilar.ps1

.EXAMPLE
.\compilar.ps1 -Qt C:\Qt\6.8.3\mingw_64 -SinEscritorio
#>
[CmdletBinding()]
param(
    [string]$Qt,
    [string]$Destino = (Join-Path $env:LOCALAPPDATA 'Programs\GeneradorExamenesFI'),
    [switch]$SinPruebas,
    [switch]$SinInstalar,
    [switch]$SinEscritorio,
    [switch]$Desinstalar
)

$ErrorActionPreference = 'Stop'
$proyecto = $PSScriptRoot
$compilacion = Join-Path $proyecto 'build'
$ejecutable = 'GeneradorExamenesFI.exe'
$nombreAcceso = 'Generador de exámenes FI.lnk'
$accesoMenu = Join-Path ([Environment]::GetFolderPath('Programs')) $nombreAcceso
$accesoEscritorio = Join-Path ([Environment]::GetFolderPath('Desktop')) $nombreAcceso

function Paso([string]$texto) { Write-Host "== $texto" -ForegroundColor Cyan }

# Ejecuta un programa y para si falla.
function Ejecutar([string]$programa, [string[]]$argumentos) {
    & $programa @argumentos
    if ($LASTEXITCODE -ne 0) { throw "Ha fallado: $programa $($argumentos -join ' ')" }
}

# Quita la instalación anterior, solo si la carpeta es de verdad la del programa.
function Quitar-Instalacion {
    if (Test-Path (Join-Path $Destino $ejecutable)) {
        Remove-Item -Recurse -Force $Destino
    } elseif (Test-Path $Destino) {
        throw "$Destino no parece una instalación del programa (no tiene $ejecutable): no se toca."
    }
}

if ($Desinstalar) {
    Quitar-Instalacion
    Remove-Item -Force -ErrorAction SilentlyContinue $accesoMenu, $accesoEscritorio
    Write-Host "Desinstalado. La configuración (%APPDATA%\UNED-FI) y los bancos de preguntas no se han tocado."
    exit 0
}

# El Qt más reciente de C:\Qt o de %USERPROFILE%\Qt, prefiriendo el de MinGW (el instalador de Qt trae su compilador).
function Buscar-Qt {
    foreach ($raiz in 'C:\Qt', (Join-Path $env:USERPROFILE 'Qt')) {
        if (-not (Test-Path $raiz)) { continue }
        $versiones = Get-ChildItem $raiz -Directory | Where-Object { $_.Name -match '^6\.\d+(\.\d+)?$' } |
            Sort-Object { [version]$_.Name } -Descending
        foreach ($version in $versiones) {
            $kits = Get-ChildItem $version.FullName -Directory |
                Where-Object { $_.Name -match '^(mingw|llvm-mingw|msvc)' -and $_.Name -notmatch 'arm64' -and
                               (Test-Path (Join-Path $_.FullName 'lib\cmake\Qt6\Qt6Config.cmake')) } |
                Sort-Object { if ($_.Name -like 'mingw*') { 0 } elseif ($_.Name -like 'llvm-mingw*') { 1 } else { 2 } }
            if ($kits) { return @($kits)[0].FullName }
        }
    }
    return $null
}

# La herramienta más reciente de C:\Qt\Tools cuyo nombre empieza por $patron (la carpeta bin, si la tiene).
function Buscar-Herramienta([string]$herramientas, [string]$patron) {
    if (-not (Test-Path $herramientas)) { return $null }
    $carpeta = Get-ChildItem $herramientas -Directory -Filter $patron | Sort-Object Name -Descending | Select-Object -First 1
    if (-not $carpeta) { return $null }
    $bin = Join-Path $carpeta.FullName 'bin'
    if (Test-Path $bin) { return $bin }
    return $carpeta.FullName
}

if ($Qt) {
    $Qt = (Resolve-Path $Qt).Path
} else {
    $Qt = Buscar-Qt
    if (-not $Qt) {
        throw "No se encuentra Qt 6 en C:\Qt. Instálelo (con el instalador de Qt, con el compilador MinGW y, si se " +
              "quiere leer los PDF de los exámenes anteriores, Qt PDF) o indique su carpeta con -Qt."
    }
}
if (-not (Test-Path (Join-Path $Qt 'lib\cmake\Qt6\Qt6Config.cmake'))) {
    throw "$Qt no es una carpeta de Qt 6 (falta lib\cmake\Qt6\Qt6Config.cmake)."
}
Write-Host "Qt: $Qt"
$kit = Split-Path $Qt -Leaf
$herramientas = Join-Path (Split-Path (Split-Path $Qt)) 'Tools'

# Las bibliotecas de Qt, para las pruebas; y las herramientas que trae el instalador de Qt, si no hay otras.
$rutas = @(Join-Path $Qt 'bin')
if ($kit -like 'llvm-mingw*') {
    $rutas += Buscar-Herramienta $herramientas 'llvm-mingw*'
} elseif ($kit -like 'mingw*') {
    $rutas += Buscar-Herramienta $herramientas 'mingw*'
}
$rutas += Buscar-Herramienta $herramientas 'Ninja'
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $rutas += Buscar-Herramienta $herramientas 'CMake*' }
$env:PATH = (($rutas | Where-Object { $_ }) -join ';') + ';' + $env:PATH

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "No se encuentra CMake. Instálelo con el instalador de Qt (Qt → Developer and Designer Tools → CMake) o desde cmake.org."
}

# El generador, solo la primera vez (después, CMake usa el de build\).
$configurar = @('-S', $proyecto, '-B', $compilacion, "-DCMAKE_PREFIX_PATH=$Qt", '-DCMAKE_BUILD_TYPE=Release',
                "-DPRUEBAS=$(if ($SinPruebas) { 'OFF' } else { 'ON' })")
if (-not (Test-Path (Join-Path $compilacion 'CMakeCache.txt'))) {
    if ($kit -like 'msvc*') {
        $configurar += @('-A', 'x64')            # el Visual Studio más reciente que encuentre CMake
    } elseif (Get-Command ninja -ErrorAction SilentlyContinue) {
        $configurar += @('-G', 'Ninja')
    } else {
        $configurar += @('-G', 'MinGW Makefiles')
    }
}

Paso 'Configurando'
Ejecutar cmake $configurar

Paso 'Compilando'
Ejecutar cmake @('--build', $compilacion, '--config', 'Release', '--parallel')

if (-not $SinPruebas) {
    Paso 'Pasando las pruebas'
    Ejecutar ctest @('--test-dir', $compilacion, '-C', 'Release', '--output-on-failure')
}

if ($SinInstalar) {
    Write-Host "Compilado en $compilacion."
    exit 0
}

Paso "Instalando en $Destino"
Quitar-Instalacion
Ejecutar cmake @('--install', $compilacion, '--config', 'Release', '--prefix', $Destino)

# Accesos directos: el del menú Inicio siempre; el del escritorio, salvo que no se quiera (si ya había uno, se actualiza).
$shell = New-Object -ComObject WScript.Shell
$accesos = @($accesoMenu)
if (-not $SinEscritorio -or (Test-Path $accesoEscritorio)) { $accesos += $accesoEscritorio }
$programa = Join-Path $Destino $ejecutable
foreach ($ruta in $accesos) {
    $acceso = $shell.CreateShortcut($ruta)
    $acceso.TargetPath = $programa
    $acceso.WorkingDirectory = $Destino
    $acceso.IconLocation = "$programa,0"
    $acceso.Description = 'Exámenes tipo test de Fundamentos de Informática (Curso de Acceso, UNED)'
    $acceso.Save()
}

Write-Host ''
Write-Host "Instalado: $programa" -ForegroundColor Green
Write-Host "Acceso directo «Generador de exámenes FI» en el menú Inicio$(if ($accesos.Count -gt 1) { ' y en el escritorio' })."
