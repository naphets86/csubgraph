#include <gtest/gtest.h>

#include "MultiOmics.h"
#include "SubgraphAlgorithm.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================================
// Hilfsmittel
// ============================================================================

namespace {

using Matrix = MultiOmics::Matrix;
using LayerStack = MultiOmics::LayerStack;
using Result = MultiOmics::Result;
using Mode = MultiOmics::Mode;
using Strategy = MultiOmics::Strategy;
using Op = MultiOmics::IntegrationOp;

constexpr std::size_t operator""_z(unsigned long long v) {
    return static_cast<std::size_t>(v);
}

/// Baut eine n x n-Matrix aus Spaltenwörtern: Bit i von words[j] ist Eintrag (i, j).
/// Die Zeilenkomponente der Spalte j ist dann genau words[j].
Matrix fromColumns(std::size_t n, const std::vector<uint64_t>& words) {
    Matrix m(n, std::vector<int>(n, 0));
    for (std::size_t j = 0; j < n; ++j) {
        for (std::size_t i = 0; i < n; ++i) {
            m[i][j] = static_cast<int>((words[j] >> i) & 1u);
        }
    }
    return m;
}

/// Alle n*n Bits einer Zahl als Matrix (für erschöpfende Tests, n <= 4).
Matrix fromBits(uint64_t bits, std::size_t n) {
    Matrix m(n, std::vector<int>(n, 0));
    for (std::size_t k = 0; k < n * n; ++k) {
        m[k / n][k % n] = static_cast<int>((bits >> k) & 1u);
    }
    return m;
}

Matrix randomMatrix(std::mt19937& rng, std::size_t n, double p) {
    std::bernoulli_distribution coin(p);
    Matrix m(n, std::vector<int>(n, 0));
    for (auto& row : m) {
        for (auto& v : row) {
            v = coin(rng) ? 1 : 0;
        }
    }
    return m;
}

LayerStack randomStack(std::mt19937& rng, std::size_t layers, std::size_t n, double p) {
    LayerStack s;
    for (std::size_t l = 0; l < layers; ++l) {
        s.push_back(randomMatrix(rng, n, p));
    }
    return s;
}

std::size_t countEdges(const Matrix& m) {
    std::size_t e = 0;
    for (const auto& row : m) {
        for (int v : row) {
            e += static_cast<std::size_t>(v);
        }
    }
    return e;
}

/// Prüft, dass f eine std::invalid_argument mit dem Textbestandteil `part` auslöst.
template <typename F>
bool throwsInvalidArgumentContaining(F&& f, const std::string& part) {
    try {
        f();
    } catch (const std::invalid_argument& e) {
        return std::string(e.what()).find(part) != std::string::npos;
    } catch (...) {
        return false;
    }
    return false;
}

const Mode kModes[] = {Mode::COHERENT, Mode::INDEPENDENT};
const Strategy kStrategies[] = {Strategy::BIGRAM, Strategy::DYNAMIC};

/// Übersetzt die bekannte Einzelschicht-Entscheidung der Bibliothek in "A enthalten in B".
bool libraryContains(const Matrix& a, const Matrix& b) {
    switch (SubgraphAlgorithm::compareGraphs(a, b)) {
        case SubgraphAlgorithm::Result::KEEP_B:
        case SubgraphAlgorithm::Result::EQUAL_KEEP_A:
        case SubgraphAlgorithm::Result::EQUAL_KEEP_B:
            return true;
        default:
            return false;  // IDENTICAL wird getrennt behandelt
    }
}

}  // namespace

// ============================================================================
// Gültigkeitsprüfung von Schichtstapeln
// ============================================================================

class LayerStackValidationTest : public ::testing::Test {};

TEST_F(LayerStackValidationTest, EmptyStackIsInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{}));
}

TEST_F(LayerStackValidationTest, SingleValidLayerIsValid) {
    EXPECT_TRUE(MultiOmics::isValidLayerStack(LayerStack{{{0, 1}, {1, 0}}}));
}

TEST_F(LayerStackValidationTest, SingleNodeStackIsValid) {
    EXPECT_TRUE(MultiOmics::isValidLayerStack(LayerStack{{{0}}, {{1}}}));
}

TEST_F(LayerStackValidationTest, SeveralValidLayersAreValid) {
    EXPECT_TRUE(MultiOmics::isValidLayerStack(
        LayerStack{{{0, 1}, {1, 0}}, {{0, 0}, {0, 0}}, {{1, 1}, {1, 1}}}));
}

TEST_F(LayerStackValidationTest, EmptyFirstLayerIsInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{Matrix{}}));
}

TEST_F(LayerStackValidationTest, EmptyLaterLayerIsInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, 1}, {1, 0}}, Matrix{}}));
}

TEST_F(LayerStackValidationTest, NonSquareLayerIsInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, 1}, {1, 0}, {0, 0}}}));
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, 1}, {1}}}));
}

TEST_F(LayerStackValidationTest, NonBinaryEntryIsInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, 2}, {1, 0}}}));
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, -1}, {1, 0}}}));
}

TEST_F(LayerStackValidationTest, InvalidSecondLayerMakesStackInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, 1}, {1, 0}}, {{0, 3}, {1, 0}}}));
}

TEST_F(LayerStackValidationTest, LayersOfDifferentSizeAreInvalid) {
    EXPECT_FALSE(MultiOmics::isValidLayerStack(LayerStack{{{0, 1}, {1, 0}}, {{0}}}));
    EXPECT_FALSE(MultiOmics::isValidLayerStack(
        LayerStack{{{0}}, {{0, 1}, {1, 0}}}));
}

TEST_F(LayerStackValidationTest, MaximumNodeCountIsAccepted) {
    EXPECT_EQ(MultiOmics::kMaxNodes, 63_z);
    LayerStack s{Matrix(63, std::vector<int>(63, 0)), Matrix(63, std::vector<int>(63, 1))};
    EXPECT_TRUE(MultiOmics::isValidLayerStack(s));
}

TEST_F(LayerStackValidationTest, TooManyNodesIsRejected) {
    LayerStack s{Matrix(64, std::vector<int>(64, 0))};
    EXPECT_FALSE(MultiOmics::isValidLayerStack(s));
    LayerStack big{Matrix(100, std::vector<int>(100, 0))};
    EXPECT_FALSE(MultiOmics::isValidLayerStack(big));
}

// ============================================================================
// Integrationsoperatoren
// ============================================================================

