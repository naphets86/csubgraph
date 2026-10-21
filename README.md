# csubgraph

C++17-Implementierung des Subgraph Algorithmus von Stephan Epp (2026). Die Bibliothek vergleicht zwei Graphen anhand ihrer Adjazenzmatrizen und bestimmt, ob und in welcher Richtung eine Subgraph-Beziehung besteht. Darüber hinaus enthält das Projekt:

- **`MultiOmics`**: die Erweiterung auf Schichtstapel (mehrere Adjazenzmatrizen über derselben Knotenmenge) mit Integrationsoperatoren und einer Mehrschicht-Relation,
- **`ExactSubgraph`**: einen *exakten* Teilgraphtest (Subgraph-Monomorphismus) ohne Knotengrenze,
- **`subgraph-cli`**: ein Kommandozeilenwerkzeug mit JSON-Schnittstelle,
- **`subgraph`**: einen Datei-Adapter für den [Subgraph-SAT-Solver](https://github.com/hjstephan86/subgraph-sat-solver),
- eine Testsuite auf Basis von Google Test (144 Testfälle),
- die wissenschaftliche Arbeit zur Wiederverwendung des Algorithmus (Verzeichnis `science/`).

`csubgraph` ist die erste Wiederverwendung des in Python entwickelten Algorithmus (Repository `subgraph`) und selbst der gemeinsame Kern, auf dem mehrere andere Projekte aufbauen (siehe [Einsatz in anderen Projekten](#einsatz-in-anderen-projekten)).

## Inhalt

- [Überblick über die Module](#überblick-über-die-module)
- [Voraussetzungen](#voraussetzungen)
- [Build](#build)
- [Tests](#tests)
- [Verwendung als Bibliothek](#verwendung-als-bibliothek)
- [Kommandozeilenwerkzeug `subgraph-cli`](#kommandozeilenwerkzeug-subgraph-cli)
- [Adapter für den Subgraph-SAT-Solver](#adapter-für-den-subgraph-sat-solver)
- [API-Referenz `SubgraphAlgorithm`](#api-referenz-subgraphalgorithm)
- [API-Referenz `MultiOmics`](#api-referenz-multiomics)
- [API-Referenz `ExactSubgraph` und `BitGraph`](#api-referenz-exactsubgraph-und-bitgraph)
- [Algorithmus](#algorithmus)
- [Fehlerbehandlung](#fehlerbehandlung)
- [Einschränkungen](#einschränkungen)
- [Einsatz in anderen Projekten](#einsatz-in-anderen-projekten)
- [Bekannte Befunde](#bekannte-befunde)
- [Projektstruktur](#projektstruktur)
- [Wissenschaftlicher Hintergrund](#wissenschaftlicher-hintergrund)
- [Mögliche Erweiterungen](#mögliche-erweiterungen)
- [Erwerb](#erwerb)

## Überblick über die Module

| Modul / Ziel        | Dateien                                 | Aufgabe                                                                                   | Garantie |
|---------------------|-----------------------------------------|-------------------------------------------------------------------------------------------|----------|
| `SubgraphAlgorithm` | `SubgraphAlgorithm.h/.cpp`              | Signaturen, zyklische Rotation, längste gemeinsame zusammenhängende Teilfolge, Vergleich zweier Graphen | Laufzeit O(n³), höchstens 63 Knoten |
| `MultiOmics`        | `MultiOmics.h/.cpp`                     | Schichtstapel, Integration (Vereinigung, Schnitt, Konsens), kohärente und unabhängige Mehrschicht-Relation, Strategien `BIGRAM` und `DYNAMIC` | höchstens 63 Knoten, Prüfung der Grenze |
| `ExactSubgraph`     | `ExactSubgraph.h/.cpp`                  | exakter Teilgraphtest auf `BitGraph`, Spezialfall k-Clique, Zeitlimit                      | immer korrekt, im ungünstigsten Fall exponentiell |
| `subgraph-cli`      | `Cli.cpp`                               | JSON auf der Standardeingabe, JSON auf der Standardausgabe (`compareGraphs`)               | –        |
| `subgraph`          | `SolverCli.cpp`                         | Dateischnittstelle für den Subgraph-SAT-Solver (`exact` oder `heuristic`)                  | Exit-Codes 0 bis 3 |

Wichtig für die Wahl des Moduls: `SubgraphAlgorithm::compareGraphs` und `ExactSubgraph::contains` beantworten **verschiedene Fragen**. Die erste berechnet die Relation des Signaturalgorithmus (Paarschnitt, siehe [Algorithmus](#algorithmus)), die zweite entscheidet, ob ein Mustergraph tatsächlich als Teilgraph in einem Wirtsgraphen enthalten ist. Für Anwendungen, die einen echten Teilgraphtest brauchen (etwa die SAT-Reduktion), ist ausschließlich `ExactSubgraph` korrekt.

## Voraussetzungen

- C++17-fähiger Compiler (getestet mit MinGW-w64/GCC und GCC 13 unter Linux; MSVC wird in der Build-Konfiguration berücksichtigt)
- CMake ab Version 3.10
- Internetzugang beim ersten Konfigurieren, da CMake die folgenden Abhängigkeiten per `FetchContent` herunterlädt:
  - nlohmann/json 3.11.2 (für das Kommandozeilenwerkzeug `subgraph-cli`)
  - Google Test 1.14.0 (für die Tests)

Die Bibliothek selbst (`SubgraphAlgorithm`, `MultiOmics`, `ExactSubgraph`) und der Adapter `subgraph` (`SolverCli.cpp`) benötigen keine externen Abhängigkeiten und lassen sich auch ohne CMake mit einem einzelnen Compileraufruf bauen (siehe [Build ohne CMake](#build-ohne-cmake)).

## Build

Unter Linux und macOS:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Unter Windows mit MinGW wird der Generator explizit angegeben, und das Verzeichnis `bin` der MinGW-Installation sowie das von CMake müssen im `PATH` liegen:

```powershell
$env:Path += ";<Pfad zu cmake>\bin"
$env:Path += ";<Pfad zu mingw64>\bin"

mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Der Build erzeugt vier Ziele:

| Ziel             | Art           | Beschreibung                                                  |
|------------------|---------------|---------------------------------------------------------------|
| `subgraphlib`    | Bibliothek    | `SubgraphAlgorithm`, `MultiOmics` und `ExactSubgraph` als statische Bibliothek |
| `subgraph-cli`   | Programm      | Kommandozeilenwerkzeug mit JSON-Ein-/Ausgabe                  |
| `subgraph`       | Programm      | Datei-Adapter für den Subgraph-SAT-Solver                     |
| `subgraph-tests` | Programm      | Testsuite (drei Testdateien in einem Programm)                |

Bei Verwendung von GCC oder Clang werden Bibliothek und Tests mit `--coverage` übersetzt, sodass Abdeckungsberichte mit `gcov` erzeugt werden können. Mit GCC und Clang gelten `-Wall -Wextra -Wpedantic`, mit MSVC `/W4 /permissive-`.

### Build ohne CMake

Für die Bibliothek und den Adapter genügt ein Compileraufruf (geprüft mit GCC unter Linux, ohne Warnungen bei `-Wall -Wextra -Wpedantic`):

```
g++ -std=c++17 -O2 SubgraphAlgorithm.cpp MultiOmics.cpp ExactSubgraph.cpp SolverCli.cpp -o subgraph
```

`subgraph-cli` (`Cli.cpp`) braucht zusätzlich den Header von nlohmann/json (`-I<Pfad>`), die Tests brauchen Google Test.

### Einbindung in Anwendungen, die die Bibliothek in eine gemeinsame Bibliothek linken

`subgraphlib` ist eine **statische** Bibliothek. Wird sie unter Linux in eine gemeinsame Bibliothek (`.so`) gelinkt, etwa für eine Python-Anbindung über `ctypes`, muss sie positionsunabhängig übersetzt sein. Die `CMakeLists.txt` setzt dies derzeit nicht selbst (siehe [Bekannte Befunde](#bekannte-befunde), B1). Als Abhilfe beim Konfigurieren:

```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
```

Unter Windows (MinGW, MSVC) ist das nicht nötig.

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

Die Testsuite umfasst **144 Testfälle**: 50 in `tests/TestSubgraphAlgorithm.cpp`, 87 in `tests/TestMultiOmics.cpp` und 7 in `tests/TestExactSubgraph.cpp`. Alle drei Dateien werden zum Programm `subgraph-tests` gelinkt.

### `SubgraphAlgorithm` (50 Testfälle in 9 Gruppen)

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

### `MultiOmics` (87 Testfälle in 13 Gruppen)

| Testgruppe                      | Anzahl | Gegenstand                                                   |
|---------------------------------|-------:|--------------------------------------------------------------|
| `LayerStackValidationTest`      | 12     | Gültige und ungültige Schichtstapel, Grenze bei 63 und 64 Knoten |
| `IntegrationTest`               | 12     | Vereinigung, Schnitt, k-aus-L-Konsens, Fehlerfälle, algebraische Gesetze (zufällig, fester Seed) |
| `ProjectionTest`                | 7      | Schichtauswahl, Reihenfolge, Duplikate, Index- und Stapelfehler |
| `TotalEdgesTest`                | 4      | Gesamtkantenzahl, Schleifen, Zufallsprüfung, Fehlerfälle     |
| `LayerRowComponentsTest`        | 6      | Zeilenkomponenten von Hand gerechnet, Abgleich mit `SubgraphAlgorithm`, 63-Bit-Grenze |
| `ContainsTest`                  | 12     | Paar im Host, zyklischer Umlauf, lineare Anfrage, Reihenfolge, Einzelknoten, 63 Knoten, Ausnahmen |
| `CoherenceTest`                 | 6      | Kohärenter gegen unabhängigen Modus, Gegenbeispiel, Implikation auf Zufallsstapeln |
| `StrategyEquivalenceTest`       | 2      | `BIGRAM` gegen `DYNAMIC` auf Zufallsstapeln bis 63 Knoten    |
| `SingleLayerConsistencyTest`    | 4      | Übereinstimmung mit `SubgraphAlgorithm::compareGraphs` (alle Matrizen bis 2 Knoten, Stichproben bis 63 Knoten) |
| `CompareLayeredTest`            | 11     | Alle sechs Ergebniswerte, Kantenzahl-Entscheidung, Spiegelung, Ausnahmen |
| `InvarianceTest`                | 4      | Schichtpermutation, doppelte Schichten, Projektionen, Integration |
| `MultiOmicsEdgeCasesTest`       | 5      | 63 Knoten, 64 Knoten, 2-Knoten-Graphen, Nullen und Einsen, viele Schichten |
| `ResultTextCompletionTest`      | 2      | Textdarstellung für `EQUAL_KEEP_A` und unbekannte Werte      |

### `ExactSubgraph` (7 Testfälle in 1 Gruppe)

| Testfall                      | Gegenstand                                                      |
|-------------------------------|-----------------------------------------------------------------|
| `EdgeInPath`                  | Fall, in dem die Signaturheuristik versagt: `K_2` im Pfad v0–v3–v1–v2, mit Prüfung der Abbildung |
| `TriangleInGraph`             | Dreieck im Graphen enthalten, `K_4` nicht                       |
| `PathNotInStar`               | Pfad über vier Knoten ist nicht im Stern enthalten              |
| `EmptyAndTrivialPatterns`     | leeres Muster, ein Knoten, drei isolierte Knoten; Muster mit mehr Knoten als der Wirt |
| `MoreThan64Nodes`             | 200 Knoten mit versteckter `K_5` (mehrere Bit-Wörter je Zeile), `K_6` nicht enthalten |
| `RandomAgainstBruteForce`     | 400 Zufallsfälle (fester Seed) gegen Brute-Force, mit Prüfung der Abbildung |
| `TimeoutIsReported`           | Zufallsgraph mit 400 Knoten, Suche nach `K_40` mit sehr kleinem Zeitlimit: entweder `Timeout` oder rechtzeitiges Ergebnis |

Die Zufallstests verwenden feste Seeds und sind damit reproduzierbar. Mit GCC ergibt `gcov` für `MultiOmics.cpp` alle Zeilen ausgeführt; nicht ausgeführt bleibt nur der durch die Struktur der Fallunterscheidung unerreichbare Zweig `aInB && !bInA` in der Prüfung auf wechselseitige Enthaltung sowie Ausnahmepfade, die `gcov` als zusätzliche Verzweigungen zählt.

Das Protokoll eines Durchlaufs liegt unter `tests/results.txt` (Kodierung UTF-16). Es stammt von einem Lauf mit 137 Testfällen in 22 Gruppen (ohne `ExactSubgraphTest`) und ist deshalb älter als der aktuelle Stand mit 144 Testfällen in 23 Gruppen.

## Verwendung als Bibliothek

### `SubgraphAlgorithm`

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

### `MultiOmics`

Ein Schichtstapel ist ein `std::vector` von Adjazenzmatrizen über derselben Knotenmenge. Alle Schichten der beiden verglichenen Stapel müssen in der Schichtzahl übereinstimmen.

```cpp
#include "MultiOmics.h"
#include <iostream>

int main() {
    using M = MultiOmics;
    M::Matrix kette3 = {{0,1,0},{0,0,1},{0,0,0}};
    M::Matrix kette4 = {{0,1,0,0},{0,0,1,0},{0,0,0,1},{0,0,0,0}};

    M::LayerStack klein = {kette3, kette3};   // zwei Schichten
    M::LayerStack gross = {kette4, kette4};

    // Ist klein in gross enthalten? (kohärent, schnelle Bigramm-Strategie)
    bool drin = M::contains(klein, gross, M::Mode::COHERENT, M::Strategy::BIGRAM);

    auto r = M::compareLayered(klein, gross);
    std::cout << SubgraphAlgorithm::resultToString(r) << "  contains=" << drin << std::endl;
    // KEEP_B  contains=1

    // Integration: Kante, wenn sie in mindestens einer Schicht vorkommt
    auto vereinigung = M::integrateUnion(gross);
    return 0;
}
```

### `ExactSubgraph`

```cpp
#include "ExactSubgraph.h"
#include <iostream>

int main() {
    BitGraph G(4), H(3);
    // G: Dreieck 0-1-2 mit Anhang 2-3
    G.addEdge(0, 1); G.addEdge(1, 2); G.addEdge(0, 2); G.addEdge(2, 3);
    // H: Dreieck
    H.addEdge(0, 1); H.addEdge(1, 2); H.addEdge(0, 2);

    std::vector<size_t> mapping;
    try {
        bool drin = ExactSubgraph::contains(G, H, &mapping, /*timeoutSec=*/10.0);
        std::cout << drin << " " << mapping.size() << std::endl;   // 1 3
    } catch (const ExactSubgraph::Timeout&) {
        std::cerr << "Zeitlimit überschritten" << std::endl;
    }
    return 0;
}
```

### Einbindung in ein eigenes CMake-Projekt

Zum Einbinden genügt es, das Verzeichnis als Unterprojekt aufzunehmen und gegen `subgraphlib` zu linken:

```cmake
add_subdirectory(csubgraph)
target_link_libraries(meine_anwendung PRIVATE subgraphlib)
```

## Kommandozeilenwerkzeug `subgraph-cli`

`subgraph-cli` liest ein JSON-Objekt von der Standardeingabe und schreibt das Ergebnis als JSON auf die Standardausgabe. Es ruft `SubgraphAlgorithm::compareGraphs` auf.

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

Die Codes 0 bis 5 sind an mehreren Stellen kodiert (hier, in der C-Schnittstelle der Anwendungen und in deren Python-Anbindung). Wer sie ändert, muss alle Stellen anpassen (siehe B5 unter [Bekannte Befunde](#bekannte-befunde)).

## Adapter für den Subgraph-SAT-Solver

Das Ziel `subgraph` (`SolverCli.cpp`) erfüllt die Dateischnittstelle des [Subgraph-SAT-Solvers](https://github.com/hjstephan86/subgraph-sat-solver):

```
subgraph <input.json> <output.json>
```

Eingabe: `{"G": {"nodes": [...], "edges": [[u,v], ...]}, "H": {...}}` mit ungerichteten Graphen. Ausgabe: `{"is_subgraph": true, "mapping": [...]}` oder `{"is_subgraph": false}`. `mapping` enthält an Position i den Index des Knotens in der Knotenliste von `G`, auf den der i-te Knoten von `H` abgebildet wird; der Solver leitet daraus die Belegung der Variablen ab. Die Eingabe wird in einem Durchgang gelesen (keine DOM-Struktur), da die Graphen der Reduktion viele Millionen Kanten haben können.

Beispiel (geprüft): Ein Dreieck `H` in einem Graphen `G` aus einem Dreieck mit Anhang.

```
$ cat in.json
{"G":{"nodes":[0,1,2,3],"edges":[[0,1],[1,2],[0,2],[2,3]]},
 "H":{"nodes":[0,1,2],"edges":[[0,1],[1,2],[0,2]]}}
$ ./subgraph in.json out.json ; echo $?
0
$ cat out.json
{"is_subgraph": true, "mapping": [0, 1, 2]}
```

Umgebungsvariablen:

| Variable | Werte | Bedeutung |
|----------|-------|-----------|
| `SUBGRAPH_MODE` | `exact` (Standard), `heuristic` | `exact`: exakter Test mit `ExactSubgraph`, keine Knotengrenze. `heuristic`: `SubgraphAlgorithm::compareGraphs`, höchstens 63 Knoten, kein Mapping |
| `SUBGRAPH_TIMEOUT_SEC` | Zahl, Standard 60 | Zeitlimit des exakten Tests, 0 = keines |

Exit-Codes:

| Code | Bedeutung |
|-----:|-----------|
| 0    | Erfolg (Ergebnis steht in `output.json`) |
| 1    | Fehler (Datei, JSON, ungültiger Graph) |
| 2    | mehr als 63 Knoten im Modus `heuristic` |
| 3    | Zeitlimit überschritten |

Der Modus `heuristic` ist **kein Orakel** für die SAT-Reduktion: `compareGraphs` entscheidet nicht, ob ein Teilgraph enthalten ist (siehe den Hinweis bei [ExactSubgraph](#api-referenz-exactsubgraph-und-bitgraph)). Er eignet sich nur zur Diagnose. Der Solver und das Experimente-Framework sollten deshalb den Modus `exact` verwenden und den Modus in den Ergebnissen mit aufschreiben.

Aufruf des Solvers:

```
SubgraphSATSolver formel.cnf --subgraph-binary /pfad/zu/subgraph
```

Mit dem Experiment-Framework:

```
python -m src.run_experiments --preset quick --solver-path "build/SubgraphSATSolver --subgraph-binary /pfad/zu/subgraph"
```

Der Solver wertet nach dem Aufruf den Exit-Code des Adapters und die Datei `output.json` aus. Die Auswertung der Antwort beruht in der aktuellen Fassung des Solvers auf einer Textsuche mit fester Schreibweise (`"is_subgraph": true`, mit Leerzeichen nach dem Doppelpunkt). Die Schreibweise der Ausgabe dieses Adapters ist deshalb Teil der Schnittstelle und darf nicht verändert werden.

## API-Referenz `SubgraphAlgorithm`

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

Der erste Summand kodiert den Spalteninhalt als Bitmuster, der zweite Summand die Spaltenposition. Löst `std::invalid_argument` aus, wenn die Matrix ungültig ist. **Die Knotengrenze von 63 wird hier nicht geprüft** (siehe [Einschränkungen](#einschränkungen)).

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

Liefert die Länge der längsten gemeinsamen zusammenhängenden Teilfolge (Longest Common **Substring**) zweier Folgen mittels dynamischer Programmierung. Im Gegensatz zur längsten gemeinsamen Teilsequenz (Subsequence) müssen die übereinstimmenden Elemente in beiden Folgen unmittelbar aufeinanderfolgen. Der Name `computeLCS` ist daher missverständlich (siehe B13).

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

## API-Referenz `MultiOmics`

`MultiOmics` verallgemeinert die Relation auf Schichtstapel. Ein Multi-Omics-Netzwerk besteht aus L ≥ 1 Schichten (zum Beispiel Transkriptom, Proteom, Metabolom), die alle über dieselbe, geordnete Knotenmenge der Größe n definiert sind. Jede Schicht ist eine binäre n × n-Adjazenzmatrix.

Typen:

| Typ / Konstante             | Bedeutung |
|-----------------------------|-----------|
| `Matrix`                    | `std::vector<std::vector<int>>` |
| `LayerStack`                | `std::vector<Matrix>` |
| `Result`                    | dieselbe Aufzählung wie `SubgraphAlgorithm::Result` |
| `kMaxNodes`                 | 63, größte unterstützte Knotenzahl |
| `Mode::COHERENT`            | alle Schichten müssen an derselben Rotation und Position übereinstimmen |
| `Mode::INDEPENDENT`         | jede Schicht darf eine eigene Rotation und Position verwenden |
| `Strategy::BIGRAM`          | gemeinsames benachbartes Paar, schnell (O(n log n)) |
| `Strategy::DYNAMIC`         | n Rotationen mit längster gemeinsamer zusammenhängender Teilfolge (Referenz) |
| `IntegrationOp`             | `UNION`, `INTERSECTION`, `CONSENSUS` |

Funktionen (alle statisch; bei ungültigen Eingaben wird `std::invalid_argument` ausgelöst):

| Funktion | Beschreibung |
|----------|--------------|
| `isValidLayerStack(stack)` | `true`, wenn L ≥ 1, jede Schicht eine gültige Adjazenzmatrix ist, alle Schichten dieselbe Größe n haben und n ≤ 63 gilt |
| `integrateUnion(stack)` | elementweises ODER aller Schichten |
| `integrateIntersection(stack)` | elementweises UND aller Schichten |
| `integrateConsensus(stack, k)` | Eintrag 1, wenn mindestens k Schichten die Kante enthalten; Fehler bei k = 0 oder k > L |
| `projectLayers(stack, indices)` | wählt die angegebenen Schichten in der angegebenen Reihenfolge aus; Fehler bei leerer Auswahl oder ungültigem Index |
| `totalEdges(stack)` | Gesamtzahl der Kanten über alle Schichten |
| `layerRowComponents(stack)` | Zeilenkomponenten je Schicht, `Ergebnis[l][j]` |
| `contains(a, b, mode, strategy)` | Ist A (Teilgraph) in B enthalten (A ⊑ B)? Für L = 1 identisch mit der Relation von `compareGraphs` |
| `compareLayered(a, b, mode, strategy)` | Vergleich zweier Stapel analog zu `compareGraphs`; bei wechselseitiger Enthaltung entscheidet die Gesamtkantenzahl |

Beide Strategien liefern nachweislich dieselben Ergebnisse (Satz zur Bigramm-Charakterisierung in der Arbeit, geprüft durch `StrategyEquivalenceTest`). Der kohärente Modus impliziert den unabhängigen (`CoherenceTest`). Für L = 1 stimmt das Ergebnis in beiden Modi und Strategien mit `SubgraphAlgorithm::compareGraphs` überein (`SingleLayerConsistencyTest`).

## API-Referenz `ExactSubgraph` und `BitGraph`

### `BitGraph`

Ungerichteter einfacher Graph als Bit-Adjazenzmatrix, ohne Knotengrenze. Schleifen werden ignoriert. Speicher: n · ⌈n/64⌉ · 8 Byte.

| Methode | Beschreibung |
|---------|--------------|
| `BitGraph(n)` | Graph mit n Knoten ohne Kanten |
| `size()`, `words()` | Knotenzahl, Zahl der 64-Bit-Wörter je Zeile |
| `addEdge(u, v)` | fügt die Kante {u, v} symmetrisch ein |
| `hasEdge(u, v)` | prüft die Kante |
| `row(u)` | Zeiger auf die Bitzeile des Knotens u |
| `degree(u)`, `edgeCount()` | Grad eines Knotens, Gesamtzahl der Kanten |

### `ExactSubgraph::contains`

```cpp
static bool contains(const BitGraph& host, const BitGraph& pattern,
                     std::vector<size_t>* mapping = nullptr, double timeoutSec = 0.0);
```

Entscheidet exakt, ob `pattern` (H) als (nicht notwendig induzierter) Teilgraph in `host` (G) enthalten ist (Subgraph-Monomorphismus). Das Ergebnis ist immer korrekt, die Laufzeit im schlechtesten Fall exponentiell (das Problem ist NP-vollständig). Bei Erfolg enthält `mapping` an Position i den Knoten von G, auf den der i-te Knoten von H abgebildet wird. Bei Überschreitung des Zeitlimits (`timeoutSec > 0`) wird `ExactSubgraph::Timeout` ausgelöst; 0 bedeutet kein Limit. Das Zeitlimit deckt derzeit die Vorverarbeitung (Kern, Färbung) nicht vollständig ab (B14).

- Ist `H` vollständig (`K_k`), wird eine k-Clique gesucht: k-Kern-Filter, einmalige Färbung von `G`, Schranke über die Zahl der nicht leeren Farbklassen und Verzweigung auf die kleinste Klasse. Bei m-partiten Graphen wie dem Clique-Graphen der 3-SAT-Reduktion entspricht das der Wahl der Klausel mit den wenigsten Kandidaten.
- Sonst: Backtracking mit Gradfilter und Bitset-Schnitt der Nachbarschaften.

**Hinweis zum Signaturverfahren:** `compareGraphs` entscheidet nicht, ob ein Teilgraph enthalten ist. Beispiel: Für den Pfad `v0–v3–v1–v2` und die Kante `K_2` liefert es `KEEP_BOTH`. Für die SAT-Reduktion ist daher `exact` der Standard.

## Algorithmus

`compareGraphs` arbeitet in folgenden Schritten:

1. Beide Matrizen werden validiert.
2. Für beide Graphen werden die Signaturen berechnet und auf ihre Zeilenkomponenten reduziert.
3. Haben beide Graphen gleiche Knotenzahl und identische Zeilenkomponenten, lautet das Ergebnis `IDENTICAL`.
4. Ist ein Graph nicht kleiner als der andere, wird die Folge der Zeilenkomponenten des größeren Graphen nacheinander um alle Positionen zyklisch rotiert und jeweils mit der Folge des kleineren Graphen verglichen. Gilt für mindestens eine Rotation, dass die längste gemeinsame zusammenhängende Teilfolge die Länge 2 oder mehr hat, gilt der kleinere Graph als enthalten.
5. Aus den beiden Enthaltenseinsrichtungen wird das Ergebnis abgeleitet (siehe Tabelle unter `compareGraphs`).

Die Verwendung zyklischer Rotationen anstelle vollständiger Permutationen reduziert die Zahl der Vergleiche von n! auf n.

### Was die Relation bedeutet: die Paarcharakterisierung

Die wissenschaftliche Arbeit (Verzeichnis `science/`) zeigt, dass die Relation des Algorithmus ein **Paarschnitt** ist: Ein Graph A gilt als in B enthalten, wenn die Folge der Zeilenkomponenten von A und eine Rotation der Folge von B ein gemeinsames benachbartes Paar teilen. Daraus folgen zwei praktische Konsequenzen:

- Die Relation lässt sich mit einem **invertierten Paarindex** ohne einzelne Vergleiche auswerten. Gen-DB nutzt das für die universelle Suche; nach den Messdateien von Gen-DB ist das rund das 80- bis 115-Fache gegenüber Einzelvergleichen in Python, bei gleichen Trefferlisten.
- Die Relation berechnet **keinen** Teilgraphtest. Dafür ist `ExactSubgraph` zuständig.

### Komplexität

| Größe       | Wert    | Begründung                                                  |
|-------------|---------|-------------------------------------------------------------|
| Zeit        | O(n³)   | n Rotationen, je ein Vergleich mit Aufwand O(n²)            |
| Speicher    | O(n²)   | Tabelle der dynamischen Programmierung                      |

Dabei bezeichnet n die Knotenzahl des größeren Graphen. Die Schranke gilt für den Signaturalgorithmus, **nicht** für `ExactSubgraph` (im ungünstigsten Fall exponentiell) und nicht für ganze Anwendungen, die ihn aufrufen. Beispielsweise wächst in der Reduktionskette des Subgraph-SAT-Solvers die Zeit für die gelösten ISCAS'85-Instanzen nach der Auswertung der Experimente etwa quadratisch in der Zahl der Literale (Exponent 2,08 ± 0,07). Dieser Wert gilt für die gesamte Pipeline aus Reduktion, Dateiein- und -ausgabe und externem Prozess und ist keine Aussage über die Schranke des Algorithmus.

## Fehlerbehandlung

Die Bibliothek löst `std::invalid_argument` aus bei:

- leeren Matrizen
- nicht quadratischen Matrizen
- Einträgen, die weder 0 noch 1 sind
- bei `MultiOmics` zusätzlich: ungleich großen Schichten, mehr als 63 Knoten, ungleicher Schichtzahl, ungültigem k oder ungültigem Schichtindex

`ExactSubgraph::contains` löst `ExactSubgraph::Timeout` (abgeleitet von `std::runtime_error`) aus, wenn das Zeitlimit überschritten wird.

```cpp
try {
    auto result = SubgraphAlgorithm::compareGraphs(emptyMatrix, graphB);
} catch (const std::invalid_argument& e) {
    std::cerr << "Fehler: " << e.what() << std::endl;
}
```

## Einschränkungen

- Die Signaturen werden als `uint64_t` gespeichert. Die Zeilenkomponenten belegen n Bit, daher ist `SubgraphAlgorithm` auf Graphen mit höchstens 63 Knoten ausgelegt. **`SubgraphAlgorithm` prüft diese Grenze derzeit nicht**; bei größeren Matrizen ist das Verhalten nicht definiert (die Verschiebung um 64 oder mehr Bit ist in C++ undefiniert, B6). `MultiOmics` prüft die Grenze (`kMaxNodes`) und löst `std::invalid_argument` aus; `ExactSubgraph` hat keine Knotengrenze.
- Eingaben müssen binäre Adjazenzmatrizen sein. Kantengewichte werden nicht unterstützt.
- Gerichtete Graphen werden von `SubgraphAlgorithm` und `MultiOmics` unterstützt, da die Matrix nicht symmetrisch sein muss. Schleifen (Einträge auf der Diagonalen) sind zulässig. `BitGraph` und `ExactSubgraph` arbeiten dagegen auf **ungerichteten** einfachen Graphen und ignorieren Schleifen.
- Das Kriterium einer Subgraph-Beziehung (längste gemeinsame zusammenhängende Teilfolge von mindestens 2) ist ein Merkmal des implementierten Verfahrens. Maßgeblich für seine Begründung ist die unten genannte Arbeit.
- Für identische 1×1-Graphen weicht die Python-Referenz von der C++-Fassung ab: Die Referenz liefert `keep_both` (B7).
- `subgraphlib` ist statisch und nicht positionsunabhängig übersetzt (B1, siehe [Build](#build)).

## Einsatz in anderen Projekten

`csubgraph` ist der gemeinsame Kern einer Kette von Projekten. Die folgende Übersicht zeigt, welche Teile von wem benutzt werden.

| Projekt | Rolle | Was von `csubgraph` benutzt wird | Art der Kopplung |
|---------|-------|----------------------------------|------------------|
| `subgraph` (Python) | Referenzimplementierung des Algorithmus | – (Vorbild, Differenzialtests gegen `csubgraph`) | Quelltext |
| `gen-db` | Biologie-Datenbank mit Netzwerksuche (FastAPI, PostgreSQL) | `SubgraphAlgorithm` und `MultiOmics` über C-Schnittstellen-Wrapper (`csub_compare` und weitere) und `ctypes`; Python-Paket `subgraph` als Rückfall | Bibliothek im Prozess |
| `subgraph-sat-solver` | SAT-Solver über die Kette CNF → 3-SAT → Clique → Subgraph | `ExactSubgraph` über den Adapter `subgraph` (nicht den Signaturalgorithmus) | Prozess mit Dateien |
| `subgraph-sat-solver-econ` | Variante des Solvers mit vorgebauter Bibliothek | Wrapper über die Bibliothek, geladen per `ctypes` | Bibliothek im Prozess |
| `subgraph-sat-solver-experiments` | Experimente-Framework, misst Laufzeiten des Solvers | indirekt, nur über die Außenfläche des Solvers | Prozess |

Eine ausführliche, geprüfte Darstellung aller achtzehn Wiederverwendungen, ihrer Kopplungsarten und ihrer Eigenschaften steht in der Arbeit im Verzeichnis `science/` (Katalog, Kapitel zu Gen-DB, Solver und Experimenten, Vergleich der Kopplungsarten).

### Anbindung an Gen-DB

Gen-DB baut beim ersten Gebrauch aus dem statischen Archiv (`libsubgraphlib.a`) und eigenen Wrapper-Quellen eine gemeinsame Bibliothek (Cache nach Hash aus Archiv, Quelle, Header und ABI-Version), lädt sie über `ctypes` und fällt auf das Python-Paket zurück, wenn die Bibliothek nicht gebaut oder geladen werden kann. Die Anbindung wird über die Umgebungsvariable `CSUBGRAPH_LIB_PATH` konfiguriert (Archiv oder Ordner). Damit der Wrapper unter Linux gelinkt werden kann, muss `subgraphlib` mit `-fPIC` übersetzt sein (siehe [Build](#build)).

Messwerte aus den eigenen Messdateien von Gen-DB (Windows 11, Python 3.14, vier logische CPUs, nicht neu gemessen): Der Gesamtlauf von sechs Anfragen mit zwei Workern war mit `csubgraph` um den Faktor 6,36 schneller als mit der Python-Fassung (185,6 s gegenüber 1180,3 s, bei gleichen Treffern). Je Einzelvergleich beträgt der Gewinn etwa das 37-Fache (0,074 ms gegenüber 2,69 ms); der Gesamtgewinn ist kleiner, weil danach der serielle Anteil der Suche (Datenbankabfrage, Prozesskommunikation) 66,7 % der Zeit ausmacht.

### Anbindung an den Subgraph-SAT-Solver

Der Solver erzeugt aus einer Formel einen Clique-Graphen `G` und den vollständigen Graphen `H = K_k`, schreibt beide nach `input.json`, ruft `subgraph input.json output.json` auf und leitet aus dem `mapping` eine erfüllende Belegung ab. Die Korrektheit der Kette und der Belegungsextraktion ist in der Arbeit bewiesen. Der Adapter verwendet im Standardmodus `exact` ausschließlich `ExactSubgraph`.

### Laden der Bibliothek in den Projekten

Gen-DB und `subgraph-sat-solver-econ` laden die Bibliothek auf verschiedene Weise (Bau beim ersten Gebrauch mit Rückfall gegenüber einer fertig gebauten Bibliothek ohne Rückfall). Die Arbeit vergleicht beide Verfahren und entwickelt daraus ein Konzept für ein einheitliches, betriebssystemunabhängiges Ladeverhalten (gemeinsame Bibliothek mit der Endung `.sl`, standardisierte Ladeschnittstelle). Das Konzept ist ein Entwurf und in diesem Repository **nicht umgesetzt**.

## Bekannte Befunde

Die Kompatibilitätsprüfung der Kette (Linux, GCC 13) hat Befunde ergeben, die dieses Repository betreffen. Die vollständige Liste mit Auswirkung und Verbesserung steht in der Arbeit (Kapitel „Befunde und Verbesserungen“).

| Nr. | Priorität | Befund | Verbesserung |
|-----|-----------|--------|--------------|
| B1  | hoch      | `CMakeLists.txt` setzt kein `-fPIC`; das Linken des Gen-DB-Wrappers scheitert unter Linux | `set(CMAKE_POSITION_INDEPENDENT_CODE ON)`; bis dahin beim Konfigurieren `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` |
| B5  | mittel    | Ergebniscodes 0 bis 5 sind an vier Stellen kodiert (`Cli.cpp`, zwei Wrapper, Python-Anbindung) | gemeinsamer Header, Konsistenztest |
| B6  | mittel    | `SubgraphAlgorithm` prüft die 63-Knoten-Grenze nicht; Verschiebung um ≥ 64 Bit ist undefiniert | Grenze in `calculateSignatures` prüfen, Ausnahme auslösen |
| B7  | mittel    | Python-Referenz liefert für identische 1×1-Graphen `keep_both` und weicht damit von C++ ab (127 von 20 000 Fällen) | Gleichheitsprüfung in der Python-Referenz ergänzen |
| B9  | mittel    | Modus `heuristic` des Adapters: 130 von 300 Antworten falsch, 5 Fehlerstatus | Fehler untersuchen, Modus nur als Diagnose kennzeichnen |
| B13 | niedrig   | `computeLCS` berechnet einen längsten gemeinsamen *Teilstring* | umbenennen (`longestCommonSubstring`) |
| B14 | niedrig   | Zeitlimit von `ExactSubgraph` deckt die Vorverarbeitung nicht ab | `tick()` auch in Kern und Färbung |

Weitere Befunde betreffen die Nachbarprojekte (Exit-Code 20 des Solvers ist mehrdeutig, das Zeitlimit des Solvers wird nicht an den Adapter weitergereicht, die Abhängigkeit von Gen-DB hängt auf `@main` und anderes).

Ergebnis der Prüfung, soweit es dieses Repository betrifft: Zwischen der Python-Referenz und `csubgraph` wurde in den geprüften Instanzen keine Abweichung gefunden, mit einer erklärbaren Ausnahme bei identischen 1×1-Graphen (B7).

## Projektstruktur

```
csubgraph/
├── CMakeLists.txt            Build-Konfiguration (Bibliothek, CLI, Adapter, Tests)
├── SubgraphAlgorithm.h/.cpp  Signaturalgorithmus
├── MultiOmics.h/.cpp         Schichtstapel, Integration, Mehrschicht-Relation
├── ExactSubgraph.h/.cpp      BitGraph und exakter Teilgraphtest
├── Cli.cpp                   subgraph-cli (JSON über Standardeingabe/-ausgabe)
├── SolverCli.cpp             subgraph (Datei-Adapter für den Subgraph-SAT-Solver)
├── tests/
│   ├── TestSubgraphAlgorithm.cpp   50 Testfälle
│   ├── TestMultiOmics.cpp          87 Testfälle
│   ├── TestExactSubgraph.cpp        7 Testfälle
│   └── results.txt                 Protokoll eines früheren Durchlaufs (UTF-16, 137 Tests)
├── science/                  wissenschaftliche Arbeit (LaTeX und PDF)
│   ├── csubgraph.tex         Hauptdatei (lädt p1, p2, p3)
│   ├── p1.tex                Teil II: Der Kern
│   ├── p2.tex                Teil III: Wiederverwendungen (Katalog, Gen-DB, Solver, Experimente, Laden)
│   ├── p3.tex                Teile IV und V: Prüfung, Befunde, Anleitung, Zusammenfassung, Ausblick
│   └── csubgraph.pdf         übersetzte Arbeit
├── LICENSE
└── README.md
```

## Wissenschaftlicher Hintergrund

Das Verfahren beruht auf folgenden Bausteinen:

- eindeutige Spaltensignaturen aus Bitmuster und Spaltenposition
- zyklische Rotationen zur Berücksichtigung der Ordnung
- Vergleich über die längste gemeinsame zusammenhängende Teilfolge mit Schwellwert 2

Die formale Herleitung einschließlich der Aussagen zu Korrektheit und Optimalität findet sich in:

- Epp, S. (2026): *Der Subgraph Algorithmus*, Datei `subgraph.tex` im Verzeichnis `science/` des Repositoriums https://github.com/naphets86/subgraph

Die Arbeit im Verzeichnis `science/` dieses Repositoriums (*Subgraph Algorithmus – Wiederverwendung*) behandelt die Wiederverwendung des Algorithmus:

- einen Katalog der achtzehn Wiederverwendungen und sieben Formen der Wiederverwendung,
- die **Paarcharakterisierung** der Relation und den daraus folgenden invertierten Paarindex,
- die Reduktionskette des Subgraph-SAT-Solvers (CNF → 3-SAT → Clique → Subgraph) mit Korrektheitsbeweis und Belegungsextraktion,
- ein UML-Sequenzdiagramm einer Suche in Gen-DB vom Browser bis zum Ergebnis,
- den Vergleich der Kopplungsarten (Bibliothek im Prozess, Prozess mit Dateien, Übernahme der Idee),
- den Vergleich der zwei Ladeverfahren von Gen-DB und `subgraph-sat-solver-econ` und das Konzept einer einheitlichen gemeinsamen Bibliothek (`.sl`) mit standardisiertem Ladeverfahren,
- die Auswertung der Experimente zum Subgraph-SAT-Solver (zwei Messreihen),
- die Kompatibilitätsprüfung mit vierzehn Befunden sowie Zusammenfassung und Ausblick.

Übersetzen der Arbeit (LaTeX mit `pdflatex`, deutsches `babel` und `lmodern`):

```
cd science
pdflatex csubgraph.tex
pdflatex csubgraph.tex
pdflatex csubgraph.tex
```

## Mögliche Erweiterungen

- Adjazenzlisten statt Matrizen für dünn besetzte Graphen
- Parallelisierung der Rotationsvergleiche
- Berücksichtigung von Kantengewichten in den Signaturen
- Näherungsverfahren für große Graphen
- Unterstützung von Graphen mit mehr als 63 Knoten durch breitere Signaturen (mehrere Maschinenwörter); `ExactSubgraph` und `BitGraph` haben diese Grenze nicht
- Positionsunabhängige Übersetzung in der `CMakeLists.txt` und ein gemeinsamer Header für die Ergebniscodes (B1, B5)
- Phasenzeiten und Speicherverbrauch des Adapters ausgeben, damit sich Reduktion, Teilgraphtest und Dateiein- und -ausgabe in Messungen trennen lassen
- Einheitliches Laden der Bibliothek über das Konzept der gemeinsamen Bibliothek (`.sl`), sobald ein Referenzlader existiert

## Erwerb

Der Preis für diese Software beträgt 3.145.000,00 EUR.

### Zahlungsinformationen

Name: Stephan Epp  
IBAN: DE24 5003 1900 0012 5603 20
BIC: BBVADEFFXXX
