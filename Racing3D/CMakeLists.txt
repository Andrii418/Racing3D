cmake_minimum_required(VERSION 3.16)
project(Racing3D)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Konfiguracja projektu Racing3D...")

# -----------------------------------------------------------------
# 1. INTEGRACJA IMGUI: DEKLARACJA åCIEØEK I PLIK”W èR”D£OWYCH
# -----------------------------------------------------------------
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- Pliki ürÛd≥owe GRY ---
file(GLOB_RECURSE SRC_FILES_GAME "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME}
    ${SRC_FILES_GAME}
    ${IMGUI_SOURCES}
)

# -----------------------------------------------------------------
# 2. INTEGRACJA IMGUI: åCIEØKI NAG£”WKOWE I DEFINICJE KOMPILATORA
# -----------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

# Kopiowanie shaderÛw po zbudowaniu
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

# Biblioteki z vcpkg/systemu (zewnÍtrzne)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad::glad
    glm::glm
    OpenGL::GL
)

# Dodatkowe ó opcjonalnie: kopiowanie shaderÛw (jeúli chcesz teø skopiowaÊ do katalogu build)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    message(STATUS "Kopiowanie shaderÛw do katalogu build...")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
else()
    message(WARNING "Folder 'shaders' nie istnieje ó pomijam kopiowanie.")
endif()
