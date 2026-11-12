# AC y ACC en la ventana principal

Dos botones grandes y permanentes seleccionan **AC · Studio** o **ACC · Overlay**. El modo seleccionado se recuerda; cambiar de pantalla no corta la captura. AC usa la misma interfaz oscura/verde y la misma ventana redimensionable: ya no abre un diálogo separado.

AC Studio muestra circuito, coche, vuelta actual, último tiempo del juego, barras de pedales, velocidad y estado de grabación. Las lecturas se refrescan aproximadamente cada 100 ms para la interfaz, sin cambiar la frecuencia del CSV. Si se pausa o pierde la señal, las barras y velocidad dejan de mostrarse como activas; el último tiempo permanece identificado como última lectura.

**Captura automática** está activada por defecto. Abre LANDELTA antes de entrar en práctica, clasificación o carrera de Assetto Corsa (2014); el lector espera al proceso y solo escribe muestras de una sesión activa. No se registran menús, pausas ni replays. Si abres LANDELTA a mitad de vuelta, esa primera vuelta será parcial. Cerrar AC guarda el archivo y deja el lector esperando a la siguiente sesión. Pausar/reanudar no fragmenta el CSV.

**Captura automática: ON · detener y guardar** detiene el lector y guarda; queda desactivada hasta volver a activarla, incluso tras reiniciar LANDELTA. La X solo deja la aplicación en segundo plano. Salir desde la bandeja guarda y termina. El inicio automático de LANDELTA con Windows sigue siendo una opción explícita desde el modo ACC → Inicio; no se activa sin permiso.

**Mis sesiones** abre los CSV locales. **Analizar vueltas / exportar** abre el visor local para cargar un CSV y exportar el ZIP de sesión completa. Para analizar la sesión actual, detén primero la captura. No ejecutes AC y ACC simultáneamente: el lector de AC se suspende si detecta ACC para evitar confundir su memoria compartida.

Pruebas: lógica y persistencia portables, verificaciones de rutas de UI/arranque, compilación Windows, renderizado de ambos modos con el código real, instalación y reinstalación. La captura automática aún requiere validación dentro del juego en tu equipo.