class IntegrationTest : public ::testing::Test {
protected:
    // Drei Schichten über 3 Knoten, von Hand gewählt
    LayerStack stack{
        {{0, 1, 0}, {0, 0, 1}, {0, 0, 0}},   // Schicht 0: Kanten 0->1, 1->2
        {{0, 1, 1}, {0, 0, 0}, {0, 0, 0}},   // Schicht 1: Kanten 0->1, 0->2
        {{0, 1, 0}, {0, 0, 1}, {1, 0, 0}}    // Schicht 2: Kanten 0->1, 1->2, 2->0
    };
};

TEST_F(IntegrationTest, UnionContainsEdgeOfAnyLayer) {
    Matrix expected{{0, 1, 1}, {0, 0, 1}, {1, 0, 0}};
    EXPECT_EQ(MultiOmics::integrateUnion(stack), expected);
}

TEST_F(IntegrationTest, IntersectionContainsOnlyEdgesOfAllLayers) {
    Matrix expected{{0, 1, 0}, {0, 0, 0}, {0, 0, 0}};
    EXPECT_EQ(MultiOmics::integrateIntersection(stack), expected);
}

TEST_F(IntegrationTest, ConsensusTwoOfThree) {
    // Kante 0->1: 3 Schichten, 1->2: 2 Schichten, 0->2: 1 Schicht, 2->0: 1 Schicht
    Matrix expected{{0, 1, 0}, {0, 0, 1}, {0, 0, 0}};
    EXPECT_EQ(MultiOmics::integrateConsensus(stack, 2), expected);
}

TEST_F(IntegrationTest, ConsensusOneEqualsUnionAndLEqualsIntersection) {
    EXPECT_EQ(MultiOmics::integrateConsensus(stack, 1), MultiOmics::integrateUnion(stack));
    EXPECT_EQ(MultiOmics::integrateConsensus(stack, 3), MultiOmics::integrateIntersection(stack));
}

TEST_F(IntegrationTest, ConsensusThresholdZeroThrows) {
    EXPECT_THROW(MultiOmics::integrateConsensus(stack, 0), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining(
        [&] { MultiOmics::integrateConsensus(stack, 0); }, "Consensus threshold"));
}

TEST_F(IntegrationTest, ConsensusThresholdAboveLayerCountThrows) {
    EXPECT_THROW(MultiOmics::integrateConsensus(stack, 4), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining(
        [&] { MultiOmics::integrateConsensus(stack, 4); }, "Consensus threshold"));
}

TEST_F(IntegrationTest, InvalidStackThrowsInEveryOperator) {
    LayerStack invalid{{{0, 2}, {1, 0}}};
    EXPECT_THROW(MultiOmics::integrateUnion(invalid), std::invalid_argument);
    EXPECT_THROW(MultiOmics::integrateIntersection(invalid), std::invalid_argument);
    EXPECT_THROW(MultiOmics::integrateConsensus(invalid, 1), std::invalid_argument);
    EXPECT_THROW(MultiOmics::integrateUnion(LayerStack{}), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining(
        [&] { MultiOmics::integrateUnion(invalid); }, "Layer stack"));
}

TEST_F(IntegrationTest, SingleLayerIsReturnedUnchangedByAllOperators) {
    LayerStack single{stack[2]};
    EXPECT_EQ(MultiOmics::integrateUnion(single), stack[2]);
    EXPECT_EQ(MultiOmics::integrateIntersection(single), stack[2]);
    EXPECT_EQ(MultiOmics::integrateConsensus(single, 1), stack[2]);
}

TEST_F(IntegrationTest, DuplicatedLayersDoNotChangeUnionOrIntersection) {
    LayerStack doubled{stack[0], stack[0], stack[1], stack[1]};
    LayerStack original{stack[0], stack[1]};
    EXPECT_EQ(MultiOmics::integrateUnion(doubled), MultiOmics::integrateUnion(original));
    EXPECT_EQ(MultiOmics::integrateIntersection(doubled), MultiOmics::integrateIntersection(original));
}

TEST_F(IntegrationTest, OperatorsAreIndependentOfLayerOrder) {
    LayerStack reversed(stack.rbegin(), stack.rend());
    EXPECT_EQ(MultiOmics::integrateUnion(reversed), MultiOmics::integrateUnion(stack));
    EXPECT_EQ(MultiOmics::integrateIntersection(reversed), MultiOmics::integrateIntersection(stack));
    for (std::size_t k = 1; k <= 3; ++k) {
        EXPECT_EQ(MultiOmics::integrateConsensus(reversed, k), MultiOmics::integrateConsensus(stack, k));
    }
}

TEST_F(IntegrationTest, AllOnesAndAllZerosLayers) {
    Matrix zeros(4, std::vector<int>(4, 0));
    Matrix ones(4, std::vector<int>(4, 1));
    LayerStack s{zeros, ones};
    EXPECT_EQ(MultiOmics::integrateUnion(s), ones);
    EXPECT_EQ(MultiOmics::integrateIntersection(s), zeros);
}

TEST_F(IntegrationTest, RandomisedAlgebraicLaws) {
    std::mt19937 rng(20260101);
    for (int round = 0; round < 200; ++round) {
        const std::size_t n = 1 + rng() % 9;
        const std::size_t layers = 1 + rng() % 5;
        LayerStack s = randomStack(rng, layers, n, 0.4);

        Matrix uni = MultiOmics::integrateUnion(s);
        Matrix inter = MultiOmics::integrateIntersection(s);

        Matrix previous = uni;  // Konsens mit k = 1
        for (std::size_t k = 1; k <= layers; ++k) {
            Matrix cons = MultiOmics::integrateConsensus(s, k);
            for (std::size_t i = 0; i < n; ++i) {
                for (std::size_t j = 0; j < n; ++j) {
                    // Monotonie: größeres k liefert eine Teilmenge der Kanten
                    EXPECT_LE(cons[i][j], previous[i][j]);
                    // direkte Definition
                    std::size_t count = 0;
                    for (const auto& layer : s) {
                        count += static_cast<std::size_t>(layer[i][j]);
                    }
                    EXPECT_EQ(cons[i][j], count >= k ? 1 : 0);
                }
            }
            previous = cons;
        }
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                EXPECT_LE(inter[i][j], uni[i][j]);
            }
        }
        if (layers == 2) {
            // Einschluss-Ausschluss für zwei Mengen: |A ∪ B| + |A ∩ B| = |A| + |B|
            EXPECT_EQ(countEdges(uni) + countEdges(inter), countEdges(s[0]) + countEdges(s[1]));
        }
    }
}

// ============================================================================
// Projektion, Kantenzahl, Zeilenkomponenten
// ============================================================================

class ProjectionTest : public ::testing::Test {
protected:
    LayerStack stack{
        {{0, 1}, {0, 0}},
        {{0, 0}, {1, 0}},
        {{1, 1}, {1, 1}}
    };
};

