#include "matFact.hpp"
#include "matrix.hpp"
#include "parser.hpp"

int main(int argc, char const** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [filename]\n";
        return EXIT_FAILURE;
    }
    auto mmatrices = parser::parse(argv[1]);
    if (mmatrices.is_err()) {
        switch (std::move(mmatrices).unwrap_err()) {
            case parser::ParserError::IO:
                std::cerr << "IO Error\n";
                return EXIT_FAILURE;
            case parser::ParserError::INVALID_FORMAT:
                std::cerr << "Format Error\n";
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    auto matrices = std::move(mmatrices).unwrap();
    matrices.output(factorization::iter(matrices));
    return EXIT_SUCCESS;
}
