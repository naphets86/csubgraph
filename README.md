# Subgraph Algorithmus C++ Implementierung

Vollständige C++ 17-Implementierung des Subgraph Algorithmus von **Stephan Epp (2026)**.

## Projektstruktur

```
.
├── SubgraphAlgorithm.h          # Header-Datei mit Klasse-Definition
├── SubgraphAlgorithm.cpp        # Implementierung (ca. 200 Zeilen)
├── SubgraphTest.cpp             # Unit Tests mit Google Test
├── CMakeLists.txt               # Build-Konfiguration
└── README.md                    # Diese Datei
```

## Build

### 1. MinGW & CMake zum PATH hinzufügen (falls noch nicht)
```powershell
$env:Path += ";C:\Users\...\Downloads\cmake-4.4.0-rc3-windows-x86_64\bin"
$env:Path += ";C:\Users\...\Downloads\mingw64\bin"
```

### 2. Build-Verzeichnis erstellen

```powershell
mkdir build
cd build
```

### 3. CMake konfigurieren

```powershell
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

### 4. Kompilieren

```powershell
cmake --build . --config Release
```

## Test-Abdeckung

### Tests ausführen

```
# MinGW bin-Verzeichnis zum PATH hinzufügen
$env:Path = "C:\Users\...\Downloads\mingw64\bin;$env:Path"

# Verifiziere, dass die DLLs da sind
ls C:\Users\...\Downloads\mingw64\bin\libgcc*.dll
ls C:\Users\...\Downloads\mingw64\bin\libstdc++*.dll

# Jetzt wieder testen
cd C:\Users\...\Git\csubgraph\build
.\subgraph-tests.exe
```

## Getestete Funktionen

### `calculateSignatures()`
```cpp
std::vector<uint64_t> SubgraphAlgorithm::calculateSignatures(
    const std::vector<std::vector<int>>& matrix
);
```
Berechnet eindeutige Signaturen: σ_j = Σ(A_ij * 2^i) + j * 2^n

**Tests:**
- 2x2, 3x3, 4x4 Matrizen
- Injektivität (alle Signaturen unterschiedlich)
- Fehlgeschlagene Eingaben (leere, ungültige Matrizen)

### `computeLCS()`
```cpp
size_t SubgraphAlgorithm::computeLCS(
    const std::vector<uint64_t>& seqA,
    const std::vector<uint64_t>& seqB
);
```
Berechnet längste gemeinsame Teilsequenz mittels dynamischer Programmierung.

**Tests:**
- Identische, teils gemeinsame, disjunkte Sequenzen
- Edge Cases (leere Sequenzen)
- Große Werte (uint64_t)

### `rotateSequence()`
```cpp
std::vector<uint64_t> SubgraphAlgorithm::rotateSequence(
    const std::vector<uint64_t>& seq,
    size_t rotation
);
```
Zyklische Rechtsrotation einer Sequenz.

**Tests:**
- Rotation 0, 1, full, modulo
- Leere und Single-Element-Sequenzen

### `compareGraphs()`
```cpp
Result SubgraphAlgorithm::compareGraphs(
    const std::vector<std::vector<int>>& graphA,
    const std::vector<std::vector<int>>& graphB
);
```
Vergleicht zwei Graphen und bestimmt Subgraph-Beziehung.

**Rückgabewerte:**
- `KEEP_A` - Graph A ist Subgraph von B oder hat gleiche Struktur
- `KEEP_B` - Graph B ist Subgraph von A oder hat gleiche Struktur
- `KEEP_BOTH` - Keine Subgraph-Beziehung
- `IDENTICAL` - Graphen sind identisch

**Tests:**
- Identische Graphen
- Subgraph-Beziehungen
- Verschiedene Größen
- Validierung (leere, ungültige Matrizen)

## Beispiel-Verwendung

```cpp
#include "SubgraphAlgorithm.h"
#include <iostream>

int main() {
    // Definiere zwei Graphen als Adjazenzmatrizen
    std::vector<std::vector<int>> graphA = {
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };
    
    std::vector<std::vector<int>> graphB = {
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };
    
    // Vergleiche Graphen
    auto result = SubgraphAlgorithm::compareGraphs(graphA, graphB);
    
    std::cout << "Ergebnis: " 
              << SubgraphAlgorithm::resultToString(result) 
              << std::endl;
    
    // Output: "KEEP_B (Graph B ist Subgraph von A ...)"
    
    return 0;
}
```

## Algorithmus-Details

### Komplexität
- **Zeitkomplexität:** O(n^3)
- **Speicherkomplexität:** O(n^2)

### Korrektheit
Der Algorithmus nutzt:
1. **Eindeutige Signaturen** für Spalten (injektive Funktion)
2. **Zyklische Rotationen** statt vollständiger Permutationen (nur n statt n!)
3. **LCS-Vergleich** zur Subgraph-Erkennung (LCS ≥ 2)

### Optimalität
Unter SETH (Strong Exponential Time Hypothesis) ist der Algorithmus asymptotisch optimal.

## Fehlerbehandlung

Der Code wirft `std::invalid_argument` Exception für:
- Leere Matrizen
- Nicht-quadratische Matrizen
- Nicht-binäre Einträge (nur 0 und 1 erlaubt)
- Größen-Mismatches

**Beispiel:**
```cpp
try {
    auto result = SubgraphAlgorithm::compareGraphs(emptyMatrix, graphB);
} catch (const std::invalid_argument& e) {
    std::cerr << "Fehler: " << e.what() << std::endl;
}
```

## Wissenschaftlicher Hintergrund

Der Algorithmus ist basiert auf:
- **Epp, S. (2026)**: "Der Subgraph Algorithmus"
- **Signatur-Basis**: Injektive Spaltengewichtung
- **Rotations-Basis**: Zyklische Ordnung-Erhaltung
- **Subgraph-Kriterium**: LCS ≥ 2

Siehe `subgraph.tex` in meinem Repository https://github.com/hjstephan86/subgraph für die vollständige formale Herleitung.

## Erweiterungsmöglichkeiten

1. **Sparse Graphs**: Adjazenzlisten statt Matrizen
2. **Parallelisierung**: Rotation-Vergleiche parallel
3. **Gewichtete Graphen**: Kantengewichte in Signaturen
4. **Gerichtete Graphen**: Bereits unterstützt
5. **Approximative Algorithmen**: Für große Graphen
