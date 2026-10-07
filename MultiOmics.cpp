#include "MultiOmics.h"

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using Seq = std::vector<uint64_t>;
using Mode = MultiOmics::Mode;
using Strategy = MultiOmics::Strategy;

/// Vorberechnete Zeilenkomponenten eines Schichtstapels
struct Prepared {
    std::size_t n;               ///< Knotenzahl
    std::vector<Seq> rows;       ///< rows[l][j] = Zeilenkomponente der Spalte j in Schicht l
};

void requireValid(const MultiOmics::LayerStack& stack, const char* name) {
    if (!MultiOmics::isValidLayerStack(stack)) {
        throw std::invalid_argument(std::string(name) + " is not a valid layer stack");
    }
}

Prepared prepare(const MultiOmics::LayerStack& stack) {
    Prepared p;
    p.n = stack.front().size();
    p.rows = MultiOmics::layerRowComponents(stack);
    return p;
}

/// Entscheidet, ob Folge a in Folge b "enthalten" ist (b mindestens so lang wie a,
/// gemeinsame zusammenhängende Teilfolge der Länge >= 2 mit einer Rotation von b).
bool containsSequence(const Seq& a, const Seq& b, Strategy strategy) {
    if (b.size() < a.size()) {
        return false;
    }
    if (strategy == Strategy::DYNAMIC) {
        for (std::size_t r = 0; r < b.size(); ++r) {
            if (SubgraphAlgorithm::computeLCS(a, SubgraphAlgorithm::rotateSequence(b, r)) >= 2) {
                return true;
            }
        }
        return false;
    }

    // Bigramm-Charakterisierung: Es genügt ein benachbartes Paar von a, das als
    // zyklisch benachbartes Paar in b vorkommt.
    if (a.size() < 2) {
        return false;
    }
    const std::size_t nb = b.size();
    std::set<std::pair<uint64_t, uint64_t>> cyclicPairs;
    for (std::size_t k = 0; k < nb; ++k) {
        cyclicPairs.insert({b[k], b[(k + 1) % nb]});
    }
    for (std::size_t i = 0; i + 1 < a.size(); ++i) {
        if (cyclicPairs.count({a[i], a[i + 1]}) != 0) {
            return true;
        }
    }
    return false;
}

/// Ordnet jedem Tupel (Zeilenkomponenten über alle Schichten) eine Zahl zu.
/// Die Zuordnung ist injektiv, daher ist Tupelgleichheit gleich Zahlengleichheit.
Seq encodeTuples(const Prepared& p, std::map<Seq, uint64_t>& dictionary) {
    Seq ids;
    ids.reserve(p.n);
    for (std::size_t j = 0; j < p.n; ++j) {
        Seq tuple;
        tuple.reserve(p.rows.size());
        for (const auto& layer : p.rows) {
            tuple.push_back(layer[j]);
        }
        const uint64_t next = static_cast<uint64_t>(dictionary.size());
        ids.push_back(dictionary.emplace(tuple, next).first->second);
    }
    return ids;
}

bool containsPrepared(const Prepared& a, const Prepared& b, Mode mode, Strategy strategy) {
    if (mode == Mode::INDEPENDENT) {
        for (std::size_t l = 0; l < a.rows.size(); ++l) {
            if (!containsSequence(a.rows[l], b.rows[l], strategy)) {
                return false;
            }
        }
        return true;
    }
    std::map<Seq, uint64_t> dictionary;
    const Seq idsA = encodeTuples(a, dictionary);
    const Seq idsB = encodeTuples(b, dictionary);
    return containsSequence(idsA, idsB, strategy);
}

void requireComparable(const MultiOmics::LayerStack& a, const MultiOmics::LayerStack& b) {
    requireValid(a, "Stack A");
    requireValid(b, "Stack B");
    if (a.size() != b.size()) {
        throw std::invalid_argument("Layer stacks must have the same number of layers");
    }
}

}  // namespace

