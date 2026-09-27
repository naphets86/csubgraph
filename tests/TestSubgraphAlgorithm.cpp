#include <gtest/gtest.h>
#include "SubgraphAlgorithm.h"
#include <stdexcept>

// ============================================================================
// Test: Signatur-Berechnung
// ============================================================================

class SignatureCalculationTest : public ::testing::Test {
protected:
    // 2x2 Matrix Test
    std::vector<std::vector<int>> matrix2x2{{0, 1}, {1, 0}};

    // 3x3 Matrix Test
    std::vector<std::vector<int>> matrix3x3{
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0}
    };

    // 4x4 Matrix aus Beispiel
    std::vector<std::vector<int>> matrix4x4{
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };

    // Vollständig gefüllte 2x2 Matrix
    std::vector<std::vector<int>> fullMatrix2x2{
        {1, 1},
        {1, 1}
    };
};

TEST_F(SignatureCalculationTest, CalculateSignatures2x2) {
    auto sigs = SubgraphAlgorithm::calculateSignatures(matrix2x2);
    EXPECT_EQ(sigs.size(), 2);
    // Spalte 0: [0, 1] → 2^1 + 0*2^2 = 2
    // Spalte 1: [1, 0] → 2^0 + 1*2^2 = 1 + 4 = 5
    EXPECT_EQ(sigs[0], 2);
    EXPECT_EQ(sigs[1], 5);
}

TEST_F(SignatureCalculationTest, CalculateSignatures4x4) {
    auto sigs = SubgraphAlgorithm::calculateSignatures(matrix4x4);
    EXPECT_EQ(sigs.size(), 4);
    // Signatur-Vektor sollte distinkte Werte haben (Injektivität)
    for (size_t i = 0; i < sigs.size(); ++i) {
        for (size_t j = i + 1; j < sigs.size(); ++j) {
            EXPECT_NE(sigs[i], sigs[j]) << "Signatures should be unique at index " << i << " and " << j;
        }
    }
}

TEST_F(SignatureCalculationTest, CalculateSignaturesFullMatrix) {
    auto sigs = SubgraphAlgorithm::calculateSignatures(fullMatrix2x2);
    EXPECT_EQ(sigs.size(), 2);
    // Beide Spalten sind [1, 1] aber unterschiedliche Spaltenindizes
    EXPECT_NE(sigs[0], sigs[1]);
}

TEST_F(SignatureCalculationTest, CalculateSignaturesEmptyMatrixThrows) {
    std::vector<std::vector<int>> emptyMatrix;
    EXPECT_THROW(
        SubgraphAlgorithm::calculateSignatures(emptyMatrix),
        std::invalid_argument
    );
}

TEST_F(SignatureCalculationTest, CalculateSignaturesInvalidMatrixThrows) {
    std::vector<std::vector<int>> invalidMatrix{
        {0, 1},
        {1, 2}  // 2 ist ungültig
    };
    EXPECT_THROW(
        SubgraphAlgorithm::calculateSignatures(invalidMatrix),
        std::invalid_argument
    );
}

// ============================================================================
// Test: LCS (Longest Common Subsequence)
// ============================================================================

class LCSTest : public ::testing::Test {};

TEST_F(LCSTest, LCSIdenticalSequences) {
    std::vector<uint64_t> seq{1, 2, 3, 4};
    size_t lcs = SubgraphAlgorithm::computeLCS(seq, seq);
    EXPECT_EQ(lcs, 4);
}

TEST_F(LCSTest, LCSNoCommonElements) {
    std::vector<uint64_t> seq1{1, 2, 3};
    std::vector<uint64_t> seq2{4, 5, 6};
    size_t lcs = SubgraphAlgorithm::computeLCS(seq1, seq2);
    EXPECT_EQ(lcs, 0);
}

