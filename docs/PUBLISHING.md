# Publicar LANDELTA en GitHub

Configuración preparada: repositorio **landelta**, público y con código. Descripción sugerida:

> Transparent telemetry overlays for Assetto Corsa Competizione on Windows. Standings, relatives, lap/sector timing, pedals, delta and track history.

Temas sugeridos: `assetto-corsa-competizione`, `simracing`, `telemetry`, `overlay`, `windows`, `cpp`.

## Con GitHub Desktop

1. Inicia sesión en GitHub Desktop.
2. Crea un repositorio local llamado `landelta` desde **File → New repository**. No elijas una licencia automáticamente: la decisión sobre la licencia del código sigue pendiente.
3. Copia el contenido de la carpeta preparada a la raíz de ese repositorio. Incluye `.github`, `.gitignore` y `.gitattributes`; no copies una carpeta `.git` de otro lugar.
4. Comprueba que README, `src/`, `tests/` y `vendor/` estén en la raíz, sin una carpeta contenedora adicional.
5. Revisa el listado de archivos. Deben quedar fuera EXE, ZIP, configuraciones del juego, historiales y resultados de pruebas.
6. Haz el commit inicial: **Initial LANDELTA 0.11 beta**.
7. Pulsa **Publish repository**, desmarca **Keep this code private** y publica en tu cuenta.
8. Revisa la pestaña Actions: el flujo debe ejecutar las pruebas portables y compilar Windows. El flujo está preparado, pero su ejecución real aún debe comprobarse en GitHub.

## Publicar la descarga

El flujo **Build and test** puede publicar esta primera beta tras superar las pruebas y la compilación en Windows. En **Actions → Build and test → Run workflow**, elige `main` y activa `publish_beta`. También se activa con el marcador `[release v0.11.0]` en el mensaje del commit de publicación en `main`.

Publica como pre-release, adjunta EXE, ZIP con fuentes y avisos, y SHA256SUMS.txt. El empaquetado excluye los diseños exportados y solo utiliza archivos del repositorio, nunca configuraciones o historial locales. Si `v0.11.0` ya existe, no reemplaza sus archivos. Para una versión futura deben actualizarse el tag, la condición y las notas antes de publicarla.

Alternativa manual:

En el repositorio, abre **Releases → Draft a new release**:

- Tag nuevo: `v0.11.0`.
- Target: `main`.
- Título: **LANDELTA 0.11 — Windows Beta**.
- Notas: contenido de `docs/RELEASE-v0.11.0.md`.
- Marca **This is a pre-release**.
- Adjunta `LANDELTA-Windows-Beta.zip` y `LANDELTA.exe` de la beta 0.11 verificada.
- Revisa archivos y notas antes de publicar.

No subas el ZIP como sustituto del código del repositorio. El código va en archivos y las descargas ejecutables como assets de Releases. La descarga automática Source code no contiene el ejecutable si se respeta el `.gitignore`.

## Donaciones

No se ha agregado ningún destinatario ni enlace de pago. Cuando se elija la plataforma y exista tu enlace, se podrá añadir al README y a `.github/FUNDING.yml`.
