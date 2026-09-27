#include "SubgraphAlgorithm.h"
#include <stdexcept>
#include <cmath>
#include <limits>

std::vector<uint64_t> SubgraphAlgorithm::calculateSignatures(const std::vector<std::vector<int>>& matrix) {
    if (!isValidAdjacencyMatrix(matrix)) {
        throw std::invalid_argument("Invalid adjacency matrix for signature calculation");
    }

    size_t n = matrix.size();
    std::vector<uint64_t> signatures;
    signatures.reserve(n);

    // Für jede Spalte j die Signatur berechnen
    for (size_t j = 0; j < n; ++j) {
        uint64_t signature = 0;

        // Zeilenkomponente: Σ(i=0 bis n-1) A_ij * 2^i
        for (size_t i = 0; i < n; ++i) {
            if (matrix[i][j] != 0) {
                signature += (static_cast<uint64_t>(1) << i);
            }
        }

        // Spaltengewichtung: j * 2^n
        uint64_t columnWeight = static_cast<uint64_t>(j) << static_cast<uint64_t>(n);
        signature += columnWeight;

        signatures.push_back(signature);
    }

    return signatures;
}

size_t SubgraphAlgorithm::computeLCS(const std::vector<uint64_t>& seqA, const std::vector<uint64_t>& seqB) {
    size_t m = seqA.size();
    size_t n = seqB.size();

    // DP-Tabelle für Longest Common SUBSTRING (nicht Subsequence!)
    // Wichtig: Bei Mismatch RESET auf 0, nicht max() wie bei LCS Subsequence!
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));
    size_t maxLength = 0;

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (seqA[i - 1] == seqB[j - 1]) {
                // Match: Verlängere die zusammenhängende Teilsequenz
                dp[i][j] = dp[i - 1][j - 1] + 1;
                maxLength = std::max(maxLength, dp[i][j]);
            } else {
                // MISMATCH: RESET auf 0 (nicht max!)
                // Dies macht es zum Longest Common SUBSTRING-Algorithmus
                dp[i][j] = 0;
            }
        }
    }

    return maxLength;
}

std::vector<uint64_t> SubgraphAlgorithm::rotateSequence(const std::vector<uint64_t>& seq, size_t rotation) {
    if (seq.empty()) {
        return seq;
    }

    size_t n = seq.size();
    rotation = rotation % n;

    std::vector<uint64_t> rotated;
    rotated.reserve(n);

    // FALSCH (nach links):
    // for (size_t i = 0; i < n; ++i) {
    //     rotated.push_back(seq[(i + rotation) % n]);
    // }

    // RICHTIG (nach rechts):
    for (size_t i = 0; i < n; ++i) {
        rotated.push_back(seq[(i + n - rotation) % n]);
    }

    return rotated;
}

std::vector<uint64_t> SubgraphAlgorithm::extractRowComponents(const std::vector<uint64_t>& signatures, size_t n) {
    std::vector<uint64_t> rowComponents;
    rowComponents.reserve(signatures.size());

    // Entferne die Spaltengewichtung (j * 2^n) um nur Zeilenkomponenten zu erhalten
    uint64_t columnWeightMask = ((static_cast<uint64_t>(1) << n) - 1);

    for (const auto& sig : signatures) {
        rowComponents.push_back(sig & columnWeightMask);
    }

    return rowComponents;
}

bool SubgraphAlgorithm::isValidAdjacencyMatrix(const std::vector<std::vector<int>>& matrix) {
    // Prüfe ob Matrix leer ist
    if (matrix.empty()) {
        return false;
    }

    size_t n = matrix.size();

    // Prüfe auf quadratische Form
    for (const auto& row : matrix) {
        if (row.size() != n) {
            return false;
        }
    }

    // Prüfe ob alle Einträge 0 oder 1 sind (binär)
    for (const auto& row : matrix) {
        for (int val : row) {
            if (val != 0 && val != 1) {
                return false;
            }
        }
    }

    return true;
}

std::string SubgraphAlgorithm::resultToString(Result result) {
    switch (result) {
        case Result::KEEP_A:
            return "KEEP_A (Graph A ist Subgraph von B oder hat gleiche Struktur)";
        case Result::KEEP_B:
            return "KEEP_B (Graph B ist Subgraph von A oder hat gleiche Struktur)";
        case Result::KEEP_BOTH:
            return "KEEP_BOTH (Keine Subgraph-Beziehung)";
        case Result::IDENTICAL:
            return "IDENTICAL (Graphen sind identisch)";
        default:
            return "UNKNOWN";
    }
}

SubgraphAlgorithm::Result SubgraphAlgorithm::compareGraphs(
    const std::vector<std::vector<int>>& graphA,
    const std::vector<std::vector<int>>& graphB) {

    // Validiere beide Graphen
    if (!isValidAdjacencyMatrix(graphA)) {
        throw std::invalid_argument("Graph A is not a valid adjacency matrix");
    }
    if (!isValidAdjacencyMatrix(graphB)) {
        throw std::invalid_argument("Graph B is not a valid adjacency matrix");
    }

    size_t nA = graphA.size();
    size_t nB = graphB.size();

    // Berechne Signaturen
    std::vector<uint64_t> signaturesA = calculateSignatures(graphA);
    std::vector<uint64_t> signaturesB = calculateSignatures(graphB);

    // Extrahiere Zeilenkomponenten
    std::vector<uint64_t> rowCompA = extractRowComponents(signaturesA, nA);
    std::vector<uint64_t> rowCompB = extractRowComponents(signaturesB, nB);

    // Prüfe ob Graphen identisch sind
    if (nA == nB && rowCompA == rowCompB) {
        return Result::IDENTICAL;
    }

    // Prüfe Subgraph-Beziehungen durch zyklische Rotationen
    bool isSubgraphAinB = false;
    bool isSubgraphBinA = false;

    // Prüfe ob A Subgraph von B ist (für jede Rotation von B)
    if (nB >= nA) {
        for (size_t rotation = 0; rotation < nB; ++rotation) {
            std::vector<uint64_t> rotatedB = rotateSequence(rowCompB, rotation);
            size_t lcs = computeLCS(rowCompA, rotatedB);
            if (lcs >= 2) {  // Schwelle: mindestens 2 übereinstimmende Signaturen
                isSubgraphAinB = true;
                break;
            }
        }
    }

    // Prüfe ob B Subgraph von A ist (für jede Rotation von A)
    if (nA >= nB) {
        for (size_t rotation = 0; rotation < nA; ++rotation) {
            std::vector<uint64_t> rotatedA = rotateSequence(rowCompA, rotation);
            size_t lcs = computeLCS(rotatedA, rowCompB);
            if (lcs >= 2) {  // Schwelle: mindestens 2 übereinstimmende Signaturen
                isSubgraphBinA = true;
                break;
            }
        }
    }

    // Bestimme Ergebnis basierend auf Subgraph-Beziehungen
    if (isSubgraphAinB && !isSubgraphBinA) {
        return Result::KEEP_B;  // B hat mehr Informationen
    } else if (isSubgraphBinA && !isSubgraphAinB) {
        return Result::KEEP_A;  // A hat mehr Informationen
    } else if (isSubgraphAinB && isSubgraphBinA) {
        // Beide sind Subgraphen voneinander → wähle nach Größe
        if (nA >= nB) {
            return Result::KEEP_A;
        } else {
            return Result::KEEP_B;
        }
    } else {
        return Result::KEEP_BOTH;  // Keine Subgraph-Beziehung
    }
}