TEST_F(LCSTest, LCSPartialCommon) {
    std::vector<uint64_t> seq1{1, 2, 3, 4};
    std::vector<uint64_t> seq2{2, 3, 5};
    size_t lcs = SubgraphAlgorithm::computeLCS(seq1, seq2);
    EXPECT_EQ(lcs, 2);  // [2, 3]
}

TEST_F(LCSTest, LCSEmptyFirstSequence) {
    std::vector<uint64_t> seq1;
    std::vector<uint64_t> seq2{1, 2, 3};
    size_t lcs = SubgraphAlgorithm::computeLCS(seq1, seq2);
    EXPECT_EQ(lcs, 0);
}

TEST_F(LCSTest, LCSEmptySecondSequence) {
    std::vector<uint64_t> seq1{1, 2, 3};
    std::vector<uint64_t> seq2;
    size_t lcs = SubgraphAlgorithm::computeLCS(seq1, seq2);
    EXPECT_EQ(lcs, 0);
}

TEST_F(LCSTest, LCSBothEmpty) {
    std::vector<uint64_t> seq1;
    std::vector<uint64_t> seq2;
    size_t lcs = SubgraphAlgorithm::computeLCS(seq1, seq2);
    EXPECT_EQ(lcs, 0);
}

TEST_F(LCSTest, LCSWithLargeValues) {
    std::vector<uint64_t> seq1{1000000, 2000000, 3000000};
    std::vector<uint64_t> seq2{1000000, 3000000};
    size_t lcs = SubgraphAlgorithm::computeLCS(seq1, seq2);
    EXPECT_EQ(lcs, 2);  // [1000000, 3000000]
}

// ============================================================================
// Test: Zyklische Rotation
// ============================================================================

class RotationTest : public ::testing::Test {};

TEST_F(RotationTest, RotateSequenceZero) {
    std::vector<uint64_t> seq{1, 2, 3, 4};
    auto rotated = SubgraphAlgorithm::rotateSequence(seq, 0);
    EXPECT_EQ(rotated, seq);
}

TEST_F(RotationTest, RotateSequenceOne) {
    std::vector<uint64_t> seq{1, 2, 3, 4};
    auto rotated = SubgraphAlgorithm::rotateSequence(seq, 1);
    std::vector<uint64_t> expected{4, 1, 2, 3};
    EXPECT_EQ(rotated, expected);
}

TEST_F(RotationTest, RotateSequenceFull) {
    std::vector<uint64_t> seq{1, 2, 3, 4};
    auto rotated = SubgraphAlgorithm::rotateSequence(seq, 4);
    EXPECT_EQ(rotated, seq);
}

TEST_F(RotationTest, RotateSequenceModulo) {
    std::vector<uint64_t> seq{1, 2, 3, 4};
    auto rotated1 = SubgraphAlgorithm::rotateSequence(seq, 1);
    auto rotated5 = SubgraphAlgorithm::rotateSequence(seq, 5);
    EXPECT_EQ(rotated1, rotated5);
}

TEST_F(RotationTest, RotateSequenceEmpty) {
    std::vector<uint64_t> seq;
    auto rotated = SubgraphAlgorithm::rotateSequence(seq, 1);
    EXPECT_EQ(rotated.size(), 0);
}

TEST_F(RotationTest, RotateSequenceSingle) {
    std::vector<uint64_t> seq{42};
    auto rotated = SubgraphAlgorithm::rotateSequence(seq, 1);
    EXPECT_EQ(rotated[0], 42);
}

// ============================================================================
// Test: Zeilenkomponenten-Extraktion
// ============================================================================

class RowComponentsTest : public ::testing::Test {};

TEST_F(RowComponentsTest, ExtractRowComponents2x2) {
    std::vector<uint64_t> sigs{2, 5};  // Mit Spaltengewichtung
    auto rowComp = SubgraphAlgorithm::extractRowComponents(sigs, 2);
    EXPECT_EQ(rowComp.size(), 2);
    // 2 & 0b11 = 2, 5 & 0b11 = 1
    EXPECT_EQ(rowComp[0], 2);
    EXPECT_EQ(rowComp[1], 1);
}

