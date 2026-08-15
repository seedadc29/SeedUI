# Gera seedui.ico (multi-tamanho) a partir de SEEDui.png
# Uso: powershell -NoProfile -ExecutionPolicy Bypass -File gen_ico.ps1
param(
    [string]$Src = "D:\Vamos testar as otimizacoes\SeedEngine-main\SEEDui.png",
    [string]$Out = "D:\Vamos testar as otimizacoes\SeedEngine-main\SeedUI\seedui.ico"
)

Add-Type -AssemblyName System.Drawing

$img = [System.Drawing.Image]::FromFile($Src)
if ($null -eq $img) { Write-Host "ERRO: nao consegui abrir a imagem."; exit 1 }
$imgW = $img.Width
$imgH = $img.Height
Write-Host ("Fonte: {0}x{1} - {2}" -f $imgW, $imgH, $Src)

# Tamanhos que o Windows usa: 16 (título), 24/32 (explorer/toolbar),
# 48 (atalhos), 64/128 (pastas), 256 (área de trabalho / Alt+Tab).
$sizes = @(16, 24, 32, 48, 64, 128, 256)
$entries = New-Object System.Collections.ArrayList

foreach ($s in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($s, $s, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $scale = [Math]::Min($s / $imgW, $s / $imgH)
    $w = [int][Math]::Max(1, [Math]::Round($imgW * $scale))
    $h = [int][Math]::Max(1, [Math]::Round($imgH * $scale))
    $x = [int](($s - $w) / 2)
    $y = [int](($s - $h) / 2)
    $g.DrawImage($img, $x, $y, $w, $h)
    $g.Dispose()

    if ($s -ge 64) {
        # Tamanhos grandes: PNG comprimido dentro do ICO (suportado pelo Windows)
        $ms = New-Object System.IO.MemoryStream
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $null = $entries.Add(@{ Size = $s; Png = $true; Bytes = $ms.ToArray() })
        $ms.Dispose()
    } else {
        # Tamanhos pequenos: DIB 32bpp (BGRA) + máscara AND - compatibilidade máxima
        $stride = $s * 4
        $xor = New-Object byte[] ($stride * $s)
        for ($yy = 0; $yy -lt $s; $yy++) {
            $dstRow = ($s - 1 - $yy) * $stride
            for ($xx = 0; $xx -lt $s; $xx++) {
                $c = $bmp.GetPixel($xx, $yy)
                $dst = $dstRow + $xx * 4
                $xor[$dst]     = $c.B
                $xor[$dst + 1] = $c.G
                $xor[$dst + 2] = $c.R
                $xor[$dst + 3] = $c.A
            }
        }
        $maskRowBytes = [int][Math]::Ceiling($s / 8.0)
        $maskRowPadded = [int][Math]::Ceiling($maskRowBytes / 4.0) * 4
        $andMask = New-Object byte[] ($maskRowPadded * $s)

        $ms = New-Object System.IO.MemoryStream
        $bw = New-Object System.IO.BinaryWriter($ms)
        $bw.Write([uint32]40)          # biSize
        $bw.Write([int32]$s)           # biWidth
        $bw.Write([int32]($s * 2))     # biHeight (XOR + AND)
        $bw.Write([uint16]1)           # biPlanes
        $bw.Write([uint16]32)          # biBitCount
        $bw.Write([uint32]0)           # biCompression
        $bw.Write([uint32]0)           # biSizeImage
        $bw.Write([int32]0)            # biXPelsPerMeter
        $bw.Write([int32]0)            # biYPelsPerMeter
        $bw.Write([uint32]0)           # biClrUsed
        $bw.Write([uint32]0)           # biClrImportant
        $bw.Write($xor)
        $bw.Write($andMask)
        $bw.Flush()
        $null = $entries.Add(@{ Size = $s; Png = $false; Bytes = $ms.ToArray() })
        $bw.Dispose()
        $ms.Dispose()
    }
    $bmp.Dispose()
    Write-Host ("  + {0}x{1}" -f $s, $s)
}
$img.Dispose()

# Monta o arquivo .ico
$count = $entries.Count
$offset = 6 + 16 * $count
$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)
$bw.Write([uint16]0)                  # reservado
$bw.Write([uint16]1)                  # tipo: ícone
$bw.Write([uint16]$count)             # quantidade
foreach ($e in $entries) {
    $dim = 0
    if ($e.Size -ne 256) { $dim = $e.Size }
    $bw.Write([byte]$dim)              # largura (0 = 256)
    $bw.Write([byte]$dim)              # altura
    $bw.Write([byte]0)                 # cores
    $bw.Write([byte]0)                 # reservado
    $bw.Write([uint16]1)               # planos
    $bw.Write([uint16]32)              # bits
    $bw.Write([uint32]$e.Bytes.Length) # bytes da imagem
    $bw.Write([uint32]$offset)         # offset
    $offset += $e.Bytes.Length
}
foreach ($e in $entries) { $bw.Write($e.Bytes) }
$bw.Flush()
[System.IO.File]::WriteAllBytes($Out, $ms.ToArray())
$bw.Dispose()
$ms.Dispose()

$sizeKb = [Math]::Round((Get-Item $Out).Length / 1KB, 1)
Write-Host ("OK: {0} ({1} KB, {2} tamanhos embutidos)" -f $Out, $sizeKb, $count)