TEST_F(ProjectionTest, SelectsLayersInGivenOrder) {
    LayerStack selected = MultiOmics::projectLayers(stack, {2, 0});
    ASSERT_EQ(selected.size(), 2_z);
    EXPECT_EQ(selected[0], stack[2]);
    EXPECT_EQ(selected[1], stack[0]);
}

TEST_F(ProjectionTest, AllowsDuplicateIndicesAndSingleSelection) {
    LayerStack selected = MultiOmics::projectLayers(stack, {1, 1, 1});
    ASSERT_EQ(selected.size(), 3_z);
    EXPECT_EQ(selected[2], stack[1]);
    LayerStack one = MultiOmics::projectLayers(stack, {0});
    ASSERT_EQ(one.size(), 1_z);
    EXPECT_EQ(one[0], stack[0]);
}

TEST_F(ProjectionTest, FullIdentityProjectionReturnsEqualStack) {
    EXPECT_EQ(MultiOmics::projectLayers(stack, {0, 1, 2}), stack);
}

TEST_F(ProjectionTest, EmptySelectionThrows) {
    EXPECT_THROW(MultiOmics::projectLayers(stack, {}), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining(
        [&] { MultiOmics::projectLayers(stack, {}); }, "Layer selection must not be empty"));
}

TEST_F(ProjectionTest, OutOfRangeIndexThrows) {
    EXPECT_THROW(MultiOmics::projectLayers(stack, {3}), std::invalid_argument);
    EXPECT_THROW(MultiOmics::projectLayers(stack, {0, 99}), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining(
        [&] { MultiOmics::projectLayers(stack, {3}); }, "Layer index out of range"));
}

TEST_F(ProjectionTest, InvalidStackThrows) {
    EXPECT_THROW(MultiOmics::projectLayers(LayerStack{}, {0}), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining(
        [&] { MultiOmics::projectLayers(LayerStack{}, {0}); }, "Layer stack"));
}

TEST_F(ProjectionTest, ProjectionIsACopy) {
    LayerStack selected = MultiOmics::projectLayers(stack, {0});
    selected[0][0][0] = 1;
    EXPECT_EQ(stack[0][0][0], 0);
}

class TotalEdgesTest : public ::testing::Test {};

TEST_F(TotalEdgesTest, SumsEdgesOfAllLayers) {
    LayerStack s{{{0, 1}, {0, 0}}, {{1, 1}, {1, 1}}, {{0, 0}, {0, 0}}};
    EXPECT_EQ(MultiOmics::totalEdges(s), 5_z);
}

TEST_F(TotalEdgesTest, CountsLoops) {
    EXPECT_EQ(MultiOmics::totalEdges(LayerStack{{{1}}}), 1_z);
    EXPECT_EQ(MultiOmics::totalEdges(LayerStack{{{0}}}), 0_z);
}

TEST_F(TotalEdgesTest, EqualsSumOfPerLayerCountsOnRandomStacks) {
    std::mt19937 rng(7);
    for (int round = 0; round < 100; ++round) {
        LayerStack s = randomStack(rng, 1 + rng() % 4, 1 + rng() % 12, 0.35);
        std::size_t sum = 0;
        for (const auto& layer : s) {
            sum += countEdges(layer);
        }
        EXPECT_EQ(MultiOmics::totalEdges(s), sum);
    }
}

TEST_F(TotalEdgesTest, InvalidStackThrows) {
    EXPECT_THROW(MultiOmics::totalEdges(LayerStack{}), std::invalid_argument);
    EXPECT_THROW(MultiOmics::totalEdges(LayerStack{{{0, 5}, {0, 0}}}), std::invalid_argument);
}

class LayerRowComponentsTest : public ::testing::Test {};

TEST_F(LayerRowComponentsTest, MatchesHandComputedColumnWords) {
    // Spalte j, Bit i = Eintrag (i, j)
    LayerStack s{{{0, 1, 0}, {0, 0, 1}, {0, 0, 0}},   // Spalten: 0, 1, 2
                 {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}}};   // Spalten: 7, 4, 1
    auto rows = MultiOmics::layerRowComponents(s);
    ASSERT_EQ(rows.size(), 2_z);
    EXPECT_EQ(rows[0], (std::vector<uint64_t>{0, 1, 2}));
    EXPECT_EQ(rows[1], (std::vector<uint64_t>{7, 4, 1}));
}

TEST_F(LayerRowComponentsTest, EqualsSubgraphAlgorithmPerLayer) {
    std::mt19937 rng(11);
    for (int round = 0; round < 100; ++round) {
        const std::size_t n = 1 + rng() % 20;
        LayerStack s = randomStack(rng, 3, n, 0.3);
        auto rows = MultiOmics::layerRowComponents(s);
        ASSERT_EQ(rows.size(), 3_z);
        for (std::size_t l = 0; l < 3; ++l) {
            EXPECT_EQ(rows[l], SubgraphAlgorithm::extractRowComponents(
                                   SubgraphAlgorithm::calculateSignatures(s[l]), n));
        }
    }
}

TEST_F(LayerRowComponentsTest, FromColumnsRoundTrip) {
    const std::vector<uint64_t> words{5, 0, 3, 6};
    auto rows = MultiOmics::layerRowComponents(LayerStack{fromColumns(4, words)});
    EXPECT_EQ(rows[0], words);
}

TEST_F(LayerRowComponentsTest, FullMatrixAtMaximumSizeUses63Bits) {
    LayerStack s{Matrix(63, std::vector<int>(63, 1))};
    auto rows = MultiOmics::layerRowComponents(s);
    ASSERT_EQ(rows[0].size(), 63_z);
    const uint64_t expected = (static_cast<uint64_t>(1) << 63) - 1;
    for (uint64_t w : rows[0]) {
        EXPECT_EQ(w, expected);
    }
}

TEST_F(LayerRowComponentsTest, HighestRowBitAtMaximumSize) {
    Matrix m(63, std::vector<int>(63, 0));
    m[62][0] = 1;  // Bit 62 in Spalte 0
    auto rows = MultiOmics::layerRowComponents(LayerStack{m});
    EXPECT_EQ(rows[0][0], static_cast<uint64_t>(1) << 62);
    EXPECT_EQ(rows[0][1], 0u);
}

TEST_F(LayerRowComponentsTest, InvalidStackThrows) {
    EXPECT_THROW(MultiOmics::layerRowComponents(LayerStack{}), std::invalid_argument);
    EXPECT_THROW(MultiOmics::layerRowComponents(LayerStack{Matrix(64, std::vector<int>(64, 0))}),
                 std::invalid_argument);
}