TEST_F(RowComponentsTest, ExtractRowComponents3x3) {
    std::vector<uint64_t> sigs{0, 1, 2};
    auto rowComp = SubgraphAlgorithm::extractRowComponents(sigs, 3);
    EXPECT_EQ(rowComp.size(), 3);
}

TEST_F(RowComponentsTest, ExtractRowComponentsEmpty) {
    std::vector<uint64_t> sigs;
    auto rowComp = SubgraphAlgorithm::extractRowComponents(sigs, 5);
    EXPECT_EQ(rowComp.size(), 0);
}

// ============================================================================
// Test: Validierung von Adjazenzmatrizen
// ============================================================================

class MatrixValidationTest : public ::testing::Test {};

TEST_F(MatrixValidationTest, ValidMatrix) {
    std::vector<std::vector<int>> valid{{0, 1}, {1, 0}};
    EXPECT_TRUE(SubgraphAlgorithm::isValidAdjacencyMatrix(valid));
}

TEST_F(MatrixValidationTest, EmptyMatrix) {
    std::vector<std::vector<int>> empty;
    EXPECT_FALSE(SubgraphAlgorithm::isValidAdjacencyMatrix(empty));
}

TEST_F(MatrixValidationTest, NonSquareMatrix) {
    std::vector<std::vector<int>> nonSquare{{0, 1}, {1, 0}, {0, 1}};
    EXPECT_FALSE(SubgraphAlgorithm::isValidAdjacencyMatrix(nonSquare));
}

TEST_F(MatrixValidationTest, InvalidBinaryValue) {
    std::vector<std::vector<int>> invalid{{0, 1}, {1, 2}};
    EXPECT_FALSE(SubgraphAlgorithm::isValidAdjacencyMatrix(invalid));
}

TEST_F(MatrixValidationTest, RowMismatchSize) {
    std::vector<std::vector<int>> mismatch{{0, 1}, {1}};
    EXPECT_FALSE(SubgraphAlgorithm::isValidAdjacencyMatrix(mismatch));
}

// ============================================================================
// Test: Graph-Vergleich (Kernfunktionalität)
// ============================================================================

class GraphComparisonTest : public ::testing::Test {
protected:
    // Einfache verkettete Graphen
    std::vector<std::vector<int>> chainGraph4{
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };

    // Wie chainGraph4, aber mit zusätzlicher Kante
    std::vector<std::vector<int>> chainGraph4Extended{
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };

    // Identisch zu chainGraph4
    std::vector<std::vector<int>> chainGraph4Identical{
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };

    // 3-Knoten Graph
    std::vector<std::vector<int>> graph3nodes{
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0}
    };

    // 2-Knoten Graph
    std::vector<std::vector<int>> graph2nodes{
        {0, 1},
        {0, 0}
    };
};

TEST_F(GraphComparisonTest, CompareIdenticalGraphs) {
    auto result = SubgraphAlgorithm::compareGraphs(chainGraph4, chainGraph4Identical);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::IDENTICAL);
}

TEST_F(GraphComparisonTest, CompareDifferentGraphs) {
    auto result = SubgraphAlgorithm::compareGraphs(graph2nodes, graph3nodes);
    EXPECT_NE(result, SubgraphAlgorithm::Result::IDENTICAL);
}

TEST_F(GraphComparisonTest, CompareWithInvalidGraphAThrows) {
    std::vector<std::vector<int>> invalid{{0, 2}};
    EXPECT_THROW(
        SubgraphAlgorithm::compareGraphs(invalid, chainGraph4),
        std::invalid_argument
    );
}

TEST_F(GraphComparisonTest, CompareWithInvalidGraphBThrows) {
    std::vector<std::vector<int>> invalid{{0, 1}, {1}};
    EXPECT_THROW(
        SubgraphAlgorithm::compareGraphs(chainGraph4, invalid),
        std::invalid_argument
    );
}

