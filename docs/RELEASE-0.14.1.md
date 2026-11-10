# LANDELTA 0.14.1 — revisión de Race Studio

Conserva las funciones y limitaciones descritas en [0.14.0](RELEASE-0.14.0.md).

- Una carga de historial en segundo plano ya no puede reemplazar una selección posterior de juego ni volver a sacar al usuario del directo.
- Posiciones, visibilidad y tamaño de los overlays se editan como borrador: Guardar aplica; Cancelar descarta. Se avisa antes de descartar al cambiar de pestaña/juego.
- Vuelve el ajuste independiente del tamaño de Delta en Tu pantalla.
- El resumen se recalcula con las últimas muestras al pausar/cerrar, aunque no haya transcurrido el segundo del refresco normal.
- Se espera la actualización del tiempo oficial al cruzar meta; no se acepta ciegamente el valor arrastrado de la vuelta anterior. Tiempos idénticos requieren estabilidad y un cruce compatible. Si no se puede confirmar, permanece incompleto.
- Se admiten sectores que llegan hasta 500 ms después de su cambio de índice, sin reutilizar el valor anterior ni capturarlos dos veces.
- La mejor registrada excluye capturas parciales. Los huecos también se cuentan entre vueltas y no se unen microsectores entre stints.
- Los CSV nuevos de ACC/iRacing conservan delta, mejor tiempo y stint al reabrirlos. Los CSV anteriores siguen siendo legibles; no se inventan canales ausentes.
- Pruebas nuevas: tiempos atrasados/idénticos, huecos, sectores atrasados, cancelación de lectura, cierre rápido, CSV y acciones nativas de Guardar/Cancelar/perfiles en Windows.

Pendiente de validación real: conducción en AC/ACC/iRacing, transiciones entre sesiones y escalado/múltiples monitores. iRacing sigue experimental. ACC/iRacing registran hasta 10 muestras nuevas por segundo; no es captura completa a la frecuencia del SDK.
