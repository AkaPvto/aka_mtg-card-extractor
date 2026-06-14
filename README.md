# Aka MTG Card Extractor

A command-line application written in C++ designed to interact with the [MTGJSON v5](https://mtgjson.com/api/v5/) API.

The main goal of this tool is to download Magic: The Gathering set data and extract only unique functional cards — ignoring reprints, promotional cards, and alternate art versions. The output is exported as clean, well-structured **Markdown** files, ideal for importing into AI tools such as **NotebookLM**.

## Features

- 🚀 **Written in modern C++**: Uses `libcurl` for HTTP requests and `nlohmann/json` for fast JSON parsing.
- 🧹 **Automatic filtering**: Automatically filters out any card flagged as `isReprint`, `isAlternative`, or `isPromo` in the MTGJSON database, ensuring no duplicate versions of the same card appear in the output.
- 📝 **NotebookLM-optimized output**: Generates `.md` files where each card has its own section detailing its mana cost, type, power/toughness, and rules text — no messy JSON syntax.
- 🔮 **Future-proof**: You can query the up-to-date set list using the `--list` command and extract new expansions as they are released.

## Prerequisites

To build the project you will need:
- A compiler with **C++17** support (e.g. `g++` or `clang++`).
- **CMake** (version 3.10 or higher).
- **libcurl** (e.g. the `libcurl4-openssl-dev` package on Ubuntu/Debian).

*(Note: The `nlohmann/json.hpp` parsing library is already bundled in the source tree under `third_party/` for convenience.)*

## Building

```bash
# 1. Create and enter the build directory
mkdir build
cd build

# 2. Configure the project with CMake
cmake ..

# 3. Build the executable
make
```

Once done, the `mtg_extractor` binary will be available inside the `build/` folder.

## Usage

### 1. List sets
Lists the code, release date, type, and name of all sets available on MTGJSON.
```bash
./mtg_extractor --list
```
*Tip: Narrow the list with `--setType`, e.g. `--list --setType core` to show only core sets.*

### 2. Export a single set
Downloads the JSON for the requested expansion, processes its unique cards, and writes a Markdown file.
```bash
./mtg_extractor --set ONE
```

### 3. Export all sets
Downloads and processes the entire database, one expansion at a time.
```bash
./mtg_extractor --all
```
*Note: This downloads hundreds of expansions. Consider adding `--setType expansion` to limit the output to main sets and skip supplemental products.*

### 4. Export the most recent set
Downloads only the latest set released.
```bash
./mtg_extractor --last
```

### Extra options

These can be combined with any of the actions above:

| Flag | Description |
|------|-------------|
| `--setType <TYPE>` | Filter by set type (e.g. `expansion`, `commander`, `core`, `masters`). Only sets matching this type will be listed or exported. |
| `--outDir <DIR>` | Base output directory for exported files. Defaults to `extraction`. |
| `--prune` | Automatically deletes any Markdown files that end up empty (sets where all cards were promos or reprints). |

**Example combining multiple flags**:
```bash
./mtg_extractor --all --setType commander --prune --outDir ./commander_sets
```

---

## Sample Markdown output

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