TEST_F(GraphComparisonTest, CompareEmptyGraphThrows) {
    std::vector<std::vector<int>> empty;
    EXPECT_THROW(
        SubgraphAlgorithm::compareGraphs(empty, chainGraph4),
        std::invalid_argument
    );
}

TEST_F(GraphComparisonTest, CompareSimpleChainGraphs) {
    std::vector<std::vector<int>> g1{{0, 1}, {0, 0}};
    std::vector<std::vector<int>> g2{{0, 1}, {0, 0}};
    auto result = SubgraphAlgorithm::compareGraphs(g1, g2);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::IDENTICAL);
}

TEST_F(GraphComparisonTest, CompareComplexGraphs) {
    std::vector<std::vector<int>> g1{
        {0, 1, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };
    std::vector<std::vector<int>> g2{
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };
    auto result = SubgraphAlgorithm::compareGraphs(g1, g2);
    EXPECT_NE(result, SubgraphAlgorithm::Result::IDENTICAL);
}

TEST_F(GraphComparisonTest, CompareLargerGraph) {
    std::vector<std::vector<int>> g5{
        {0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0}
    };
    std::vector<std::vector<int>> g4{
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
        {0, 0, 0, 0}
    };
    auto result = SubgraphAlgorithm::compareGraphs(g4, g5);
    EXPECT_NE(result, SubgraphAlgorithm::Result::IDENTICAL);
}

// ============================================================================
// Test: Result-zu-String Konvertierung
// ============================================================================

class ResultStringTest : public ::testing::Test {};

TEST_F(ResultStringTest, ResultToStringKeepA) {
    auto str = SubgraphAlgorithm::resultToString(SubgraphAlgorithm::Result::KEEP_A);
    EXPECT_NE(str.find("KEEP_A"), std::string::npos);
}

TEST_F(ResultStringTest, ResultToStringKeepB) {
    auto str = SubgraphAlgorithm::resultToString(SubgraphAlgorithm::Result::KEEP_B);
    EXPECT_NE(str.find("KEEP_B"), std::string::npos);
}

TEST_F(ResultStringTest, ResultToStringKeepBoth) {
    auto str = SubgraphAlgorithm::resultToString(SubgraphAlgorithm::Result::KEEP_BOTH);
    EXPECT_NE(str.find("KEEP_BOTH"), std::string::npos);
}

TEST_F(ResultStringTest, ResultToStringIdentical) {
    auto str = SubgraphAlgorithm::resultToString(SubgraphAlgorithm::Result::IDENTICAL);
    EXPECT_NE(str.find("IDENTICAL"), std::string::npos);
}

// ============================================================================
// Test: Edge Cases und Grenzen
// ============================================================================

class EdgeCasesTest : public ::testing::Test {};

TEST_F(EdgeCasesTest, SingleNodeGraph) {
    std::vector<std::vector<int>> g1{{0}};
    std::vector<std::vector<int>> g2{{0}};
    auto result = SubgraphAlgorithm::compareGraphs(g1, g2);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::IDENTICAL);
}

TEST_F(EdgeCasesTest, DisconnectedGraph) {
    std::vector<std::vector<int>> disconnected{
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };
    std::vector<std::vector<int>> connected{{0, 1}, {0, 0}};
    auto result = SubgraphAlgorithm::compareGraphs(disconnected, connected);
    EXPECT_NE(result, SubgraphAlgorithm::Result::IDENTICAL);
}

TEST_F(EdgeCasesTest, CompleteGraph2) {
    std::vector<std::vector<int>> complete{
        {1, 1},
        {1, 1}
    };
    std::vector<std::vector<int>> sparse{{0, 1}, {0, 0}};
    auto result = SubgraphAlgorithm::compareGraphs(sparse, complete);
    EXPECT_NE(result, SubgraphAlgorithm::Result::KEEP_A);
}

