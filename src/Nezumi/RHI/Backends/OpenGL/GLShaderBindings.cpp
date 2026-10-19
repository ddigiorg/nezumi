// =============================================================================
// GLShaderBindings.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLShaderBindings.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <regex>
#include <string_view>

namespace Nezumi::RHI::GL {

namespace {

// A copy of `source` with comments replaced by spaces (newlines kept), so
// offsets into it are offsets into `source`.
std::string blank_comments(const std::string& source) {
    std::string out = source;
    std::size_t i = 0;
    while (i < out.size()) {
        if (out.compare(i, 2, "//") == 0) {
            while (i < out.size() && out[i] != '\n') out[i++] = ' ';
        } else if (out.compare(i, 2, "/*") == 0) {
            const std::size_t end = out.find("*/", i + 2);
            const std::size_t stop = end == std::string::npos ? out.size() : end + 2;
            for (; i < stop; ++i) {
                if (out[i] != '\n') out[i] = ' ';
            }
        } else {
            ++i;
        }
    }
    return out;
}

std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))  s.remove_suffix(1);
    return s;
}

struct Edit {
    std::size_t offset = 0;
    std::size_t length = 0;
    std::string text;
};

} // namespace

std::vector<GLUniformBinding> strip_binding_qualifiers(std::string& source) {
    // layout( <qualifiers> ) uniform <type or block name> <name or '{'>
    static const std::regex declaration(
        R"(layout\s*\(([^)]*)\)\s*uniform\s+(\w+)\s*(\{|\w+))");
    static const std::regex binding_qualifier(R"(^binding\s*=\s*(\d+)$)");

    const std::string scan = blank_comments(source);

    std::vector<GLUniformBinding> bindings;
    std::vector<Edit>             edits;

    for (auto it = std::sregex_iterator(scan.begin(), scan.end(), declaration);
         it != std::sregex_iterator(); ++it)
    {
        const std::smatch& m = *it;

        // Split the qualifier list, pulling out `binding = N`.
        std::string   kept;
        bool          found   = false;
        std::uint32_t binding = 0;

        const std::string list = m[1].str();
        std::size_t start = 0;
        while (start <= list.size()) {
            std::size_t comma = list.find(',', start);
            if (comma == std::string::npos) comma = list.size();

            const std::string item(trim(std::string_view(list).substr(start, comma - start)));
            std::smatch b;
            if (std::regex_match(item, b, binding_qualifier)) {
                found   = true;
                binding = static_cast<std::uint32_t>(std::stoul(b[1].str()));
            } else if (!item.empty()) {
                if (!kept.empty()) kept += ", ";
                kept += item;
            }
            start = comma + 1;
        }

        if (!found) continue;

        const bool block = m[3].str() == "{";
        bindings.push_back({ block ? m[2].str() : m[3].str(), binding, block });

        // Replace the whole `layout(...)`, keeping any newlines it spanned.
        const std::size_t offset = static_cast<std::size_t>(m.position(0));
        const std::size_t length = scan.find(')', offset) + 1 - offset;

        std::string text = kept.empty() ? std::string() : "layout(" + kept + ")";
        text.append(static_cast<std::size_t>(
            std::count(scan.begin() + static_cast<std::ptrdiff_t>(offset),
                       scan.begin() + static_cast<std::ptrdiff_t>(offset + length), '\n')), '\n');

        edits.push_back({ offset, length, std::move(text) });
    }

    // Back to front, so earlier offsets stay valid.
    for (auto it = edits.rbegin(); it != edits.rend(); ++it) {
        source.replace(it->offset, it->length, it->text);
    }

    return bindings;
}

void apply_bindings(GLuint program, std::span<const GLUniformBinding> bindings) {
    GLint previous = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous);
    glUseProgram(program);

    for (const GLUniformBinding& b : bindings) {
        if (b.block) {
            const GLuint index = glGetUniformBlockIndex(program, b.name.c_str());
            if (index != GL_INVALID_INDEX) glUniformBlockBinding(program, index, b.binding);
        } else {
            const GLint location = glGetUniformLocation(program, b.name.c_str());
            if (location >= 0) glUniform1i(location, static_cast<GLint>(b.binding));
        }
    }

    glUseProgram(static_cast<GLuint>(previous));
}

} // namespace Nezumi::RHI::GL
