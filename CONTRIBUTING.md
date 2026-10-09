# Contribuir a LANDELTA

Antes de proponer un cambio, abre un Issue explicando el problema concreto y el resultado esperado. Los reportes de pruebas reales en ACC son especialmente útiles.

## Reportes útiles

Incluye versión de LANDELTA y Windows, circuito, modalidad, online/offline, pasos para reproducir y una captura si ayuda. El diagnóstico muestra el estado de las dos conexiones; elimina rutas y nombres personales antes de compartirlo.

No adjuntes credenciales, `broadcasting.json` ni carreras personales completas. Usa datos sintéticos mínimos para reproducir problemas de protocolo.

## Desarrollo

- `src/main.cpp`: aplicación Windows, dibujo, memoria compartida y UDP.
- `src/telemetry.hpp` y `runtime.hpp`: protocolo y ciclo de vida de sesiones.
- `src/driving.hpp`: pedales, delta y comparación de sectores.
- `src/background.hpp` y `startup.hpp`: visibilidad, bandeja e inicio con Windows.
- `src/analytics.hpp` y `history.hpp`: ritmo, alcance e historial.
- `tests/`: lógica e integración con transporte y APIs simulados.
- `tools/render-design.py`: vistas del diseño a partir de las funciones de dibujo.

Compila con `build-windows.bat` desde una terminal MSYS2 UCRT64, como indica el README. Para pruebas portables en Linux necesitas `g++` y Python 3:

```bash
python3 tools/run-tests.py
```

La vista previa necesita además `cairosvg` y `pillow`; el programa de Windows no los necesita.

## Pull requests

Describe el caso que falla, el nuevo comportamiento y cómo lo comprobaste. Mantén los cambios centrados en ese caso. Evita modificar los headers vendorizados para cambios de la app.

Preserva la compatibilidad de ajustes e historial, la transparencia y los paneles sin activación de ventana. No mezcles datos de sesiones, coches o replay. Las estimaciones deben seguir diferenciándose de los tiempos oficiales.

Una prueba simulada o una captura de demo no acredita funcionamiento en ACC real: indica claramente qué entorno usaste.
