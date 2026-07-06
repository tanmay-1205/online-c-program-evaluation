#pragma once

#include <string>
#include <vector>

namespace ocpe {

struct LanguageInfo {
    std::string id;
    std::string name;
    std::string extension;
};

std::vector<LanguageInfo> supported_languages();
bool is_supported_language(const std::string& language);
std::string source_extension(const std::string& language);
std::string normalize_language(const std::string& language);

}  // namespace ocpe
