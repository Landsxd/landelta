# LANDELTA 0.14.2 · iRacing y monitores

## Captura y delta

- iRacing dispone de un grabador dedicado, guiado por `Local\IRSDKDataValidEvent`. Abrir sesiones, arrastrar o redimensionar la ventana no bloquea la captura de CSV.
- Solo se guardan paquetes nuevos y coherentes: se verifica de nuevo el contador y la estructura después de copiar la memoria compartida. La conexión es de solo lectura.
- La app muestra Hz medidos, Hz anunciados por el SDK y ticks no observados. Un SDK de 60 Hz no garantiza que Windows permita capturar todos. `sdk_hz` y `missed_ticks` quedan en las muestras exportadas; las sesiones antiguas siguen abriendo.
- El delta compara con la mejor vuelta **de la sesión**, usando `LapDeltaToSessionBestLap` y su bandera `_OK`. No cambia a una referencia calculada localmente cuando falta la del SDK. Sin mejor vuelta, en prerace, boxes, replay o tras 250 ms sin datos nuevos, no presenta un número.
- Los relativos y gaps de iRacing solo usan pasos observados por puntos de pista, nunca distancia multiplicada por una vuelta supuesta. Son estimaciones (`~`), no intervalos oficiales. Al principio pueden quedar vacíos.
- Los coches que el SDK marca fuera del mundo dejan de contribuir a los relativos.
- El directo es una vista reducida. En Mis sesiones se abre el CSV completo; los ZIP conservan las muestras originales. ACC mantiene su captura de hasta 10 Hz y AC su grabador existente.

## Tu pantalla

- Enumera monitores reales por separado. Selecciona Monitor 1, 2, etc.; cada miniatura conserva su resolución y proporciones.
- Permite posiciones negativas, pantallas verticales y resoluciones distintas. Pulsa el nombre de un panel situado en otra pantalla para traerlo a la seleccionada.
- Cambiar de monitor conserva el borrador; Guardar distribución aplica todas las pantallas y Cancelar descarta los cambios. Restablecer afecta a la pantalla seleccionada.
- La ventana declara compatibilidad DPI por monitor. Un panel mayor que la pantalla genera un aviso para reducir su tamaño o sus filas.

## Identidad visual

- Verde lima racing, fondos grafito, contraste más alto y acentos contenidos.
- Tipografía de interfaz Segoe UI, títulos Bahnschrift cuando está disponible y cifras tabulares Consolas. Fallback a Segoe UI sin instalaciones de fuentes externas.
- Navegación con iconos, marca delta vectorial, tarjetas y controles con jerarquía más clara.

## Comprobación

Pruebas portables para delta válido/ausente, boxes, replay, muestras detenidas, ticks repetidos/perdidos y monitores con origen negativo. Prueba nativa con memoria SDK **sintética y aislada** comprueba que el grabador produce el CSV sin un bucle de interfaz. Pruebas de Guardar/Cancelar y traslado al segundo monitor, capturas nativas e instalador de Windows.

Estas pruebas no sustituyen conducir en iRacing ni usar dos monitores físicos con distintos DPI. La integración sigue en validación de pista; no se promete precisión perfecta ni cero paquetes perdidos. No se inventa validez por vuelta ni se convierten microsectores en sectores oficiales.

Referencias técnicas: [SDK de iRacing](https://github.com/vipoo/irsdk/blob/master/irsdk_defines.h), [EnumDisplayMonitors](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaymonitors), [DPI por monitor](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged).
