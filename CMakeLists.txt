cmake_minimum_required(VERSION 3.16)
project(SystemWidget LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую генерацию мета-объектов Qt (MOC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Находим Qt6 (Core, Gui, Widgets, Svg)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

# Добавляем пути к заголовочным файлам SDK (раскомментировать после скачивания)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/NVAPI)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ADL/include)

add_executable(SystemWidget
    main.cpp
    ResourceWidget.cpp
    ResourceWidget.h
    GpuMonitor.cpp
    GpuMonitor.h
)

target_link_libraries(SystemWidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

# Для получения системных данных на Windows
if(WIN32)
    target_link_libraries(SystemWidget PRIVATE user32)

    # Линковка NVIDIA NVAPI (раскомментировать после скачивания)
    target_link_libraries(SystemWidget PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/NVAPI/amd64/nvapi64.lib)
endif()
