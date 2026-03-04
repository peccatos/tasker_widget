# SystemWidget (Windows)

## Что уже есть
- Сборка через `CMake` без запуска Qt Creator.
- Упаковка для распространения:
  - `setup.exe` (NSIS), если установлен `makensis`.
  - `zip` portable-пакет (всегда).
- Автокопирование Qt/MSVC runtime через `windeployqt`, поэтому конечным пользователям не нужны Qt/Visual Studio.

## Поддержка GPU
- `NVIDIA`: используется NVAPI (если в проекте есть `NVAPI/nvapi.h` и `NVAPI/amd64/nvapi64.lib`).
- `AMD`: работает без ADL SDK через Windows API (`DXGI + PDH`).
- Также есть универсальный fallback backend через Windows API.

Примечание: температура GPU в универсальном backend не доступна через стандартный WinAPI и отображается как `0°C`.

## Сборка релиза + упаковка
Запуск из папки `native-cpp`:

```bat
build-release.bat
```

Артефакты:
- `build\Desktop-Release\packages\*.exe` (если NSIS доступен)
- `build\Desktop-Release\packages\*.zip`

## Требования для сборки
- Windows 10/11 x64
- CMake
- Visual Studio 2022 Build Tools (C++)
- Qt 6 (msvc)
- Опционально NSIS (`makensis`) для генерации `setup.exe`
