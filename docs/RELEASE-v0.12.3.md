# LANDELTA 0.12.3 — recuperación de conexión offline

- El estado OFF de la memoria compartida ya no detiene ni cierra UDP. La app sigue escuchando y puede registrar pilotos durante una transición.
- Vuelve a abrir periódicamente la memoria marcada como menú, pausa o lectura inválida, para recuperar un mapeo antiguo. Una pausa legítima conserva sus tiempos.
- Una señal UDP posterior al estado OFF puede mostrar la sesión y los overlays; los paquetes anteriores a esa transición no la confirman.
- Corrige el paquete de baja UDP: ahora incluye el identificador de la conexión registrada.
- Reconectar también vuelve a abrir una lectura incierta, sin borrar los tiempos de una lectura activa válida.
- Añade al diagnóstico el estado y paquete de memoria observados, sus edades, las reaperturas y los intentos de registro UDP.

## Actualizar desde 0.12.2

1. Guarda o termina la sesión en curso y cierra ACC por completo.
2. Junto al reloj de Windows, usa **Salir de LANDELTA**; la X solo la oculta.
3. Descarga **LANDELTA-Windows-Beta.zip**, extrae y reemplaza la versión anterior.
4. Abre LANDELTA y después ACC. Este reinicio inicial permite empezar sin conexiones que haya dejado la versión anterior.
5. Si usas inicio con Windows, en **Inicio** desactiva y vuelve a activar la opción para que apunte al nuevo ejecutable.

No se borran ajustes ni historial. Para comprobar el arreglo, pasa de clasificación a carrera offline con la app abierta, sin reparar entre sesiones.

Validación automatizada: mapeos antiguos detenidos en OFF y PAUSE, fallo al reabrir, reconexión manual, registro UDP independiente, 30 transiciones consecutivas con una sola suscripción y contenido de la baja UDP. Compilación Windows x64 y pruebas completas. La validación en ACC real sigue pendiente.

Referencia de implementación consultada para la baja UDP: [cliente accbroadcast](https://github.com/geniusdex/broawp/blob/d7f5ba008f03/accbroadcast/client.go), que envía el identificador de conexión de 32 bits después de la orden de baja.
