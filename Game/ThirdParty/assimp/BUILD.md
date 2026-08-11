# Assimp build contract

This project vendors Assimp 5.4.3 as a shared x64 library built with MSVC.

The bundled binary was compiled with `ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS`.
This definition changes the public `aiBone` memory layout, so every C++ target
that includes Assimp headers must use the same definition. `Game.vcxproj`
therefore declares it for Debug, Development, and Release.

Changing this option requires rebuilding the DLL and import library together
and updating the consuming preprocessor definition at the same time. Do not
mix headers and binaries produced with different values for ABI-affecting
Assimp options such as this one or `ASSIMP_DOUBLE_PRECISION`.