// ============================================================================
// contains: handgerechnete Fälle
// ============================================================================

class ContainsTest : public ::testing::Test {
protected:
    // Anfrage: n = 2, Spaltenwörter (2, 1)
    LayerStack query{fromColumns(2, {2, 1})};
};

TEST_F(ContainsTest, FindsPairInsideHost) {
    LayerStack host{fromColumns(4, {0, 2, 1, 0})};  // Paar (2,1) an Position 1,2
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_TRUE(MultiOmics::contains(query, host, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, FindsPairAcrossCyclicWrapOfHost) {
    // Host n = 4, Paar (2,1) liegt zyklisch über das Ende: letzte Spalte = 2, erste Spalte = 1
    LayerStack host{fromColumns(4, {1, 0, 0, 2})};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_TRUE(MultiOmics::contains(query, host, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, QueryPairIsNotWrappedAround) {
    // Die Anfrage selbst wird linear gelesen: Das Paar (Letzte, Erste) = (1,2) der Anfrage
    // darf nicht als Paar gelten. Host enthält nur (1,2).
    LayerStack host{fromColumns(3, {1, 2, 0})};
    // zyklische Paare des Hosts: (1,2), (2,0), (0,1); lineares Paar der Anfrage: (2,1)
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_FALSE(MultiOmics::contains(query, host, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, OrderOfThePairMatters) {
    LayerStack host{fromColumns(3, {1, 2, 0})};  // enthält (1,2) aber nicht (2,1)
    LayerStack reversedQuery{fromColumns(2, {1, 2})};
    for (Strategy strategy : kStrategies) {
        EXPECT_TRUE(MultiOmics::contains(reversedQuery, host, Mode::COHERENT, strategy));
        EXPECT_FALSE(MultiOmics::contains(query, host, Mode::COHERENT, strategy));
    }
}

TEST_F(ContainsTest, SingleSharedColumnIsNotEnough) {
    LayerStack host{fromColumns(4, {2, 0, 0, 3})};  // Spalte 2 vorhanden, Nachfolger passt nicht
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_FALSE(MultiOmics::contains(query, host, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, HostColumnsWithHighRowBitsNeverMatch) {
    // Spaltenwort 2 + 4 hat Bit 2 gesetzt; die Anfrage (n = 2) besitzt nur Bits 0 und 1.
    LayerStack host{fromColumns(4, {6, 5, 0, 0})};
    for (Strategy strategy : kStrategies) {
        EXPECT_FALSE(MultiOmics::contains(query, host, Mode::COHERENT, strategy));
    }
}

TEST_F(ContainsTest, LargerQueryIsNeverContainedInSmallerHost) {
    LayerStack bigQuery{fromColumns(3, {2, 1, 0})};
    LayerStack smallHost{fromColumns(2, {2, 1})};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_FALSE(MultiOmics::contains(bigQuery, smallHost, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, SingleNodeQueryIsNeverContained) {
    LayerStack one{{{0}}};
    LayerStack host{fromColumns(3, {0, 0, 0})};
    LayerStack same{{{0}}};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_FALSE(MultiOmics::contains(one, host, mode, strategy));
            EXPECT_FALSE(MultiOmics::contains(one, same, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, AGraphWithAtLeastTwoNodesContainsItself) {
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_TRUE(MultiOmics::contains(query, query, mode, strategy));
        }
    }
}

TEST_F(ContainsTest, DefaultArgumentsAreCoherentAndBigram) {
    LayerStack host{fromColumns(4, {0, 2, 1, 0})};
    EXPECT_EQ(MultiOmics::contains(query, host),
              MultiOmics::contains(query, host, Mode::COHERENT, Strategy::BIGRAM));
    EXPECT_EQ(MultiOmics::compareLayered(query, host),
              MultiOmics::compareLayered(query, host, Mode::COHERENT, Strategy::BIGRAM));
}

TEST_F(ContainsTest, ExceptionsForInvalidOrIncompatibleStacks) {
    LayerStack invalid{{{0, 2}, {1, 0}}};
    LayerStack twoLayers{fromColumns(2, {2, 1}), fromColumns(2, {1, 2})};
    EXPECT_THROW(MultiOmics::contains(invalid, query), std::invalid_argument);
    EXPECT_THROW(MultiOmics::contains(query, invalid), std::invalid_argument);
    EXPECT_THROW(MultiOmics::contains(LayerStack{}, query), std::invalid_argument);
    EXPECT_THROW(MultiOmics::contains(query, twoLayers), std::invalid_argument);
    EXPECT_THROW(MultiOmics::contains(twoLayers, query), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::contains(invalid, query); }, "Stack A"));
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::contains(query, invalid); }, "Stack B"));
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::contains(invalid, invalid); }, "Stack A"));
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::contains(query, twoLayers); },
                                                "same number of layers"));
}

TEST_F(ContainsTest, WorksAtMaximumNodeCountWithCyclicWrap) {
    std::vector<uint64_t> words(63, 0);
    words[62] = 2;  // vorletztes Glied des Paares
    words[0] = 1;   // erstes Glied = Nachfolger über die Zyklusgrenze
    LayerStack host{fromColumns(63, words)};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_TRUE(MultiOmics::contains(query, host, mode, strategy));
            EXPECT_FALSE(MultiOmics::contains(host, query, mode, strategy));
        }
    }
}

// ============================================================================
// Kohärent gegen unabhängig
// ============================================================================

class CoherenceTest : public ::testing::Test {
protected:
    // Schicht 1 passt nur an Position 0 des Hosts, Schicht 2 nur an Position 1.
    LayerStack query{fromColumns(2, {2, 1}), fromColumns(2, {2, 0})};
    LayerStack host{fromColumns(3, {2, 1, 0}), fromColumns(3, {0, 2, 0})};
};

TEST_F(CoherenceTest, IndependentModeAcceptsDifferentPositionsPerLayer) {
    for (Strategy strategy : kStrategies) {
        EXPECT_TRUE(MultiOmics::contains(query, host, Mode::INDEPENDENT, strategy));
    }
}

TEST_F(CoherenceTest, CoherentModeRejectsDifferentPositionsPerLayer) {
    for (Strategy strategy : kStrategies) {
        EXPECT_FALSE(MultiOmics::contains(query, host, Mode::COHERENT, strategy));
    }
}

TEST_F(CoherenceTest, EachSingleLayerMatchesOnItsOwn) {
    for (std::size_t l = 0; l < 2; ++l) {
        for (Strategy strategy : kStrategies) {
            EXPECT_TRUE(MultiOmics::contains(LayerStack{query[l]}, LayerStack{host[l]},
                                             Mode::COHERENT, strategy));
        }
    }
}

