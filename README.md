# MTGJSON Data Extractor CLI

Una aplicación de línea de comandos (CLI) escrita en C++ diseñada para interactuar con la API de [MTGJSON v5](https://mtgjson.com/api/v5/). 

El objetivo principal de esta herramienta es descargar datos de expansiones de Magic: The Gathering y extraer únicamente las cartas funcionales únicas (ignorando reimpresiones, cartas promocionales y artes alternativos). El resultado se exporta en archivos **Markdown** limpios y bien estructurados, ideales para ser importados y procesados por herramientas de Inteligencia Artificial como **NotebookLM**.

## Características

- 🚀 **Escrito en C++ moderno**: Utiliza `libcurl` para conexiones HTTP y la librería `nlohmann/json` para un parseo rápido de JSON.
- 🧹 **Filtrado Automático**: Filtra automáticamente cualquier carta marcada como `isReprint`, `isAlternative` o `isPromo` en la base de datos de MTGJSON, asegurándote de no tener versiones duplicadas de la misma carta en la salida final.
- 📝 **Salida Optimizada para NotebookLM**: Genera archivos `.md` donde cada carta tiene su propia sección detallando su coste de maná, tipo, fuerza/resistencia y reglas, sin sintaxis JSON confusa.
- 🔮 **A prueba de futuro**: Puedes consultar la lista actualizada de sets usando el comando `list` y extraer nuevas expansiones a medida que vayan saliendo a la venta.

## Requisitos Previos

Para compilar el proyecto, necesitarás:
- Un compilador con soporte para **C++17** (ej. `g++` o `clang++`).
- **CMake** (versión 3.10 o superior).
- **libcurl** (ej. paquete `libcurl4-openssl-dev` en Ubuntu/Debian).

*(Nota: La librería de parseo de JSON `nlohmann/json.hpp` ya está incluida en el código fuente para facilitar la compilación).*

## Compilación

Sigue estos pasos para compilar el ejecutable:

```bash
# 1. Crea y entra al directorio de construcción
mkdir build
cd build

# 2. Configura el proyecto con CMake
cmake ..

# 3. Compila el ejecutable
make
```

Una vez terminado, tendrás el ejecutable `mtg_extractor` en la carpeta `build/`.

## Modo de Uso

La herramienta ahora funciona mediante una sintaxis basada en argumentos (flags) muy versátil. Puedes usar las siguientes acciones principales:

### 1. Listar Sets
Lista el código (SET_CODE), fecha, tipo y nombre de todas las expansiones disponibles en MTGJSON. 
```bash
./mtg_extractor --list
```
*Tip: Puedes reducir la lista usando `--setType`, por ejemplo `--list --setType core` para ver solo colecciones básicas.*

### 2. Exportar un Set (Markdown)
Descarga el JSON de la expansión solicitada, procesa las cartas únicas y crea un archivo Markdown.
```bash
./mtg_extractor --set ONE
```

### 3. Exportar TODO de golpe
Descarga la base de datos entera procesada expansión por expansión. 
```bash
./mtg_extractor --all
```
*Advertencia: Descargará y procesará cientos de expansiones. Ideal si le añades el filtro `--setType expansion` para descargar solo juegos principales y evitar colecciones suplementarias raras.*

### 4. Exportar el Set más Reciente
Si tan solo te interesa bajarte la última colección salida al mercado:
```bash
./mtg_extractor --last
```

### Opciones Extra (Filtros y Directorios)
Puedes mezclar las acciones anteriores con las siguientes opciones extra:
- `--setType <TIPO>`: (ej: `expansion`, `commander`, `core`, `masters`). Aplica este filtro y solo descargará o listará las expansiones que pertenezcan a ese tipo concreto. 
- `--outDir <CARPETA>`: Determina la carpeta base donde se crearán los exports. Por defecto es `extraction`.
- `--prune`: Si se añade, la app detectará y borrará automáticamente los archivos Markdown que se generen vacíos (sets donde se exportaron 0 cartas porque todas eran promos/reprints). ¡Perfecto para mantener la carpeta de extracción 100% limpia sin archivos inútiles!

**Ejemplo complejo combinando flags**:
```bash
./mtg_extractor --all --setType commander --prune --outDir ./sets_de_commander
```

---

## Preguntas Frecuentes (FAQ)

**1. ¿Qué es el archivo `ARN_cards.md`?**
Es un archivo de ejemplo que generamos como prueba. Contiene única y exclusivamente las cartas funcionales (sin incluir reimpresiones ni versiones promocionales) correspondientes a la expansión clásica *Arabian Nights (ARN)*.

**2. ¿A dónde van a parar los archivos exportados?**
Por defecto, si no especificas nada, los archivos (como `ONE_cards.md` o `ARN_cards.md`) se guardarán en **el directorio actual** en el que estés ejecutando la terminal (habitualmente representando por el punto `.`).
Sin embargo, **puedes especificar cualquier carpeta de salida** como parámetro final. Por ejemplo:
`./build/mtg_extractor export ARN /home/akapvto/mis_archivos/`

---

## Ejemplo de Salida Markdown

```markdown
# Phyrexia: All Will Be One (ONE)

## Elesh Norn, Mother of Machines
**Mana Cost**: {4}{W}
**Type**: Legendary Creature — Phyrexian Praetor
**Power/Toughness**: 4/7
**Text**: 
> Vigilance
> If a permanent entering the battlefield causes a triggered ability of a permanent you control to trigger, that ability triggers an additional time.
```
