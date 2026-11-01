# LANDELTA 0.13.4 para Windows

Para usuarios: descarga **LANDELTA-Setup-0.13.4.exe**, ejecútalo y abre **LANDELTA** desde el escritorio o Inicio. No hace falta extraer carpetas ni instalar Python, .NET o herramientas de desarrollo. AC Studio se abre desde la misma aplicación; incluye el visor local y captura datos reales. No hay un acceso ni botón de demo.

Si aparece que LANDELTA sigue abierta, pulsa **Salir de LANDELTA** en su icono junto al reloj y continúa el instalador. La X solamente oculta la ventana. No se fuerza el cierre para evitar interrumpir una grabación. Las versiones portátiles viejas no se borran: usa el nuevo acceso LANDELTA, no el ejecutable que quedó en Descargas.

Se instala por usuario en `%LOCALAPPDATA%\Programs\LANDELTA`, sin pedir administrador. Actualizar utiliza la misma ubicación. Si ya había inicio con Windows, su acceso se redirige a la copia instalada. Una instalación nueva no activa inicio automático sin pedirlo: puedes hacerlo desde Inicio en la app.

Los ajustes, historial y CSV permanecen en `%LOCALAPPDATA%\VortexACC`; instalar o desinstalar no los elimina. Windows permite desinstalar LANDELTA desde Aplicaciones instaladas.

El instalador todavía no tiene firma digital. Windows puede mostrar una advertencia de editor desconocido; no desactives la protección del sistema. Distribuye solo archivos procedentes del repositorio oficial y comprobados.

Validación: pruebas portables y prueba automatizada de instalación/reinstalación, accesos, versión, migración del inicio, bloqueo si la app está abierta y desinstalación en Windows. Esto no reemplaza probar una sesión real en AC/ACC ni la interacción manual del asistente en diferentes equipos.
