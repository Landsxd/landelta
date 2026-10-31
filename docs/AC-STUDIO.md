# AC Studio — experimental, no validado en pista

Dentro de LANDELTA, pulsa **AC Studio** (arriba a la derecha). ACC conserva sus paneles actuales. AC Studio es un grabador separado para Assetto Corsa (2014), incluidos mods que expongan la memoria estándar. No usa el UDP de ACC ni modifica el juego.

1. Extrae el paquete completo, incluido `tools/ac-analyzer.html`.
2. Abre AC, entra en pista y pulsa **Grabar AC**. No ejecutes AC y ACC simultáneamente: comparten nombres de memoria.
3. Se guardan CSV + JSON en `%LOCALAPPDATA%\VortexACC\ac-telemetry`. **Abrir sesiones** lleva allí.
4. Pulsa **Detener y guardar**, abre **Analizar vueltas** y carga el CSV.
5. Selecciona dos vueltas y 20, 50, 100 o 200 microsectores. Exporta el análisis JSON y adjúntalo al chat junto con el CSV y el JSON de metadatos.

Registra muestras nuevas de física: gas, freno, velocidad, dirección nativa, marcha nativa, RPM, combustible y aceleraciones; además copia tiempo de vuelta, sector, posición normalizada y coordenadas de la página gráfica. Las dos páginas tienen relojes/actualizaciones independientes; se conservan ambos identificadores de paquete. La frecuencia solicitada de sondeo es 10 ms, no una promesa de 100 Hz. Se guarda el hueco entre muestras y la interfaz muestra la frecuencia media observada.

No se inventan muestras entre lecturas. Los microsectores se interpolan sobre la posición normalizada y el reloj de vuelta. Son segmentos iguales de avance normalizado, no curvas identificadas ni distancias exactas en metros. Los huecos superiores a 250 ms y los saltos hacia atrás no se interpolan. El primer y último microsector pueden estar vacíos por falta de una muestra exactamente en meta. Las vueltas parciales se conservan, no se presentan como completas. Milésimas mostradas no equivalen a exactitud de milisegundos.

La validez oficial se considera desconocida: ruedas fuera de pista y boxes son avisos, no certificación. No se graban replays como sesiones en vivo. No recupera telemetría de prácticas anteriores que no se grabaron. Cambios de auto/circuito/tipo de sesión, retroceso de vueltas o reinicio del reloj abren archivos nuevos. Un reinicio de sesión del mismo tipo sin cambios observables puede no detectarse; detén y reinicia la captura en ese caso.

Los archivos permanecen locales. No se envía nada al chat automáticamente. No se guardan nombres del piloto. El JSON de análisis no contiene todas las muestras: adjunta también el CSV para estudiar frenada/trail braking. No garantiza reconstruir tráfico, incidentes ni información visual que el juego no exponga.

Pendiente: validación en Windows con AC y RSS Supreme 25, mejores gráficos dentro de la ventana nativa, selección de curvas manual y pruebas con CSP. No es una actualización estable ni reemplaza la beta ACC publicada.

Referencia del formato: https://github.com/mdjarv/assettocorsasharedmemory (Physics.cs, Graphics.cs, StaticInfo.cs). Se usa un lector original de los prefijos del formato, no código de captura copiado.
