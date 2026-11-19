# Daily Steven - UN SOLO COMANDO para compilar en Windows usando WSL.
#
#   powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1          # compila (rápido si solo cambias textos/C)
#   powershell -ExecutionPolicy Bypass -File scripts\build-wsl.ps1 -Clean   # regenera todo desde cero
#
# Qué hace: sincroniza ESTA carpeta con una copia en WSL (finales LF), regenera la
# expansion si hace falta, compila y deja la ROM en  web\game\dailymaximo.gba.gz
# y la huella en  web\game\BUILD-INFO.txt  (para saber siempre qué versión estás probando).
param(
    [switch]$Clean,
    [switch]$Debug,   # ROM de pruebas: semilla 900001 = derrota segura, 900002 = victoria segura (?seed=900001&debug=1)
    [string]$Distro = "Ubuntu"
)
$ErrorActionPreference = "Stop"

$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$wslRepo = (& wsl -d $Distro -- wslpath -a ($repo -replace "\\", "/")).Trim()
if (-not $wslRepo) { throw "No encuentro la distro WSL '$Distro'. Pásala con -Distro NOMBRE." }

$args2 = @("-d", $Distro, "--", "bash", "$wslRepo/scripts/wsl-build.sh", $wslRepo)
if ($Clean) { $args2 += "--clean" }
if ($Debug) { $args2 += "--debug" }

$t0 = Get-Date
& wsl @args2
if ($LASTEXITCODE -ne 0) { throw "La compilación falló (código $LASTEXITCODE). Mira el mensaje de arriba." }

$gz = Join-Path $repo ($(if ($Debug) { "web\game\dailymaximo-debug.gba.gz" } else { "web\game\dailymaximo.gba.gz" }))
Write-Host ""
Write-Host ("ROM lista: {0}  ({1:N0} bytes, {2:HH:mm:ss})" -f $gz, (Get-Item $gz).Length, (Get-Item $gz).LastWriteTime)
Write-Host ("Tiempo total: {0:mm\:ss}" -f ((Get-Date) - $t0))
Write-Host "Para jugar:  node scripts/serve.mjs   y abre http://localhost:8080  (recarga con Ctrl+F5)"
