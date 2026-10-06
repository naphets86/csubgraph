# csubgraph

C++17-Implementierung des Subgraph-Algorithmus von Stephan Epp (2026). Die Bibliothek vergleicht zwei Graphen anhand ihrer Adjazenzmatrizen und bestimmt, ob und in welcher Richtung eine Subgraph-Beziehung besteht. Zusätzlich enthält das Projekt ein Kommandozeilenwerkzeug mit JSON-Schnittstelle sowie eine Testsuite auf Basis von Google Test.

## Inhalt

- [Projektstruktur](#projektstruktur)
- [Voraussetzungen](#voraussetzungen)
- [Build](#build)
- [Tests](#tests)
- [Verwendung als Bibliothek](#verwendung-als-bibliothek)
- [Kommandozeilenwerkzeug](#kommandozeilenwerkzeug)
- [API-Referenz](#api-referenz)
- [Algorithmus](#algorithmus)
- [Fehlerbehandlung](#fehlerbehandlung)
- [Einschränkungen](#einschränkungen)
- [Wissenschaftlicher Hintergrund](#wissenschaftlicher-hintergrund)
- [Erwerb](#erwerb)

## Projektstruktur

```
.
├── SubgraphAlgorithm.h              Klassendeklaration mit Dokumentationskommentaren
├── SubgraphAlgorithm.cpp            Implementierung des Algorithmus
├── Cli.cpp                          Kommandozeilenwerkzeug (JSON über stdin/stdout)
├── tests/
│   └── TestSubgraphAlgorithm.cpp    Unit-Tests (Google Test, 50 Testfälle)
├── doc/
│   └── tests.txt                    Protokoll eines Testlaufs
├── CMakeLists.txt                   Build-Konfiguration
├── LICENSE                          Lizenzbedingungen
└── README.md
```

## Voraussetzungen

- C++17-fähiger Compiler (getestet mit MinGW-w64/GCC; MSVC wird in der Build-Konfiguration berücksichtigt)
- CMake ab Version 3.10
- Internetzugang beim ersten Konfigurieren, da CMake die folgenden Abhängigkeiten per `FetchContent` herunterlädt:
  - nlohmann/json 3.11.2 (für das Kommandozeilenwerkzeug)
  - Google Test 1.14.0 (für die Tests)

## Build

Die folgenden Befehle sind plattformunabhängig. Unter Windows mit MinGW muss das Verzeichnis `bin` der MinGW-Installation sowie das von CMake im `PATH` liegen.

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Unter Windows mit MinGW wird der Generator explizit angegeben:

```powershell
$env:Path += ";<Pfad zu cmake>\bin"
$env:Path += ";<Pfad zu mingw64>\bin"

mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Der Build erzeugt drei Ziele:

| Ziel             | Art           | Beschreibung                                |
|------------------|---------------|---------------------------------------------|
| `subgraphlib`    | Bibliothek    | Der Algorithmus als statische Bibliothek    |
| `subgraph-cli`   | Programm      | Kommandozeilenwerkzeug mit JSON-Ein-/Ausgabe |
| `subgraph-tests` | Programm      | Testsuite                                   |

Bei Verwendung von GCC oder Clang werden Bibliothek und Tests mit `--coverage` übersetzt, sodass Abdeckungsberichte mit `gcov` erzeugt werden können.

## Tests

Ausführung über CTest oder direkt:

```
cd build
ctest --output-on-failure
```

```
./subgraph-tests          # Linux/macOS
.\subgraph-tests.exe      # Windows
```

Unter Windows müssen die Laufzeitbibliotheken von MinGW (`libgcc*.dll`, `libstdc++*.dll`, `libwinpthread*.dll`) auffindbar sein, in der Regel durch Aufnahme von `<Pfad zu mingw64>\bin` in den `PATH`.

Die Testsuite umfasst 50 Testfälle in neun Testgruppen:

| Testgruppe                 | Anzahl | Gegenstand                                                   |
|----------------------------|-------:|--------------------------------------------------------------|
| `SignatureCalculationTest` | 5      | Signaturen für 2x2- und 4x4-Matrizen, vollbesetzte Matrix, ungültige Eingaben |
| `LCSTest`                  | 7      | Identische, teilweise übereinstimmende und disjunkte Folgen, leere Folgen, große Werte |
| `RotationTest`             | 6      | Rotation um 0, 1, die volle Länge und Vielfache, leere und einelementige Folgen |
| `RowComponentsTest`        | 3      | Extraktion der Zeilenkomponenten                             |
| `MatrixValidationTest`     | 5      | Gültige, leere, nicht quadratische und nicht binäre Matrizen |
| `GraphComparisonTest`      | 9      | Identische, verschiedene, ungültige und wechselseitig enthaltene Graphen |
| `ResultStringTest`         | 5      | Textdarstellung der Ergebniswerte                            |
| `EdgeCasesTest`            | 5      | Einzelknoten, unzusammenhängende und vollständige Graphen, lange Ketten |
| `SubgraphRelationshipTest` | 5      | Subgraph-Beziehungen zwischen Graphen unterschiedlicher Größe |

Das Protokoll eines erfolgreichen Durchlaufs liegt unter `doc/tests.txt` (Kodierung UTF-16).

## Verwendung als Bibliothek

```cpp
#include "SubgraphAlgorithm.h"
#include <iostream>

int main() {
    // Gerichtete Kette über vier Knoten: 0 -> 1 -> 2 -> 3
    std::vector<std::vector<int>> graphA = {
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };

    // Kette über drei Knoten: 0 -> 1 -> 2
    std::vector<std::vector<int>> graphB = {
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0}
    };

    auto result = SubgraphAlgorithm::compareGraphs(graphA, graphB);
    std::cout << SubgraphAlgorithm::resultToString(result) << std::endl;
    // Ausgabe: KEEP_A (Graph A ist Subgraph von B oder hat gleiche Struktur)

    return 0;
}
```

Hinweis zur Textausgabe: Der von `resultToString` gelieferte Erläuterungstext zu `KEEP_A` und `KEEP_B` ist unglücklich formuliert. Maßgeblich ist die Semantik des Enum-Werts, wie unten beschrieben: `KEEP_A` bedeutet, dass Graph A den Graphen B enthält.

Zum Einbinden in ein eigenes CMake-Projekt genügt es, das Verzeichnis als Unterprojekt aufzunehmen und gegen `subgraphlib` zu linken:

```cmake
add_subdirectory(csubgraph)
target_link_libraries(meine_anwendung PRIVATE subgraphlib)
```

## Kommandozeilenwerkzeug

`subgraph-cli` liest ein JSON-Objekt von der Standardeingabe und schreibt das Ergebnis als JSON auf die Standardausgabe.

Eingabe:

```json
{
  "graph_a": [[0,1,0,0],[0,0,1,0],[0,0,0,1],[0,0,0,0]],
  "graph_b": [[0,1,0],[0,0,1],[0,0,0]]
}
```

Aufruf:

```
echo '{"graph_a":[[0,1,0,0],[0,0,1,0],[0,0,0,1],[0,0,0,0]],"graph_b":[[0,1,0],[0,0,1],[0,0,0]]}' | ./subgraph-cli
```

Ausgabe im Erfolgsfall:

```json
{"error":null,"result":"KEEP_A","result_code":0}
```

Zuordnung der Ergebniscodes:

| `result_code` | `result`       |
|--------------:|----------------|
| 0             | `KEEP_A`       |
| 1             | `KEEP_B`       |
| 2             | `KEEP_BOTH`    |
| 3             | `IDENTICAL`    |
| 4             | `EQUAL_KEEP_A` |
| 5             | `EQUAL_KEEP_B` |
| -1            | Fehlerfall     |

Im Fehlerfall ist `result` gleich `null`, `result_code` gleich `-1` und `error` enthält die Meldung. Fehlende Pflichtfelder werden auf der Standardausgabe gemeldet, Parse- und Validierungsfehler auf der Standardfehlerausgabe. Der Prozess endet in diesen Fällen mit einem von null verschiedenen Exit-Code.

## API-Referenz

Alle Funktionen sind statische Methoden der Klasse `SubgraphAlgorithm`.

### `compareGraphs`

```cpp
static Result compareGraphs(
    const std::vector<std::vector<int>>& graphA,
    const std::vector<std::vector<int>>& graphB
);
```

Vergleicht zwei Graphen und bestimmt ihre Subgraph-Beziehung. Die Graphen dürfen unterschiedlich viele Knoten besitzen. Bei ungültigen Matrizen wird `std::invalid_argument` ausgelöst.

Rückgabewerte:

| Wert           | Bedeutung                                                                                  |
|----------------|--------------------------------------------------------------------------------------------|
| `IDENTICAL`    | Beide Graphen haben gleiche Knotenzahl und identische Zeilenkomponenten.                   |
| `KEEP_A`       | B ist in A enthalten, A nicht in B. A ist die umfassendere Struktur und wird behalten.     |
| `KEEP_B`       | A ist in B enthalten, B nicht in A. B ist die umfassendere Struktur und wird behalten.     |
| `KEEP_BOTH`    | Keine Subgraph-Beziehung in einer der beiden Richtungen.                                   |
| `EQUAL_KEEP_A` | Beide Graphen sind wechselseitig enthalten, gleiche Knotenzahl, A hat mindestens so viele Kanten wie B. |
| `EQUAL_KEEP_B` | Wie `EQUAL_KEEP_A`, jedoch hat B mehr Kanten als A.                                        |

Sind die Graphen wechselseitig enthalten, aber unterschiedlich groß, wird der größere Graph behalten (`KEEP_A` beziehungsweise `KEEP_B`).

### `calculateSignatures`

```cpp
static std::vector<uint64_t> calculateSignatures(
    const std::vector<std::vector<int>>& matrix
);
```

Berechnet für jede Spalte j einer n x n-Adjazenzmatrix die Signatur

    σ_j = Σ_{i=0}^{n-1} A_ij · 2^i + j · 2^n

Der erste Summand kodiert den Spalteninhalt als Bitmuster, der zweite Summand die Spaltenposition. Löst `std::invalid_argument` aus, wenn die Matrix ungültig ist.

### `extractRowComponents`

```cpp
static std::vector<uint64_t> extractRowComponents(
    const std::vector<uint64_t>& signatures, size_t n
);
```

Entfernt aus den Signaturen den Positionsanteil `j · 2^n` und liefert nur die Bitmuster der Spalten.

### `rotateSequence`

```cpp
static std::vector<uint64_t> rotateSequence(
    const std::vector<uint64_t>& seq, size_t rotation
);
```

Zyklische Rechtsrotation einer Folge um `rotation` Positionen. Die Rotation wird modulo der Länge reduziert; leere Folgen werden unverändert zurückgegeben.

### `computeLCS`

```cpp
static size_t computeLCS(
    const std::vector<uint64_t>& seqA,
    const std::vector<uint64_t>& seqB
);
```

Liefert die Länge der längsten gemeinsamen zusammenhängenden Teilfolge (Longest Common Substring) zweier Folgen mittels dynamischer Programmierung. Im Gegensatz zur längsten gemeinsamen Teilsequenz (Subsequence) müssen die übereinstimmenden Elemente in beiden Folgen unmittelbar aufeinanderfolgen.

### `isValidAdjacencyMatrix`

```cpp
static bool isValidAdjacencyMatrix(const std::vector<std::vector<int>>& matrix);
```

Prüft, ob die Matrix nicht leer, quadratisch und ausschließlich mit den Werten 0 und 1 besetzt ist.

### `resultToString`

```cpp
static std::string resultToString(Result result);
```

Liefert eine textuelle Beschreibung eines Ergebniswerts (siehe Hinweis im Abschnitt zur Verwendung).

## Algorithmus

`compareGraphs` arbeitet in folgenden Schritten:

1. Beide Matrizen werden validiert.
2. Für beide Graphen werden die Signaturen berechnet und auf ihre Zeilenkomponenten reduziert.
3. Haben beide Graphen gleiche Knotenzahl und identische Zeilenkomponenten, lautet das Ergebnis `IDENTICAL`.
4. Ist ein Graph nicht kleiner als der andere, wird die Folge der Zeilenkomponenten des größeren Graphen nacheinander um alle Positionen zyklisch rotiert und jeweils mit der Folge des kleineren Graphen verglichen. Gilt für mindestens eine Rotation, dass die längste gemeinsame zusammenhängende Teilfolge die Länge 2 oder mehr hat, gilt der kleinere Graph als enthalten.
5. Aus den beiden Enthaltenseinsrichtungen wird das Ergebnis abgeleitet (siehe Tabelle unter `compareGraphs`).

Die Verwendung zyklischer Rotationen anstelle vollständiger Permutationen reduziert die Zahl der Vergleiche von n! auf n.

### Komplexität

| Größe       | Wert    | Begründung                                                  |
|-------------|---------|-------------------------------------------------------------|
| Zeit        | O(n³)   | n Rotationen, je ein Vergleich mit Aufwand O(n²)            |
| Speicher    | O(n²)   | Tabelle der dynamischen Programmierung                      |

Dabei bezeichnet n die Knotenzahl des größeren Graphen.

## Fehlerbehandlung

Die Bibliothek löst `std::invalid_argument` aus bei:

- leeren Matrizen
- nicht quadratischen Matrizen
- Einträgen, die weder 0 noch 1 sind

```cpp
try {
    auto result = SubgraphAlgorithm::compareGraphs(emptyMatrix, graphB);
} catch (const std::invalid_argument& e) {
    std::cerr << "Fehler: " << e.what() << std::endl;
}
```

## Einschränkungen

- Die Signaturen werden als `uint64_t` gespeichert. Die Zeilenkomponenten belegen n Bit, daher ist die Implementierung auf Graphen mit höchstens 63 Knoten ausgelegt. Eine Prüfung dieser Grenze erfolgt derzeit nicht; bei größeren Matrizen ist das Verhalten nicht definiert.
- Eingaben müssen binäre Adjazenzmatrizen sein. Kantengewichte werden nicht unterstützt.
- Gerichtete Graphen werden unterstützt, da die Matrix nicht symmetrisch sein muss. Schleifen (Einträge auf der Diagonalen) sind zulässig.
- Das Kriterium einer Subgraph-Beziehung (längste gemeinsame zusammenhängende Teilfolge von mindestens 2) ist ein Merkmal des implementierten Verfahrens. Maßgeblich für seine Begründung ist die unten genannte Arbeit.

## Wissenschaftlicher Hintergrund

Das Verfahren beruht auf folgenden Bausteinen:

- eindeutige Spaltensignaturen aus Bitmuster und Spaltenposition
- zyklische Rotationen zur Berücksichtigung der Ordnung
- Vergleich über die längste gemeinsame zusammenhängende Teilfolge mit Schwellwert 2

Die formale Herleitung einschließlich der Aussagen zu Korrektheit und Optimalität findet sich in:

- Epp, S. (2026): *Der Subgraph Algorithmus*, Datei `subgraph.tex` im Verzeichnis `science/` des Repositoriums https://github.com/naphets86/subgraph

## Mögliche Erweiterungen

- Adjazenzlisten statt Matrizen für dünn besetzte Graphen
- Parallelisierung der Rotationsvergleiche
- Berücksichtigung von Kantengewichten in den Signaturen
- Näherungsverfahren für große Graphen
- Unterstützung von Graphen mit mehr als 63 Knoten durch breitere Signaturen

## Erwerb

Der Preis für diese Software beträgt 3.145.000,00 EUR.

### Zahlungsinformationen

Name: Stephan Epp  
IBAN: DE24 5003 1900 0012 5603 20
BIC: BBVADEFFXXX
