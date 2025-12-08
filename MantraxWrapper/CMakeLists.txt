cmake_minimum_required(VERSION 3.15)
project(MantraxApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================
# ARCHIVOS FUENTE
# =============================
file(GLOB ADDON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxECS/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxRender/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxAddons/src/stb_image_impl.cpp"
)

# =============================
# CREAR DLL
# =============================
add_library(MantraxApp SHARED ${ADDON_SOURCES})

# ✅ CRÍTICO: Definir AMBAS macros para compatibilidad
target_compile_definitions(MantraxApp PRIVATE 
    MANTRAXAPP_EXPORTS    # ← Tu macro original
    MANTRAX_EXPORTS       # ← Nueva macro para EngineAPI
)

# =============================
# INCLUDES
# =============================
target_include_directories(MantraxApp PRIVATE 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # ← Para MantraxExport.h
    ../MantraxRender/include
    ../MantraxAddons/include
    ../MantraxECS/include
    ../MantraxECS/include/lua
)

# =============================
# INCLUDES PÚBLICOS (para quien use la DLL)
# =============================
target_include_directories(MantraxApp PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../MantraxRender/include
    ../MantraxAddons/include
    ../MantraxECS/include
    ../MantraxECS/include/lua
)

# =============================
# LIBRERÍAS
# =============================
link_directories(
    ../MantraxRender/libs
    ../MantraxAddons/libs
    ../MantraxECS/libs
)

# Auto-descubrir .lib
file(GLOB LIBS
    "../MantraxRender/libs/*.lib"
    "../MantraxAddons/libs/*.lib"
    "../MantraxECS/libs/*.lib"
)

# =============================
# LINK A LIBRERÍAS
# =============================
target_link_libraries(MantraxApp 
    ${LIBS}
    d3d12.lib
    dxgi.lib
    d3dcompiler.lib
    kernel32.lib
    user32.lib
    gdi32.lib
    winspool.lib
    shell32.lib
    ole32.lib
    oleaut32.lib
    uuid.lib
    comdlg32.lib
    advapi32.lib
)

# =============================
# LIBRERÍAS ESPECÍFICAS
# =============================
target_link_libraries(MantraxApp
    "${CMAKE_SOURCE_DIR}/../MantraxRender/libs/vulkan-1.lib"
    "${CMAKE_SOURCE_DIR}/../MantraxAddons/libs/assimp-vc143-mt.lib"
    "${CMAKE_SOURCE_DIR}/../MantraxECS/libs/lua-5.4.4.lib"
    "${CMAKE_SOURCE_DIR}/../MantraxECS/libs/lua54.lib"
)

# =============================
# GENERAR ARCHIVOS DE EXPORTACIÓN
# =============================
set_target_properties(MantraxApp PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF  # Usar nuestras macros explícitas
    OUTPUT_NAME "Mantrax"           # ← Genera Mantrax.dll (opcional)
)

# =============================
# POST-BUILD: Copiar DLL a carpeta de C#
# =============================
# ✅ OPCIONAL: Copiar automáticamente el DLL después de compilar
# Cambia la ruta a donde está tu ejecutable de C#
# add_custom_command(TARGET MantraxApp POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:MantraxApp>
#         "${CMAKE_SOURCE_DIR}/path/to/csharp/bin/Debug/"
#     COMMENT "Copiando Mantrax.dll a carpeta de C#..."
# )