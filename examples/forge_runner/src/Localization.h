#pragma once
#include <raygame/Core.h>
#include <map>

namespace Forge {
enum class Language { English, Chinese };
const char* LanguageCode(Language language);
Language ParseLanguage(const std::string& code);
class TextCatalog {
public:
    void Load(const std::string& path);
    const std::string& Translate(const std::string& english, Language language) const;
    std::vector<int> ChineseCodepoints() const;
    size_t Size() const {
        return translations.size();
    }

private:
    std::map<std::string, std::string> translations;
};
} // namespace Forge
