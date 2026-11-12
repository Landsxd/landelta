# AC Studio 0.13.1-dev — una sesión, un CSV

Pulsa **AC Studio → Grabar AC** una vez. Repetir el botón mientras está activo no inicia otra captura. **Detener y guardar** termina la grabación. Cambiar de circuito, coche, tipo de sesión o proceso del juego crea una sesión distinta; cruzar meta, pausar, entrar en replay o volver a boxes no fragmenta el archivo. No se graban muestras de replay.

El archivo se guarda en `%LOCALAPPDATA%\VortexACC\ac-telemetry`, con nombre `circuito_layout_AC_fecha-en-milisegundos-contador.csv`. El nombre técnico del circuito viene del juego. No se genera JSON auxiliar. Los metadatos de circuito, coche y layout están dentro del CSV, codificados como URL cuando contienen caracteres especiales. El visor los decodifica. Conserva ambos juegos separados: no ejecutes AC y ACC simultáneamente.

El CSV es la fuente completa de muestras: vueltas, reloj, último tiempo completado, sectores oficiales observados, gas y freno (0–1), volante en unidades nativas, velocidad, posición y calidad de lectura. `microsector_50` identifica uno de 50 tramos de avance normalizado. Un reinicio de vuelta fuera de meta se guarda como una tanda nueva (`stint`) dentro del mismo archivo; el número original del juego se conserva como `raw_lap`. Validez oficial desconocida, nunca se deduce de un tiempo rápido ni se confunde con cobertura parcial.

**Analizar vueltas** abre el visor gráfico local. Carga el CSV, selecciona dos vueltas/tandas, ajusta el zoom por porcentaje de pista y mueve el ratón: el cursor sincroniza las cuatro gráficas. Muestra gas y freno en porcentaje, no fuerza física. El volante se mantiene en unidades nativas, sin inventar grados. El visor requiere el ZIP completo, incluido `tools/ac-analyzer.html`.

Puedes seleccionar 20, 50, 100 o 200 microsectores. La tabla muestra sus tiempos y diferencias interpoladas. **Exportar tabla CSV** es opcional: genera una tabla pequeña de una fila por vuelta, con tiempos y todos los microsectores seleccionados. No contiene las muestras de pedales: para análisis completo en un chat usa el CSV original. **Exportar análisis JSON** también es opcional. Ninguno se crea automáticamente.

Las gráficas y cálculos funcionan localmente en el navegador. No se envían datos a servidores. Los sectores extremos pueden quedar vacíos si falta cobertura de meta; no se inventan esos tiempos. El tiempo final de una vuelta se confirma en la siguiente. Los huecos superiores a 250 ms no se interpolan. La frecuencia de sondeo solicitada no garantiza frecuencia efectiva ni exactitud de milisegundos.

Los CSV antiguos siguen siendo legibles, pero no se unen automáticamente: no contienen identidad de circuito/auto suficiente para fusionarlos con seguridad. Esta actualización evita la fragmentación en capturas nuevas. Se necesita repetir una práctica real para validar la corrección en tu instalación.
