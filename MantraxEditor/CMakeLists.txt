cmake_minimum_required(VERSION 3.15)
project(MantraxEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================
# REMOVER FLAGS PROBLEMÁTICOS DE CLANG
# =============================
# Estos flags están causando problemas con el linking
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(REPLACE "-nostartfiles" "" CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
    string(REPLACE "-nostdlib" "" CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
    string(REPLACE "-nostartfiles" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    string(REPLACE "-nostdlib" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" CACHE STRING "" FORCE)
endif()

# =============================
# SOLO FUENTES DEL EDITOR
# =============================
file(GLOB EDITOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/includes/imgui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp"
)

# =============================
# CREAR EJECUTABLE WIN32
# =============================
add_executable(MantraxEditor WIN32 ${EDITOR_SOURCES})

# =============================
# INCLUDES
# =============================
target_include_directories(MantraxEditor PRIVATE
    ${CMAKE_SOURCE_DIR}
    ../MantraxRender/include
    ../MantraxAddons/include
    ../MantraxECS/include
    ../MantraxECS/include/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
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
# LINK A LIBRERÍAS DEL SISTEMA
# =============================
target_link_libraries(MantraxEditor 
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
# LINK AL DLL DE MANTRAXAPP
# =============================
target_link_libraries(MantraxEditor
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxWrapper/build/Mantrax.lib"
)

# =============================
# LIBRERÍAS ESPECÍFICAS
# =============================
target_link_libraries(MantraxEditor
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxRender/libs/vulkan-1.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxAddons/libs/assimp-vc143-mt.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxECS/libs/lua-5.4.4.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxECS/libs/lua54.lib"
)

# =============================
# COPIAR LA DLL AL EJECUTABLE
# =============================
add_custom_command(TARGET MantraxEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../MantraxWrapper/build/Mantrax.dll"
        "$<TARGET_FILE_DIR:MantraxEditor>"
    COMMENT "Copiando MantraxApp.dll al directorio del ejecutable"
)