cmake_minimum_required(VERSION 3.16)
project(Racing3D)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Konfiguracja projektu Racing3D...")

# --- Pliki Ÿród³owe ---
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")

if(NOT SRC_FILES)
    message(FATAL_ERROR "Nie znaleziono ¿adnych plików .cpp w folderze ${CMAKE_SOURCE_DIR}/src")
endif()

add_executable(${PROJECT_NAME} ${SRC_FILES})


# --- DODANY KOD: Wymuszenie kopiowania shaderów po zbudowaniu ---
# To naprawia b³¹d "FILE_NOT_SUCCESFULLY_READ", upewniaj¹c siê,
# ¿e folder 'shaders' jest obok pliku .exe.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)


# --- Biblioteki z vcpkg ---
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

# --- Kopiowanie shaderów (jeœli istniej¹) ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    message(STATUS "Kopiowanie shaderów do katalogu build...")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_BINARY_DIR}")
else()
    message(WARNING "Folder 'shaders' nie istnieje — pomijam kopiowanie.")
endif()
