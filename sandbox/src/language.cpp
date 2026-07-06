#include "ocpe/language.hpp"

#include <algorithm>
#include <cctype>

namespace ocpe {

std::vector<LanguageInfo> supported_languages() {
    return {
        {"c", "C", ".c"},
        {"cpp", "C++", ".cpp"},
        {"python", "Python 3", ".py"},
    };
}

bool is_supported_language(const std::string& language) {
    std::string lang = normalize_language(language);
    for (const auto& l : supported_languages()) {
        if (l.id == lang) return true;
    }
    return false;
}

std::string source_extension(const std::string& language) {
    std::string lang = normalize_language(language);
    for (const auto& l : supported_languages()) {
        if (l.id == lang) return l.extension;
    }
    return ".c";
}

std::string normalize_language(const std::string& language) {
    std::string lang = language;
    std::transform(lang.begin(), lang.end(), lang.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lang == "c++" || lang == "cxx") return "cpp";
    if (lang == "py" || lang == "python3") return "python";
    return lang;
}

}  // namespace ocpe
