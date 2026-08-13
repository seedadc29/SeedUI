Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Sim2 {
    [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] pInputs, int cbSize);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("kernel32.dll")] public static extern IntPtr GetModuleHandle(string name);

    [StructLayout(LayoutKind.Sequential)]
    public struct INPUT { public uint type; public InputUnion U; }
    [StructLayout(LayoutKind.Explicit)]
    public struct InputUnion {
        [FieldOffset(0)] public KEYBDINPUT ki;
        [FieldOffset(0)] public MOUSEINPUT mi;
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Sequential)]
    public struct MOUSEINPUT { public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }

    public static void Key(ushort vk, bool down) {
        INPUT i = new INPUT();
        i.type = 1; // KEYBOARD
        i.U.ki.wVk = vk;
        i.U.ki.dwFlags = down ? 0u : 2u; // KEYEVENTF_KEYUP
        SendInput(1, new INPUT[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void MouseMove(int x, int y) {
        INPUT i = new INPUT();
        i.type = 0; // MOUSE
        i.U.mi.dx = x; i.U.mi.dy = y;
        i.U.mi.dwFlags = 0x0001 | 0x8000; // MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE
        SendInput(1, new INPUT[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
    public static void MouseButton(bool down) {
        INPUT i = new INPUT();
        i.type = 0;
        i.U.mi.dwFlags = down ? 0x0002u : 0x0004u; // LEFTDOWN / LEFTUP
        SendInput(1, new INPUT[] { i }, Marshal.SizeOf(typeof(INPUT)));
    }
}
"@
$p = Get-Process -Name SeedNotas -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $p) { Write-Host "NAO RODANDO"; exit 1 }
[Sim2]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
Start-Sleep -Milliseconds 500

# F5 real (SendInput)
[Sim2]::Key(0x74, $true)   # VK_F5 down
Start-Sleep -Milliseconds 100
[Sim2]::Key(0x74, $false)  # VK_F5 up
Start-Sleep -Milliseconds 900

# Arrasto: centro -> baixo-direita (mouse real)
[Sim2]::MouseMove(650, 400)
Start-Sleep -Milliseconds 300
[Sim2]::MouseButton($true)
Start-Sleep -Milliseconds 150
[Sim2]::MouseMove(900, 600)
Start-Sleep -Milliseconds 400
[Sim2]::MouseButton($false)
Start-Sleep -Milliseconds 700
Write-Host "simulacao concluida"
