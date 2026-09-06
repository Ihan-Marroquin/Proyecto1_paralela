# Aurora Paralela

Screensaver de una aurora cósmica hecho en C++17 y OpenMP para el Proyecto 1 de
Computación Paralela y Distribuida. Varias fuentes de energía se mueven por el canvas,
rebotan en sus límites y producen ondas de color. El argumento `N` indica cuántas
fuentes se calculan en cada píxel.

El proyecto contiene el mismo algoritmo en tres modos:

- `sequential`: recorre todo el framebuffer con un solo hilo.
- `parallel`: reparte las filas entre hilos con `omp parallel for`.
- `optimized`: reutiliza buffers, separa datos por propiedad, precalcula distancias y
  mantiene una sola región paralela con `nowait`, barrera explícita y `collapse(2)`.

Los tres modos parten del mismo estado y generan la misma imagen; esto permite comparar
tiempos sin cambiar el resultado visual.

## Requisitos

- Windows 10 u 11.
- MinGW-w64 con soporte para OpenMP. Se recomienda el entorno UCRT64 de MSYS2.
- PowerShell 5 o posterior para usar los scripts incluidos.

No se necesita SDL ni otra biblioteca gráfica. La ventana usa GDI de Windows y el
framebuffer se calcula completamente en CPU.

## Compilación

Desde la raíz del repositorio:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1
```

Para compilar con símbolos de depuración:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Configuration debug
```

El ejecutable se crea en `build/aurora_saver.exe`. La carpeta `build` está ignorada por
Git porque la entrega solicita únicamente código fuente.

También se incluye un `CMakeLists.txt` para entornos que ya tengan CMake y OpenMP
configurados.

## Uso

`N` es obligatorio y debe estar entre 1 y 512. El siguiente ejemplo abre la versión
optimizada con 24 fuentes y 8 hilos:

```powershell
./build/aurora_saver.exe 24 --mode optimized --threads 8
```

Opciones principales:

| Opción | Descripción |
|---|---|
| `--mode MODO` | `sequential`, `parallel` u `optimized` |
| `--width N` | Ancho del canvas, mínimo 640 |
| `--height N` | Alto del canvas, mínimo 480 |
| `--threads N` | Cantidad de hilos de OpenMP |
| `--fps N` | Límite de cuadros por segundo; 0 lo desactiva |
| `--seed N` | Semilla para repetir la misma escena |
| `--headless` | Ejecuta el cálculo sin abrir ventana |
| `--seconds N` | Duración del modo sin ventana |
| `--output RUTA` | Guarda el último cuadro en formato PPM |
| `--benchmark` | Ejecuta las mediciones de los tres modos |
| `--trials N` | Mediciones por modo, mínimo 10 |
| `--csv RUTA` | Ruta del archivo de resultados |
| `--self-test` | Comprueba la consistencia de la implementación |
| `--help` | Muestra la ayuda completa |

### Controles de la ventana

- `1`, `2`, `3`: cambiar a secuencial, paralelo u optimizado.
- `Espacio`: pausar o continuar.
- `F`: entrar o salir de pantalla completa.
- `S`: guardar el cuadro actual. Si no se indicó `--output`, usa
  `results/captura.ppm`.
- `Esc`: cerrar.

El título de la ventana muestra el modo activo, FPS, tiempo de renderizado y valor de
`N`.

## Mediciones

El script compila en modo release y ejecuta diez pruebas por defecto:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_benchmark.ps1 `
  -Sources 16 -Threads 8 -Trials 10
```

Las mediciones se guardan en `results/benchmark.csv`. Primero aparecen los tiempos de
cada repetición y luego el resumen. Se usan las fórmulas:

```text
speedup = tiempo_secuencial / tiempo_paralelo
eficiencia = speedup / cantidad_de_hilos * 100
```

Antes de medir se ejecuta un calentamiento. En cada repetición se alterna el orden de
los modos para reducir el sesgo de caché, y se comparan sus checksums para verificar que
la optimización no haya cambiado la salida.

## Organización del código

```text
include/                  declaraciones de cada módulo
src/AppConfig.cpp         argumentos y programación defensiva
src/AuroraSimulation.cpp  movimiento, rebotes y fuerzas trigonométricas
src/AuroraRenderer.cpp    versiones secuencial y OpenMP
src/Benchmark.cpp         mediciones, speedup, eficiencia y CSV
src/ScreensaverWindow.cpp ventana y controles interactivos
scripts/                  compilación y ejecución de pruebas
```

## Verificación

Para compilar y ejecutar las pruebas internas junto con casos de argumentos inválidos:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/test.ps1
```

La prueba principal compara cada píxel y el checksum de las tres versiones, comprueba
que las fuentes se mantengan dentro del canvas y verifica que una semilla produzca el
mismo resultado en ejecuciones distintas.
