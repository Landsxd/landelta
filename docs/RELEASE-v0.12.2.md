# LANDELTA 0.12.2 — conexión al cambiar de sesión

- Corrige un bloqueo reproducido al pasar de clasificación a carrera: los tiempos de tu auto siguen leyendo ACC aunque la conexión de pilotos todavía anuncie la sesión anterior.
- Conserva una suscripción UDP activa durante el cambio de sesión y solicita de nuevo circuito y parrilla. Descarta los autos de la sesión anterior mientras llega la nueva.
- Reconectar reinicia la conexión de pilotos sin borrar los tiempos válidos de tu auto.
- La recuperación de memoria congelada sigue funcionando cuando las dos fuentes de ACC cambian de sesión en momentos distintos.
- Reinicia vueltas, sectores y referencia de delta al detectar una sesión nueva; conserva el historial guardado.
- Incluye las mejoras de la ventana, delta e historial de las versiones anteriores.

Descarga **LANDELTA-Windows-Beta.zip**. Antes de reemplazar el ejecutable, usa el icono junto al reloj → **Salir de LANDELTA**; la X lo deja en segundo plano. Extrae el ZIP y abre la nueva versión. Se conservan ajustes e historial.

Validación: regresión que falla con el código anterior y pasa con la corrección, cambios de sesión en ambos órdenes de llegada, paquetes antiguos, reconexiones repetidas y recuperación de memoria congelada; pruebas automatizadas y compilación Windows x64. La comprobación en una carrera real de ACC sigue pendiente.
