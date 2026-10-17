#include "benchmark/benchmark_runner.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void check(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
        ++failures;
    }
}

std::vector<std::string> split(const std::string& text, const char delimiter) {
    std::vector<std::string> fields;
    std::istringstream input(text);
    std::string field;
    while (std::getline(input, field, delimiter)) {
        fields.push_back(field);
    }
    return fields;
}

}  // namespace

int main() {
    using selection_benchmark::BenchmarkCase;
    using selection_benchmark::InputFamily;

    constexpr std::array cases{
        BenchmarkCase{InputFamily::RandomUnique, 21, 10, 14487692U, 20261017U},
    };

    std::ostringstream output;
    selection_benchmark::writeBenchmarkResults(output, cases);

    std::vector<std::string> lines;
    std::istringstream csv(output.str());
    std::string line;
    while (std::getline(csv, line)) {
        lines.push_back(line);
    }

    check(lines.size() == 1 + 2 * selection_benchmark::benchmarkRepetitions,
          "CSV does not contain one header and fourteen timing rows");
    check(!lines.empty() && lines.front() == selection_benchmark::benchmarkCsvHeader,
          "CSV header is incorrect");

    for (std::size_t repetition = 0;
         repetition < selection_benchmark::benchmarkRepetitions; ++repetition) {
        const std::vector<std::string> first = split(lines[1 + 2 * repetition], ',');
        const std::vector<std::string> second =
            split(lines[2 + 2 * repetition], ',');
        check(first.size() == 9 && second.size() == 9,
              "timing row does not contain nine columns");
        if (first.size() != 9 || second.size() != 9) {
            continue;
        }

        const bool deterministicFirst = repetition % 2 == 0;
        check(first[5] ==
                  (deterministicFirst ? "median-of-medians"
                                      : "randomised-quickselect"),
              "first algorithm did not alternate correctly");
        check(second[5] ==
                  (deterministicFirst ? "randomised-quickselect"
                                      : "median-of-medians"),
              "second algorithm did not alternate correctly");
        check(first[6] == std::to_string(repetition) &&
                  second[6] == std::to_string(repetition),
              "repetition number is incorrect");
        check(first[8] == "0" && second[8] == "0",
              "benchmark runner recorded the wrong selection result");
    }

    try {
        constexpr std::array invalidCases{
            BenchmarkCase{InputFamily::Sorted, 10, 10, 1U, 2U},
        };
        std::ostringstream invalidOutput;
        selection_benchmark::writeBenchmarkResults(invalidOutput, invalidCases);
        check(false, "invalid benchmark rank should throw");
    } catch (const std::invalid_argument&) {
    } catch (...) {
        check(false, "invalid benchmark rank threw the wrong exception");
    }

    if (failures != 0) {
        std::cerr << failures << " benchmark runner test(s) failed.\n";
        return 1;
    }

    std::cout << "All benchmark runner tests passed.\n";
    return 0;
}

