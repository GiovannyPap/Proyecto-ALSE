# Gestor de Domótica para Hogares

Aplicación de escritorio en **C++17 / Qt** con persistencia en **SQLite** y documentación **Doxygen**
(proyecto 3.5 de Algoritmos en Sistemas Electrónicos).

## Funcionalidades
- **Autenticación**: crear cuenta y entrar (contraseñas con sal + SHA-256 iterado).
- **Dispositivos**: registrar, editar y eliminar luces, persianas, sensores y actuadores (habitación, potencia, uso diario).
- **Control**: encender/apagar (abrir/cerrar persianas), nivel 0-100 % en luces y persianas, doble clic para alternar.
- **Validación de estados**: Encendido, Apagado, **Fallo** (un dispositivo en fallo no se activa hasta repararlo).
- **Rutinas**: hora, días de la semana, acción (encender/apagar) y varios dispositivos; se ejecutan solas cada día.
- **Historial de eventos** con fecha y hora, filtros por dispositivo/tipo y exportación a CSV.
- **Consumo energético**: estimación (potencia × horas/día, mes de 30 días, costo con tarifa configurable) y
  consumo **medido** de los últimos 30 días a partir de las sesiones reales de encendido.
- **Panel de control** con gráfica de barras (kWh/mes por dispositivo), gráfica circular de estados y tabla.

## Estructura
```
src/core/   Dominio en C++ estándar (sin Qt): Device (Light, Blind, Sensor, Actuator), servicios,
            rutinas, energía, autenticación, SHA-256, CSV
src/db/     SqliteRepository (QtSql) y Sql.h (todas las sentencias; esquema normalizado 3FN)
src/gui/    Ventana principal, diálogos y gráficas (QPainter, sin QtCharts => ejecutable liviano)
tests/      test_core.cpp (92 comprobaciones), test_sql.py (39 comprobaciones sobre SQLite real)
```
El núcleo depende solo de la interfaz `IRepository`; la GUI y la base de datos son capas intercambiables.

## Compilar (Qt 5.15+ o Qt 6, con módulos Widgets y Sql)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/GestorDomotica            # opcional: --db ruta/archivo.db
```
Ubuntu/Debian: `sudo apt install qt6-base-dev cmake g++` (o `qtbase5-dev`). Windows: Qt Online Installer + Qt Creator
(abrir `CMakeLists.txt`). Raspberry Pi OS: mismos paquetes `apt`.

## Pruebas
```bash
ctest --test-dir build --output-on-failure
# o sin CMake/Qt:
g++ -std=c++17 -Wall -Wextra -Werror src/core/*.cpp tests/test_core.cpp -Isrc -o test_core && ./test_core
python3 tests/test_sql.py
```

## Documentación
`doxygen Doxyfile` genera `docs/html/index.html`.

## Datos
La base `domotica.db` se crea en la carpeta de datos de la aplicación del usuario. Primera ejecución: la app pide crear la cuenta inicial.
