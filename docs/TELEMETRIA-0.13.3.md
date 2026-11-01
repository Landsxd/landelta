# Sesión completa (0.13.3)

En Analizar vueltas, carga el CSV y pulsa **Exportar sesión completa**. El ZIP contiene el CSV original byte por byte, resumen de todas las vueltas con los microsectores elegidos, sesion.json y una explicación. A/B y el zoom no limitan la exportación. Los archivos originales no se modifican. No es necesario compartir un video.

Cruces de meta: se necesitan registros adyacentes de ambas vueltas, misma tanda, posiciones próximas a meta, hueco máximo de 250 ms y reloj coherente con el último tiempo oficial. Se añaden límites estimados para el cálculo, nunca filas al original. Sin esa evidencia quedan huecos. Una vuelta se marca completa solo con ambos cruces observados; entrada y salida de boxes son indicadores independientes. Capturar ya dentro de boxes no permite reconstruir una entrada anterior.

Sectores: solo se toma un cambio secuencial observado del índice de sector, y un last_sector_ms nuevo dentro de 250 ms. El dato heredado no se usa. Si el nuevo tiempo coincide exactamente con el anterior y no puede distinguirse, queda desconocido. S3 sigue siendo derivado del total menos S1 y S2; no una lectura oficial de S3. La validez oficial se conserva como desconocida: este lector no dispone de una bandera certificada por vuelta. Ruedas fuera solo genera aviso.

La calidad muestra frecuencia de paquetes nuevos de física y gráficos por separado (cambios de ID), número de filas y huecos entre capturas. No confunde interpolación ni filas duplicadas con mediciones adicionales. Las pausas dentro del archivo afectan a la frecuencia media.

Capturas nuevas incluyen versión del juego, compuesto y valores nativos de ABS/TC, embrague/blip automático y estabilidad; el original conserva sus cambios por muestra. sesion.json muestra las condiciones iniciales. Circuito, variante, coche y longitud ya se guardaban. Versión del mod/coche y temperaturas ambiente/pista todavía no se capturan: figuran como null. CSV antiguos mantienen como desconocidos los campos que no tienen. No se extrapolan grados del volante: steer_native conserva el canal del juego, sin afirmar que equivale a ángulo físico del volante. Pedales son entrada normalizada, no presión en kg o bar.

Validación automatizada: cruces con/sin evidencia, datos de sectores heredados, avisos fuera de pista, transiciones de boxes, paquetes repetidos y CRC/contenido del ZIP. Aún requiere comprobación de una práctica real en tu instalación; no certifica precisión absoluta de milisegundos.