TEST_F(CoherenceTest, CoherentMatchWhenBothLayersAgreeOnOnePosition) {
    LayerStack alignedHost{fromColumns(3, {2, 1, 0}), fromColumns(3, {2, 0, 0})};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_TRUE(MultiOmics::contains(query, alignedHost, mode, strategy));
        }
    }
}

TEST_F(CoherenceTest, AnUnmatchedLayerBlocksBothModes) {
    LayerStack hostWithoutSecond{fromColumns(3, {2, 1, 0}), fromColumns(3, {5, 5, 5})};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_FALSE(MultiOmics::contains(query, hostWithoutSecond, mode, strategy));
        }
    }
}

TEST_F(CoherenceTest, CoherentImpliesIndependentOnRandomStacks) {
    std::mt19937 rng(424242);
    int coherentTrue = 0;
    int strictGap = 0;
    for (int round = 0; round < 4000; ++round) {
        const std::size_t layers = 1 + rng() % 3;
        const std::size_t na = 2 + rng() % 3;
        const std::size_t nb = na + rng() % 4;
        LayerStack a = randomStack(rng, layers, na, 0.45);
        LayerStack b = randomStack(rng, layers, nb, 0.45);
        for (Strategy strategy : kStrategies) {
            const bool coherent = MultiOmics::contains(a, b, Mode::COHERENT, strategy);
            const bool independent = MultiOmics::contains(a, b, Mode::INDEPENDENT, strategy);
            if (coherent) {
                EXPECT_TRUE(independent);
            }
            coherentTrue += coherent ? 1 : 0;
            strictGap += (independent && !coherent) ? 1 : 0;
        }
    }
    // Die Stichprobe muss beide Fälle wirklich erreichen, sonst wäre der Test leer.
    EXPECT_GT(coherentTrue, 0);
    EXPECT_GT(strictGap, 0);
}

// ============================================================================
// Strategien liefern dasselbe
// ============================================================================

class StrategyEquivalenceTest : public ::testing::Test {};

TEST_F(StrategyEquivalenceTest, BigramAndDynamicAgreeOnRandomStacks) {
    std::mt19937 rng(987654321);
    int positives = 0;
    int negatives = 0;
    for (int round = 0; round < 6000; ++round) {
        const std::size_t layers = 1 + rng() % 3;
        const std::size_t na = 1 + rng() % 7;
        const std::size_t nb = 1 + rng() % 9;
        const double density = (round % 3 == 0) ? 0.15 : ((round % 3 == 1) ? 0.4 : 0.8);
        LayerStack a = randomStack(rng, layers, na, density);
        LayerStack b = randomStack(rng, layers, nb, density);
        for (Mode mode : kModes) {
            const bool fast = MultiOmics::contains(a, b, mode, Strategy::BIGRAM);
            const bool slow = MultiOmics::contains(a, b, mode, Strategy::DYNAMIC);
            EXPECT_EQ(fast, slow);
            EXPECT_EQ(MultiOmics::compareLayered(a, b, mode, Strategy::BIGRAM),
                      MultiOmics::compareLayered(a, b, mode, Strategy::DYNAMIC));
            positives += fast ? 1 : 0;
            negatives += fast ? 0 : 1;
        }
    }
    EXPECT_GT(positives, 100);
    EXPECT_GT(negatives, 100);
}

TEST_F(StrategyEquivalenceTest, BigramAndDynamicAgreeAtLargeSizes) {
    std::mt19937 rng(31337);
    for (int round = 0; round < 60; ++round) {
        const std::size_t na = 2 + rng() % 30;
        const std::size_t nb = na + rng() % 33;  // höchstens 63
        LayerStack a = randomStack(rng, 2, na, 0.05);
        LayerStack b = randomStack(rng, 2, nb, 0.05);
        for (Mode mode : kModes) {
            EXPECT_EQ(MultiOmics::contains(a, b, mode, Strategy::BIGRAM),
                      MultiOmics::contains(a, b, mode, Strategy::DYNAMIC));
        }
    }
}

// ============================================================================
// Einzelschicht: Übereinstimmung mit SubgraphAlgorithm
// ============================================================================

class SingleLayerConsistencyTest : public ::testing::Test {};

TEST_F(SingleLayerConsistencyTest, ExhaustiveForAllMatricesUpToTwoAndThreeNodes) {
    std::vector<std::vector<Matrix>> bySize(4);
    for (std::size_t n = 1; n <= 3; ++n) {
        for (uint64_t bits = 0; bits < (static_cast<uint64_t>(1) << (n * n)); ++bits) {
            bySize[n].push_back(fromBits(bits, n));
        }
    }
    ASSERT_EQ(bySize[1].size(), 2_z);
    ASSERT_EQ(bySize[2].size(), 16_z);
    ASSERT_EQ(bySize[3].size(), 512_z);

    std::size_t pairs = 0;
    for (std::size_t na = 1; na <= 3; ++na) {
        for (std::size_t nb = 1; nb <= 3; ++nb) {
            // Alle Paare, in denen mindestens ein Graph höchstens zwei Knoten hat;
            // bei 3 x 3 Knoten jedes 9. Paar (vollständig wäre >250000 Paare).
            const std::size_t stride = (na == 3 && nb == 3) ? 9 : 1;
            std::size_t counter = 0;
            for (const auto& a : bySize[na]) {
                for (const auto& b : bySize[nb]) {
                    if (counter++ % stride != 0) {
                        continue;
                    }
                    ++pairs;
                    const Result expected = SubgraphAlgorithm::compareGraphs(a, b);
                    for (Mode mode : kModes) {
                        for (Strategy strategy : kStrategies) {
                            EXPECT_EQ(MultiOmics::compareLayered(LayerStack{a}, LayerStack{b}, mode,
                                                                 strategy),
                                      expected);
                        }
                    }
                }
            }
        }
    }
    EXPECT_GT(pairs, 30000_z);
}

TEST_F(SingleLayerConsistencyTest, SampledFourNodeHosts) {
    std::mt19937 rng(5150);
    for (int round = 0; round < 3000; ++round) {
        const std::size_t na = 1 + rng() % 3;
        Matrix a = fromBits(rng() % (static_cast<uint64_t>(1) << (na * na)), na);
        Matrix b = fromBits(rng() % 65536u, 4);
        const Result expected = SubgraphAlgorithm::compareGraphs(a, b);
        EXPECT_EQ(MultiOmics::compareLayered(LayerStack{a}, LayerStack{b}), expected);
        EXPECT_EQ(MultiOmics::compareLayered(LayerStack{b}, LayerStack{a}),
                  SubgraphAlgorithm::compareGraphs(b, a));
    }
}

