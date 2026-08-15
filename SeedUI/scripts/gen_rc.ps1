# Gera SeedUI_icons.rc: uma entrada RCDATA por SVG de assets/icons.
# Uso: powershell -NoProfile -ExecutionPolicy Bypass -File gen_rc.ps1
param(
    [string]$ProjectDir = "D:\Vamos testar as otimizacoes\SeedEngine-main\SeedUI"
)

$iconsDir = Join-Path $ProjectDir "assets\icons"
$outFile = Join-Path $ProjectDir "SeedUI_icons.rc"

$lines = @()
$lines += "// GERADO POR scripts\gen_rc.ps1 - nao edite manualmente."
$lines += "// Uma entrada RCDATA por icone: o nome do recurso e o nome do arquivo."
$files = Get-ChildItem -LiteralPath $iconsDir -Filter *.svg | Sort-Object Name
foreach ($f in $files) {
    # Barras normais no caminho: o RC interpreta \a, \t etc. como escapes C.
    $lines += ('"{0}" RCDATA "assets/icons/{0}"' -f $f.Name)
}
$lines += ""
Set-Content -LiteralPath $outFile -Value $lines -Encoding Ascii
Write-Host ("OK: {0} ({1} icones)" -f $outFile, $files.Count)