bool MultiOmics::isValidLayerStack(const LayerStack& stack) {
    if (stack.empty()) {
        return false;
    }
    const std::size_t n = stack.front().size();
    if (n > kMaxNodes) {
        return false;
    }
    for (const auto& layer : stack) {
        if (!SubgraphAlgorithm::isValidAdjacencyMatrix(layer) || layer.size() != n) {
            return false;
        }
    }
    return true;
}

MultiOmics::Matrix MultiOmics::integrateConsensus(const LayerStack& stack, std::size_t k) {
    requireValid(stack, "Layer stack");
    if (k == 0 || k > stack.size()) {
        throw std::invalid_argument("Consensus threshold k must satisfy 1 <= k <= L");
    }
    const std::size_t n = stack.front().size();
    Matrix result(n, std::vector<int>(n, 0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            std::size_t count = 0;
            for (const auto& layer : stack) {
                count += static_cast<std::size_t>(layer[i][j]);
            }
            result[i][j] = count >= k ? 1 : 0;
        }
    }
    return result;
}

MultiOmics::Matrix MultiOmics::integrateUnion(const LayerStack& stack) {
    return integrateConsensus(stack, 1);
}

MultiOmics::Matrix MultiOmics::integrateIntersection(const LayerStack& stack) {
    return integrateConsensus(stack, stack.size());
}

MultiOmics::LayerStack MultiOmics::projectLayers(const LayerStack& stack,
                                                 const std::vector<std::size_t>& indices) {
    requireValid(stack, "Layer stack");
    if (indices.empty()) {
        throw std::invalid_argument("Layer selection must not be empty");
    }
    LayerStack selected;
    selected.reserve(indices.size());
    for (std::size_t index : indices) {
        if (index >= stack.size()) {
            throw std::invalid_argument("Layer index out of range");
        }
        selected.push_back(stack[index]);
    }
    return selected;
}

std::size_t MultiOmics::totalEdges(const LayerStack& stack) {
    requireValid(stack, "Layer stack");
    std::size_t edges = 0;
    for (const auto& layer : stack) {
        for (const auto& row : layer) {
            for (int value : row) {
                edges += static_cast<std::size_t>(value);
            }
        }
    }
    return edges;
}

std::vector<std::vector<uint64_t>> MultiOmics::layerRowComponents(const LayerStack& stack) {
    requireValid(stack, "Layer stack");
    const std::size_t n = stack.front().size();
    std::vector<std::vector<uint64_t>> result;
    result.reserve(stack.size());
    for (const auto& layer : stack) {
        result.push_back(SubgraphAlgorithm::extractRowComponents(
            SubgraphAlgorithm::calculateSignatures(layer), n));
    }
    return result;
}

bool MultiOmics::contains(const LayerStack& a, const LayerStack& b, Mode mode, Strategy strategy) {
    requireComparable(a, b);
    return containsPrepared(prepare(a), prepare(b), mode, strategy);
}

MultiOmics::Result MultiOmics::compareLayered(const LayerStack& a, const LayerStack& b,
                                              Mode mode, Strategy strategy) {
    requireComparable(a, b);
    const Prepared pa = prepare(a);
    const Prepared pb = prepare(b);

    if (pa.n == pb.n && pa.rows == pb.rows) {
        return Result::IDENTICAL;
    }

    const bool aInB = containsPrepared(pa, pb, mode, strategy);
    const bool bInA = containsPrepared(pb, pa, mode, strategy);

    if (aInB && !bInA) {
        return Result::KEEP_B;
    }
    if (bInA && !aInB) {
        return Result::KEEP_A;
    }
    if (aInB && bInA) {
        // Wechselseitige Enthaltung setzt nA <= nB und nB <= nA voraus, also nA == nB.
        return totalEdges(a) >= totalEdges(b) ? Result::EQUAL_KEEP_A : Result::EQUAL_KEEP_B;
    }
    return Result::KEEP_BOTH;
}