TEST_F(SingleLayerConsistencyTest, RandomMatricesUpToSixtyThreeNodes) {
    std::mt19937 rng(8086);
    for (int round = 0; round < 300; ++round) {
        const std::size_t na = 1 + rng() % 63;
        const std::size_t nb = 1 + rng() % 63;
        const double p = (round % 2 == 0) ? 0.03 : 0.3;
        Matrix a = randomMatrix(rng, na, p);
        Matrix b = randomMatrix(rng, nb, p);
        const Result expected = SubgraphAlgorithm::compareGraphs(a, b);
        EXPECT_EQ(MultiOmics::compareLayered(LayerStack{a}, LayerStack{b}), expected);
        EXPECT_EQ(MultiOmics::compareLayered(LayerStack{a}, LayerStack{b}, Mode::INDEPENDENT,
                                             Strategy::DYNAMIC),
                  expected);
    }
}

TEST_F(SingleLayerConsistencyTest, ContainsAgreesWithLibraryRelationOnPlantedMatches) {
    // Gleiche Spaltenwörter an verschiedenen Stellen: viele echte Treffer
    std::mt19937 rng(99);
    int hits = 0;
    for (int round = 0; round < 500; ++round) {
        const std::size_t nb = 3 + rng() % 8;
        std::vector<uint64_t> hostWords(nb);
        for (auto& w : hostWords) {
            w = rng() % 8u;  // nur drei Zeilenbits: viele Kollisionen
        }
        const std::size_t na = 2 + rng() % 2;
        std::vector<uint64_t> queryWords(na);
        for (auto& w : queryWords) {
            w = rng() % (static_cast<uint64_t>(1) << na);
        }
        Matrix a = fromColumns(na, queryWords);
        Matrix b = fromColumns(nb, hostWords);
        const bool viaLibrary = libraryContains(a, b);
        const bool viaOmics = MultiOmics::contains(LayerStack{a}, LayerStack{b});
        hits += viaOmics ? 1 : 0;
        EXPECT_EQ(viaOmics, viaLibrary);
    }
    EXPECT_GT(hits, 50);
}

// ============================================================================
// compareLayered: alle Ergebnisarten
// ============================================================================

class CompareLayeredTest : public ::testing::Test {
protected:
    // Gemeinsame Schicht 1; Schicht 2 unterscheidet sich nur in der letzten Spalte.
    LayerStack stackWithMoreEdges{fromColumns(3, {2, 4, 1}), fromColumns(3, {0, 0, 7})};
    LayerStack stackWithFewerEdges{fromColumns(3, {2, 4, 1}), fromColumns(3, {0, 0, 0})};
};

TEST_F(CompareLayeredTest, IdenticalStacks) {
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_EQ(MultiOmics::compareLayered(stackWithMoreEdges, stackWithMoreEdges, mode, strategy),
                      Result::IDENTICAL);
        }
    }
}

TEST_F(CompareLayeredTest, DifferenceInSecondLayerIsNotIdentical) {
    for (Mode mode : kModes) {
        EXPECT_NE(MultiOmics::compareLayered(stackWithMoreEdges, stackWithFewerEdges, mode),
                  Result::IDENTICAL);
    }
}

TEST_F(CompareLayeredTest, MutualContainmentKeepsStackWithMoreTotalEdges) {
    ASSERT_EQ(MultiOmics::totalEdges(stackWithMoreEdges), 6_z);
    ASSERT_EQ(MultiOmics::totalEdges(stackWithFewerEdges), 3_z);
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_EQ(MultiOmics::compareLayered(stackWithMoreEdges, stackWithFewerEdges, mode, strategy),
                      Result::EQUAL_KEEP_A);
            EXPECT_EQ(MultiOmics::compareLayered(stackWithFewerEdges, stackWithMoreEdges, mode, strategy),
                      Result::EQUAL_KEEP_B);
        }
    }
}

TEST_F(CompareLayeredTest, TotalEdgesTieKeepsFirstStack) {
    LayerStack a{fromColumns(3, {2, 4, 1}), fromColumns(3, {0, 0, 1})};
    LayerStack b{fromColumns(3, {2, 4, 1}), fromColumns(3, {0, 0, 2})};
    ASSERT_EQ(MultiOmics::totalEdges(a), MultiOmics::totalEdges(b));
    for (Mode mode : kModes) {
        EXPECT_EQ(MultiOmics::compareLayered(a, b, mode), Result::EQUAL_KEEP_A);
        EXPECT_EQ(MultiOmics::compareLayered(b, a, mode), Result::EQUAL_KEEP_A);
    }
}

TEST_F(CompareLayeredTest, SmallerQueryInLargerHostKeepsHost) {
    LayerStack small{fromColumns(2, {2, 1})};
    LayerStack large{fromColumns(4, {0, 2, 1, 0})};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_EQ(MultiOmics::compareLayered(small, large, mode, strategy), Result::KEEP_B);
            EXPECT_EQ(MultiOmics::compareLayered(large, small, mode, strategy), Result::KEEP_A);
        }
    }
}

TEST_F(CompareLayeredTest, NoSharedPairKeepsBoth) {
    LayerStack a{fromColumns(3, {1, 2, 4})};
    LayerStack b{fromColumns(3, {4, 2, 1})};
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_EQ(MultiOmics::compareLayered(a, b, mode, strategy), Result::KEEP_BOTH);
        }
    }
}

TEST_F(CompareLayeredTest, SingleNodeStacks) {
    LayerStack zero{{{0}}, {{0}}};
    LayerStack one{{{1}}, {{0}}};
    for (Mode mode : kModes) {
        EXPECT_EQ(MultiOmics::compareLayered(zero, zero, mode), Result::IDENTICAL);
        EXPECT_EQ(MultiOmics::compareLayered(zero, one, mode), Result::KEEP_BOTH);
    }
}

TEST_F(CompareLayeredTest, ModesDifferOnTheCoherenceExample) {
    LayerStack query{fromColumns(2, {2, 1}), fromColumns(2, {2, 0})};
    LayerStack host{fromColumns(3, {2, 1, 0}), fromColumns(3, {0, 2, 0})};
    EXPECT_EQ(MultiOmics::compareLayered(query, host, Mode::INDEPENDENT), Result::KEEP_B);
    EXPECT_EQ(MultiOmics::compareLayered(query, host, Mode::COHERENT), Result::KEEP_BOTH);
    EXPECT_EQ(MultiOmics::compareLayered(host, query, Mode::INDEPENDENT), Result::KEEP_A);
    EXPECT_EQ(MultiOmics::compareLayered(host, query, Mode::COHERENT), Result::KEEP_BOTH);
}

