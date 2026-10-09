# LANDELTA 0.11 — primera beta con la nueva marca

Vortex pasa a llamarse LANDELTA. Esta beta conserva tus ajustes e historial y añade una referencia de delta propia y una detección más completa del inicio de sesión.

- Nueva marca e icono.
- GAS y FRENO con barras sin porcentajes.
- Delta nativo disponible aunque falte la estimación de vuelta de ACC.
- **Δ MEJOR** compara contra tu mejor vuelta limpia completa observada, por punto del circuito. **Δ ACC** sirve de alternativa mientras se aprende.
- Los cambios de sesión, circuito y auto limpian la referencia. Las vueltas inválidas, de boxes y parciales no la crean.
- Los overlays reaparecen al entrar, cambiar o reiniciar sesión, incluso en reinicios que mantienen el mismo ID y antes de recibir la lista de pilotos.
- Opción **Mostrar overlays al entrar en sesión** en Inicio, activada por defecto.
- Diagnóstico ampliado para delta y visibilidad.

## Descargas

- **LANDELTA-Windows-Beta.zip**: paquete de Windows con ejecutable, instrucciones, fuentes y avisos de dependencias.
- **LANDELTA.exe**: ejecutable de Windows x64. Para la distribución completa, usa el ZIP.

## Actualizar

Sal de Vortex desde la bandeja, extrae la nueva beta y abre LANDELTA. Si usabas inicio con Windows, guarda el EXE en una carpeta fija y pulsa **Inicio → Activar** para actualizar su acceso.

Usa ACC en ventana sin bordes. Para probar Δ MEJOR, completa una vuelta limpia observada desde su inicio con la app abierta y cambia tu ritmo en la siguiente vuelta.

## Validación y límites

Compilación x64 y pruebas de protocolo, lógica, sesiones, delta, ajustes y transporte/APIs simulados. Esta beta no se ha ejecutado en ACC real bajo Windows desde el entorno de desarrollo. Los relativos y la predicción de alcance son estimaciones. Ver `VALIDACION.txt` para el detalle.

Las descargas de esta release se generan en Windows mediante GitHub Actions a partir del commit etiquetado y se publican solo tras superar la compilación y las pruebas portables. `SHA256SUMS.txt` permite comprobar el EXE y el ZIP.

Los reportes de pruebas reales en distintas carreras y circuitos son bienvenidos en Issues.