TEST_F(EdgeCasesTest, LongChainGraph) {
    std::vector<std::vector<int>> chain6(6, std::vector<int>(6, 0));
    for (size_t i = 0; i < 5; ++i) {
        chain6[i][i + 1] = 1;
    }
    auto sigs = SubgraphAlgorithm::calculateSignatures(chain6);
    EXPECT_EQ(sigs.size(), 6);
}

TEST_F(EdgeCasesTest, RotationSequenceLarge) {
    std::vector<uint64_t> large(100);
    for (size_t i = 0; i < 100; ++i) {
        large[i] = i;
    }
    auto rotated = SubgraphAlgorithm::rotateSequence(large, 50);
    EXPECT_EQ(rotated.size(), 100);
}


// ============================================================================
// Test: Echte Subgraph-Beziehungen (NEU)
// ============================================================================

class SubgraphRelationshipTest : public ::testing::Test {};

TEST_F(SubgraphRelationshipTest, Subgraph5x5to3x3) {
    std::vector<std::vector<int>> large{
        {0, 1, 1, 0, 0},
        {1, 0, 1, 1, 0},
        {1, 1, 0, 1, 1},
        {0, 1, 1, 0, 1},
        {0, 0, 1, 1, 0}
    };
    
    std::vector<std::vector<int>> small{
        {0, 1, 1},
        {1, 0, 1},
        {1, 1, 0}
    };
    
    auto result = SubgraphAlgorithm::compareGraphs(large, small);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::KEEP_B);
}

TEST_F(SubgraphRelationshipTest, ChainGraphSubgraph) {
    std::vector<std::vector<int>> chain5{
        {0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0},
        {0, 0, 0, 1, 0},
        {0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0}
    };
    
    std::vector<std::vector<int>> chain3{
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0}
    };
    
    auto result = SubgraphAlgorithm::compareGraphs(chain5, chain3);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::KEEP_B);
}

TEST_F(SubgraphRelationshipTest, TriangleInLargerGraph) {
    std::vector<std::vector<int>> largeTriangle{
        {0, 1, 1, 0, 0},
        {1, 0, 1, 0, 0},
        {1, 1, 0, 0, 0},
        {0, 0, 0, 0, 1},
        {0, 0, 0, 1, 0}
    };
    
    std::vector<std::vector<int>> triangle{
        {0, 1, 1},
        {1, 0, 1},
        {1, 1, 0}
    };
    
    auto result = SubgraphAlgorithm::compareGraphs(largeTriangle, triangle);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::KEEP_B);
}

TEST_F(SubgraphRelationshipTest, NoSubgraphRelationship) {
    std::vector<std::vector<int>> star{
        {0, 1, 1, 1},
        {1, 0, 0, 0},
        {1, 0, 0, 0},
        {1, 0, 0, 0}
    };
    
    std::vector<std::vector<int>> line{
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0}
    };
    
    auto result = SubgraphAlgorithm::compareGraphs(star, line);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::KEEP_BOTH);
}

TEST_F(SubgraphRelationshipTest, ComplexSubgraph6to4Nodes) {
    std::vector<std::vector<int>> graph6{
        {0, 1, 1, 0, 1, 0},
        {1, 0, 1, 1, 0, 1},
        {1, 1, 0, 1, 0, 0},
        {0, 1, 1, 0, 1, 1},
        {1, 0, 0, 1, 0, 1},
        {0, 1, 0, 1, 1, 0}
    };
    
    std::vector<std::vector<int>> graph4{
        {0, 1, 1, 0},
        {1, 0, 1, 1},
        {1, 1, 0, 1},
        {0, 1, 1, 0}
    };
    
    auto result = SubgraphAlgorithm::compareGraphs(graph6, graph4);
    EXPECT_EQ(result, SubgraphAlgorithm::Result::KEEP_B);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
