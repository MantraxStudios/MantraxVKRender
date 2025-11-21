cmake_minimum_required(VERSION 3.15)
project(MantraxApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB ADDON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../add-ons/include/imgui/*.cpp"
)


# Agregar el ejecutable
add_executable(MantraxApp WIN32 main.cpp
                                ${ADDON_SOURCES})

# Incluir directorios de encabezados
target_include_directories(MantraxApp PRIVATE 
    ${CMAKE_SOURCE_DIR}              # Directorio raíz del proyecto
    ../render/include                # Directorio de los encabezados de Vulkan
    ../add-ons/include                # Directorio de los encabezados de Vulkan
)

if(MSVC)
    set_target_properties(MantraxApp PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()


# Incluir directorio de bibliotecas
link_directories(../render/libs)     # Directorio de las bibliotecas de Vulkan

# Buscar todos los archivos .lib en el directorio ../render/libs
file(GLOB LIBS "../render/libs/*.lib")

# Enlazar con las bibliotecas de DirectX 12
target_link_libraries(MantraxApp 
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

# Enlazar con todas las bibliotecas .lib encontradas en ../render/libs
target_link_libraries(MantraxApp 
    ${LIBS}  # Las bibliotecas encontradas automáticamente
)

# Enlazar con la biblioteca de Vulkan
target_link_libraries(MantraxApp
    "C:/Users/tupap/Desktop/MantraxFastRender/render/libs/vulkan-1.lib"
)