TEST_F(CompareLayeredTest, SwappingArgumentsMirrorsTheResult) {
    std::mt19937 rng(1234);
    for (int round = 0; round < 2000; ++round) {
        const std::size_t layers = 1 + rng() % 3;
        LayerStack a = randomStack(rng, layers, 1 + rng() % 6, 0.4);
        LayerStack b = randomStack(rng, layers, 1 + rng() % 6, 0.4);
        for (Mode mode : kModes) {
            const Result ab = MultiOmics::compareLayered(a, b, mode);
            const Result ba = MultiOmics::compareLayered(b, a, mode);
            const bool tie = MultiOmics::totalEdges(a) == MultiOmics::totalEdges(b);
            switch (ab) {
                case Result::IDENTICAL:
                    EXPECT_EQ(ba, Result::IDENTICAL);
                    break;
                case Result::KEEP_BOTH:
                    EXPECT_EQ(ba, Result::KEEP_BOTH);
                    break;
                case Result::KEEP_A:
                    EXPECT_EQ(ba, Result::KEEP_B);
                    break;
                case Result::KEEP_B:
                    EXPECT_EQ(ba, Result::KEEP_A);
                    break;
                case Result::EQUAL_KEEP_A:
                    EXPECT_EQ(ba, tie ? Result::EQUAL_KEEP_A : Result::EQUAL_KEEP_B);
                    break;
                case Result::EQUAL_KEEP_B:
                    EXPECT_EQ(ba, Result::EQUAL_KEEP_A);
                    break;
            }
        }
    }
}

TEST_F(CompareLayeredTest, SizeDifferenceNeverProducesEqualVerdicts) {
    std::mt19937 rng(777);
    for (int round = 0; round < 1500; ++round) {
        const std::size_t na = 1 + rng() % 5;
        const std::size_t nb = na + 1 + rng() % 4;
        LayerStack a = randomStack(rng, 2, na, 0.5);
        LayerStack b = randomStack(rng, 2, nb, 0.5);
        for (Mode mode : kModes) {
            const Result r = MultiOmics::compareLayered(a, b, mode);
            EXPECT_NE(r, Result::IDENTICAL);
            EXPECT_NE(r, Result::EQUAL_KEEP_A);
            EXPECT_NE(r, Result::EQUAL_KEEP_B);
            EXPECT_NE(r, Result::KEEP_A);  // der kleinere Stapel kann den größeren nicht enthalten
        }
    }
}

TEST_F(CompareLayeredTest, ExceptionsForInvalidOrIncompatibleStacks) {
    LayerStack ok{fromColumns(2, {2, 1})};
    LayerStack invalid{{{0, 2}, {1, 0}}};
    LayerStack two{fromColumns(2, {2, 1}), fromColumns(2, {2, 1})};
    EXPECT_THROW(MultiOmics::compareLayered(invalid, ok), std::invalid_argument);
    EXPECT_THROW(MultiOmics::compareLayered(ok, invalid), std::invalid_argument);
    EXPECT_THROW(MultiOmics::compareLayered(ok, LayerStack{}), std::invalid_argument);
    EXPECT_THROW(MultiOmics::compareLayered(ok, two), std::invalid_argument);
    EXPECT_THROW(MultiOmics::compareLayered(two, ok), std::invalid_argument);
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::compareLayered(invalid, ok); }, "Stack A"));
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::compareLayered(ok, invalid); }, "Stack B"));
    EXPECT_TRUE(throwsInvalidArgumentContaining([&] { MultiOmics::compareLayered(ok, two); },
                                                "same number of layers"));
}

// ============================================================================
// Strukturelle Invarianzen der Mehrschicht-Relation
// ============================================================================

class InvarianceTest : public ::testing::Test {};

TEST_F(InvarianceTest, SimultaneousLayerPermutationDoesNotChangeTheResult) {
    std::mt19937 rng(2468);
    for (int round = 0; round < 1500; ++round) {
        const std::size_t layers = 2 + rng() % 3;
        LayerStack a = randomStack(rng, layers, 2 + rng() % 4, 0.45);
        LayerStack b = randomStack(rng, layers, 2 + rng() % 5, 0.45);
        std::vector<std::size_t> order(layers);
        for (std::size_t l = 0; l < layers; ++l) {
            order[l] = l;
        }
        std::shuffle(order.begin(), order.end(), rng);
        LayerStack pa = MultiOmics::projectLayers(a, order);
        LayerStack pb = MultiOmics::projectLayers(b, order);
        for (Mode mode : kModes) {
            EXPECT_EQ(MultiOmics::compareLayered(a, b, mode), MultiOmics::compareLayered(pa, pb, mode));
            EXPECT_EQ(MultiOmics::contains(a, b, mode), MultiOmics::contains(pa, pb, mode));
        }
    }
}

TEST_F(InvarianceTest, DuplicatingEveryLayerDoesNotChangeTheResult) {
    std::mt19937 rng(1357);
    for (int round = 0; round < 1500; ++round) {
        const std::size_t layers = 1 + rng() % 3;
        LayerStack a = randomStack(rng, layers, 1 + rng() % 5, 0.45);
        LayerStack b = randomStack(rng, layers, 1 + rng() % 6, 0.45);
        LayerStack da = a;
        LayerStack db = b;
        da.insert(da.end(), a.begin(), a.end());
        db.insert(db.end(), b.begin(), b.end());
        for (Mode mode : kModes) {
            EXPECT_EQ(MultiOmics::contains(a, b, mode), MultiOmics::contains(da, db, mode));
            EXPECT_EQ(MultiOmics::compareLayered(a, b, mode), MultiOmics::compareLayered(da, db, mode));
        }
    }
}

TEST_F(InvarianceTest, CoherentContainmentOfStackImpliesContainmentOfEveryProjection) {
    // Wenn alle Schichten an derselben Stelle passen, passt jede Teilauswahl von Schichten.
    // Jede zweite Runde pflanzt die Spalten der Anfrage gemeinsam in alle Schichten des Hosts.
    std::mt19937 rng(8642);
    int checked = 0;
    for (int round = 0; round < 4000; ++round) {
        const std::size_t na = 2 + rng() % 2;
        const std::size_t nb = 4 + rng() % 4;
        LayerStack a = randomStack(rng, 3, na, 0.5);
        LayerStack b = randomStack(rng, 3, nb, 0.5);
        if (round % 2 == 0) {
            const std::size_t start = rng() % nb;
            for (std::size_t l = 0; l < 3; ++l) {
                const auto words = MultiOmics::layerRowComponents(LayerStack{a[l]})[0];
                std::vector<uint64_t> hostWords = MultiOmics::layerRowComponents(LayerStack{b[l]})[0];
                hostWords[start] = words[0];
                hostWords[(start + 1) % nb] = words[1];
                b[l] = fromColumns(nb, hostWords);
            }
        }
        if (!MultiOmics::contains(a, b, Mode::COHERENT)) {
            continue;
        }
        ++checked;
        for (std::size_t drop = 0; drop < 3; ++drop) {
            std::vector<std::size_t> keep;
            for (std::size_t l = 0; l < 3; ++l) {
                if (l != drop) {
                    keep.push_back(l);
                }
            }
            EXPECT_TRUE(MultiOmics::contains(MultiOmics::projectLayers(a, keep),
                                             MultiOmics::projectLayers(b, keep), Mode::COHERENT));
        }
    }
    EXPECT_GT(checked, 1000);
}

