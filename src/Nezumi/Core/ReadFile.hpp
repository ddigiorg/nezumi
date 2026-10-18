// =============================================================================
// ReadFile.hpp
//
// Helper to read an entire file into a string.
// =============================================================================

#pragma once
#include <cstddef>   // byte
#include <cstring>   // memcpy
#include <fstream>   // ifstream
#include <ios>       // iso::binary
#include <iterator>  // istreambuf_iterator
#include <stdexcept> // runtime_error
#include <string>    // string
#include <vector>    // vector

namespace Nezumi {

inline std::vector<std::byte> read_file(std::string filepath) {
    std::ifstream file(filepath, std::ios::binary);

    if (!file) {
        throw std::runtime_error("ReadFile: could not open: '" + filepath + "'");
    }

    std::vector<char> bytes(
        /*.first = */(std::istreambuf_iterator<char>(file)),
        /*.last  = */ std::istreambuf_iterator<char>()
    );

    std::vector<std::byte> out(bytes.size());
    std::memcpy(out.data(), bytes.data(), bytes.size());

    return out;
}

} // namespace Nezumi
