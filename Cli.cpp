#include "SubgraphAlgorithm.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <cstdlib>

using json = nlohmann::json;

/**
 * @brief CLI-Tool für Subgraph-Vergleiche mit JSON I/O
 * 
 * Liest JSON von stdin mit Schema:
 * {
 *   "graph_a": [[int, ...], ...],
 *   "graph_b": [[int, ...], ...]
 * }
 * 
 * Schreibt JSON auf stdout:
 * {
 *   "result": "KEEP_A|KEEP_B|KEEP_BOTH|IDENTICAL",
 *   "result_code": 0|1|2|3,
 *   "error": null | "error message"
 * }
 */

int main() {
    try {
        // JSON von stdin lesen
        json input;
        std::cin >> input;

        // Validiere Input-Schema
        if (!input.contains("graph_a") || !input.contains("graph_b")) {
            json error_response = {
                {"result", nullptr},
                {"result_code", -1},
                {"error", "Missing required fields: graph_a and graph_b"}
            };
            std::cout << error_response.dump() << std::endl;
            return EXIT_FAILURE;
        }

        // Konvertiere JSON zu Adjazenzmatrizen
        std::vector<std::vector<int>> graphA = input["graph_a"].get<std::vector<std::vector<int>>>();
        std::vector<std::vector<int>> graphB = input["graph_b"].get<std::vector<std::vector<int>>>();

        // Führe Algorithmus aus
        SubgraphAlgorithm::Result result = SubgraphAlgorithm::compareGraphs(graphA, graphB);

        // Konvertiere Result zu String und Code
        std::string resultStr;
        int resultCode;

        switch (result) {
            case SubgraphAlgorithm::Result::KEEP_A:
                resultStr = "KEEP_A";
                resultCode = 0;
                break;
            case SubgraphAlgorithm::Result::KEEP_B:
                resultStr = "KEEP_B";
                resultCode = 1;
                break;
            case SubgraphAlgorithm::Result::KEEP_BOTH:
                resultStr = "KEEP_BOTH";
                resultCode = 2;
                break;
            case SubgraphAlgorithm::Result::IDENTICAL:
                resultStr = "IDENTICAL";
                resultCode = 3;
                break;
            default:
                resultStr = "UNKNOWN";
                resultCode = -1;
        }

        // Schreibe Erfolgs-Response
        json response = {
            {"result", resultStr},
            {"result_code", resultCode},
            {"error", nullptr}
        };

        std::cout << response.dump() << std::endl;
        return EXIT_SUCCESS;

    } catch (const json::exception& e) {
        // JSON Parse Error
        json error_response = {
            {"result", nullptr},
            {"result_code", -1},
            {"error", std::string("JSON parse error: ") + e.what()}
        };
        std::cerr << error_response.dump() << std::endl;
        return EXIT_FAILURE;

    } catch (const std::invalid_argument& e) {
        // Algorithmus Validierungsfehler
        json error_response = {
            {"result", nullptr},
            {"result_code", -1},
            {"error", std::string("Validation error: ") + e.what()}
        };
        std::cerr << error_response.dump() << std::endl;
        return EXIT_FAILURE;

    } catch (const std::exception& e) {
        // Allgemeiner Fehler
        json error_response = {
            {"result", nullptr},
            {"result_code", -1},
            {"error", std::string("Unexpected error: ") + e.what()}
        };
        std::cerr << error_response.dump() << std::endl;
        return EXIT_FAILURE;
    }
}