TEST_F(InvarianceTest, IntegrationThenComparisonUsesTheIntegratedGraph) {
    // Einzelschicht-Vergleich der integrierten Graphen entspricht dem Vergleich der Einzelschichtstapel.
    std::mt19937 rng(97531);
    for (int round = 0; round < 500; ++round) {
        LayerStack a = randomStack(rng, 3, 2 + rng() % 4, 0.3);
        LayerStack b = randomStack(rng, 3, 2 + rng() % 5, 0.3);
        for (Op op : {Op::UNION, Op::INTERSECTION, Op::CONSENSUS}) {
            Matrix ia;
            Matrix ib;
            switch (op) {
                case Op::UNION:
                    ia = MultiOmics::integrateUnion(a);
                    ib = MultiOmics::integrateUnion(b);
                    break;
                case Op::INTERSECTION:
                    ia = MultiOmics::integrateIntersection(a);
                    ib = MultiOmics::integrateIntersection(b);
                    break;
                case Op::CONSENSUS:
                    ia = MultiOmics::integrateConsensus(a, 2);
                    ib = MultiOmics::integrateConsensus(b, 2);
                    break;
            }
            EXPECT_EQ(MultiOmics::compareLayered(LayerStack{ia}, LayerStack{ib}),
                      SubgraphAlgorithm::compareGraphs(ia, ib));
        }
    }
}

// ============================================================================
// Randfälle: kleinste und größte Knotenzahl
// ============================================================================

class MultiOmicsEdgeCasesTest : public ::testing::Test {};

TEST_F(MultiOmicsEdgeCasesTest, SixtyThreeNodeStacksCompareIdenticalToThemselves) {
    std::mt19937 rng(63);
    LayerStack s = randomStack(rng, 3, 63, 0.2);
    for (Mode mode : kModes) {
        for (Strategy strategy : kStrategies) {
            EXPECT_EQ(MultiOmics::compareLayered(s, s, mode, strategy), Result::IDENTICAL);
            EXPECT_TRUE(MultiOmics::contains(s, s, mode, strategy));
        }
    }
}

TEST_F(MultiOmicsEdgeCasesTest, SixtyFourNodesAreRejectedEverywhere) {
    LayerStack big{Matrix(64, std::vector<int>(64, 0))};
    LayerStack ok{fromColumns(2, {2, 1})};
    EXPECT_THROW(MultiOmics::contains(big, ok), std::invalid_argument);
    EXPECT_THROW(MultiOmics::contains(ok, big), std::invalid_argument);
    EXPECT_THROW(MultiOmics::compareLayered(big, ok), std::invalid_argument);
    EXPECT_THROW(MultiOmics::totalEdges(big), std::invalid_argument);
    EXPECT_THROW(MultiOmics::integrateUnion(big), std::invalid_argument);
}

TEST_F(MultiOmicsEdgeCasesTest, TwoNodeStacksCoverAllSixteenGraphs) {
    for (uint64_t x = 0; x < 16; ++x) {
        for (uint64_t y = 0; y < 16; ++y) {
            LayerStack a{fromBits(x, 2)};
            LayerStack b{fromBits(y, 2)};
            const Result r = MultiOmics::compareLayered(a, b);
            EXPECT_EQ(r, SubgraphAlgorithm::compareGraphs(a[0], b[0]));
            if (x == y) {
                EXPECT_EQ(r, Result::IDENTICAL);
            }
        }
    }
}

TEST_F(MultiOmicsEdgeCasesTest, AllZeroAndAllOneGraphs) {
    LayerStack zeros{Matrix(5, std::vector<int>(5, 0)), Matrix(5, std::vector<int>(5, 0))};
    LayerStack ones{Matrix(5, std::vector<int>(5, 1)), Matrix(5, std::vector<int>(5, 1))};
    for (Mode mode : kModes) {
        EXPECT_EQ(MultiOmics::compareLayered(zeros, ones, mode), Result::KEEP_BOTH);
        EXPECT_EQ(MultiOmics::compareLayered(zeros, zeros, mode), Result::IDENTICAL);
    }
}

TEST_F(MultiOmicsEdgeCasesTest, ManyLayersAreSupported) {
    std::mt19937 rng(40);
    LayerStack a = randomStack(rng, 25, 4, 0.4);
    LayerStack b = randomStack(rng, 25, 6, 0.4);
    for (Mode mode : kModes) {
        EXPECT_EQ(MultiOmics::compareLayered(a, b, mode, Strategy::BIGRAM),
                  MultiOmics::compareLayered(a, b, mode, Strategy::DYNAMIC));
    }
    EXPECT_EQ(MultiOmics::integrateConsensus(a, 25), MultiOmics::integrateIntersection(a));
}

// ============================================================================
// Ergänzung: Textdarstellung der Ergebniswerte, die Mehrschicht-Vergleiche liefern
// ============================================================================

class ResultTextCompletionTest : public ::testing::Test {};

TEST_F(ResultTextCompletionTest, EveryResultProducedByCompareLayeredHasANamedText) {
    const Result all[] = {Result::KEEP_A,        Result::KEEP_B,       Result::KEEP_BOTH,
                          Result::IDENTICAL,     Result::EQUAL_KEEP_A, Result::EQUAL_KEEP_B};
    const char* names[] = {"KEEP_A", "KEEP_B", "KEEP_BOTH", "IDENTICAL", "EQUAL_KEEP_A", "EQUAL_KEEP_B"};
    for (std::size_t i = 0; i < 6; ++i) {
        EXPECT_NE(SubgraphAlgorithm::resultToString(all[i]).find(names[i]), std::string::npos);
    }
}

TEST_F(ResultTextCompletionTest, UnknownEnumValueMapsToUnknown) {
    EXPECT_EQ(SubgraphAlgorithm::resultToString(static_cast<Result>(99)), "UNKNOWN");
}
