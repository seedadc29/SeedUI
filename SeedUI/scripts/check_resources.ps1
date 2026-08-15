# Diagnostico: lista os recursos RCDATA embutidos em um executavel.
param(
    [string]$Exe = "D:\Vamos testar as otimizacoes\SeedEngine-main\SeedUI\Build\Release\SeedUI.exe"
)

$src = @"
using System;
using System.Runtime.InteropServices;
public static class ResCheck {
    public delegate bool EnumResNameProc(IntPtr hModule, IntPtr lpszType, IntPtr lpszName, IntPtr lParam);
    [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, uint dwFlags);
    [DllImport("kernel32.dll")]
    public static extern bool EnumResourceNames(IntPtr hModule, IntPtr lpszType, EnumResNameProc lpEnumFunc, IntPtr lParam);
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr hModule);
}
"@
Add-Type -TypeDefinition $src

$h = [ResCheck]::LoadLibraryEx($Exe, [IntPtr]::Zero, 0x2)  # LOAD_LIBRARY_AS_DATAFILE
if ($h -eq [IntPtr]::Zero) { Write-Host "ERRO: nao carregou o exe"; exit 1 }

$names = New-Object System.Collections.ArrayList
$cb = [ResCheck+EnumResNameProc]{
    param($hMod, $type, $name, $lParam)
    if (($name.ToInt64() -shr 16) -eq 0) {
        $null = $names.Add(("#{0}" -f $name.ToInt64()))
    } else {
        $null = $names.Add([Runtime.InteropServices.Marshal]::PtrToStringUni($name))
    }
    return $true
}
$rtRcdata = [IntPtr]10
[ResCheck]::EnumResourceNames($h, $rtRcdata, $cb, [IntPtr]::Zero) | Out-Null
[ResCheck]::FreeLibrary($h) | Out-Null

Write-Host ("RCDATA encontrados: {0}" -f $names.Count)
$names | Select-Object -First 20 | ForEach-Object { Write-Host "  - $_" }
