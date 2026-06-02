#include "character/character_tables.h"
#include <cstring>

namespace Core {

namespace {
constexpr const char* NATIONALITY_LABELS[] = {
    "American", "Italian", "Irish", "Chinese", "Mexican", "Russian",
    "Greek", "Polish", "Cuban", "Canadian", "British", "French"
};
constexpr int32_t NATIONALITY_LABEL_COUNT = 12;

constexpr const char* HERITAGE_LABELS[] = {
    "Italian", "Sicilian", "Irish", "Jewish", "Chinese", "Polish",
    "Russian", "Greek", "African American", "Mexican"
};
constexpr int32_t HERITAGE_LABEL_COUNT = 10;

constexpr const char* GENERATION_LABELS[] = {
    "Immigrant",
    "First Generation",
    "Second Generation",
    "Third Generation+"
};
constexpr int32_t GENERATION_LABEL_COUNT = 4;

constexpr const char* BACKGROUND_LABELS[] = {
    "Street Hustler",
    "Neighborhood Organizer",
    "Bookkeeper"
};
constexpr int32_t BACKGROUND_LABEL_COUNT = 3;

constexpr const char* BOROUGH_LABELS[] = {
    "Manhattan", "Brooklyn", "Queens", "The Bronx", "Staten Island"
};
constexpr int32_t BOROUGH_LABEL_COUNT = 5;

constexpr const char* GENERATION_ROLE_SUMMARIES[] = {
    "Strong ethnic enclave ties and import pipelines. Best for enforcer, labor rackets, street crime, and smuggling.",
    "Bridges ethnic and mainstream worlds. Best for mid-level organizer, fixer, negotiator, rising lieutenant.",
    "Mainstream institutional access with fading old-country ties. Best for political fixer, business front manager, corrupt official.",
    "Maximum legitimacy and corporate infiltration. Best for lawyer, politician, laundering pipelines, clean-face leadership."
};

bool equalsIgnoreCase(std::string_view left, const char* right) {
    size_t index = 0;
    while (index < left.size() && right[index] != '\0') {
        char leftChar = left[index];
        char rightChar = right[index];
        if (leftChar >= 'A' && leftChar <= 'Z') {
            leftChar = static_cast<char>(leftChar - 'A' + 'a');
        }
        if (rightChar >= 'A' && rightChar <= 'Z') {
            rightChar = static_cast<char>(rightChar - 'A' + 'a');
        }
        if (leftChar != rightChar) {
            return false;
        }
        ++index;
    }
    return index == left.size() && right[index] == '\0';
}
} // namespace

std::string_view getNationalityName(NationalityId nationalityId) {
    const int32_t index = static_cast<int32_t>(nationalityId) - 1;
    if (index < 0 || index >= NATIONALITY_LABEL_COUNT) {
        return "Unknown";
    }
    return NATIONALITY_LABELS[index];
}

std::string_view getHeritageName(HeritageId heritageId) {
    const int32_t index = static_cast<int32_t>(heritageId) - 1;
    if (index < 0 || index >= HERITAGE_LABEL_COUNT) {
        return "Unknown";
    }
    return HERITAGE_LABELS[index];
}

std::string_view getGenerationName(GenerationId generationId) {
    const int32_t index = static_cast<int32_t>(generationId) - 1;
    if (index < 0 || index >= GENERATION_LABEL_COUNT) {
        return "Unknown";
    }
    return GENERATION_LABELS[index];
}

std::string_view getBackgroundName(BackgroundId backgroundId) {
    const int32_t index = static_cast<int32_t>(backgroundId) - 1;
    if (index < 0 || index >= BACKGROUND_LABEL_COUNT) {
        return "Unknown";
    }
    return BACKGROUND_LABELS[index];
}

std::string_view getGenerationRoleSummary(GenerationId generationId) {
    const int32_t index = static_cast<int32_t>(generationId) - 1;
    if (index < 0 || index >= GENERATION_LABEL_COUNT) {
        return "";
    }
    return GENERATION_ROLE_SUMMARIES[index];
}

std::string_view getBoroughPreferenceName(int32_t boroughIndex) {
    if (boroughIndex < 0 || boroughIndex >= BOROUGH_LABEL_COUNT) {
        return "Unknown";
    }
    return BOROUGH_LABELS[boroughIndex];
}

int32_t getNationalityCount() { return NATIONALITY_LABEL_COUNT; }
int32_t getHeritageCount() { return HERITAGE_LABEL_COUNT; }
int32_t getGenerationCount() { return GENERATION_LABEL_COUNT; }
int32_t getBackgroundCount() { return BACKGROUND_LABEL_COUNT; }
int32_t getBoroughPreferenceCount() { return BOROUGH_LABEL_COUNT; }

const char* getNationalityLabel(int32_t index) {
    if (index < 0 || index >= NATIONALITY_LABEL_COUNT) {
        return "";
    }
    return NATIONALITY_LABELS[index];
}

const char* getHeritageLabel(int32_t index) {
    if (index < 0 || index >= HERITAGE_LABEL_COUNT) {
        return "";
    }
    return HERITAGE_LABELS[index];
}

const char* getGenerationLabel(int32_t index) {
    if (index < 0 || index >= GENERATION_LABEL_COUNT) {
        return "";
    }
    return GENERATION_LABELS[index];
}

const char* getBackgroundLabel(int32_t index) {
    if (index < 0 || index >= BACKGROUND_LABEL_COUNT) {
        return "";
    }
    return BACKGROUND_LABELS[index];
}

const char* getBoroughPreferenceLabel(int32_t index) {
    if (index < 0 || index >= BOROUGH_LABEL_COUNT) {
        return "";
    }
    return BOROUGH_LABELS[index];
}

NationalityId nationalityIdFromIndex(int32_t index) {
    if (index < 0 || index >= NATIONALITY_LABEL_COUNT) {
        return NationalityId::None;
    }
    return static_cast<NationalityId>(index + 1);
}

HeritageId heritageIdFromIndex(int32_t index) {
    if (index < 0 || index >= HERITAGE_LABEL_COUNT) {
        return HeritageId::None;
    }
    return static_cast<HeritageId>(index + 1);
}

GenerationId generationIdFromIndex(int32_t index) {
    if (index < 0 || index >= GENERATION_LABEL_COUNT) {
        return GenerationId::None;
    }
    return static_cast<GenerationId>(index + 1);
}

BackgroundId backgroundIdFromIndex(int32_t index) {
    if (index < 0 || index >= BACKGROUND_LABEL_COUNT) {
        return BackgroundId::None;
    }
    return static_cast<BackgroundId>(index + 1);
}

NationalityId parseNationalityName(std::string_view name) {
    for (int32_t index = 0; index < NATIONALITY_LABEL_COUNT; ++index) {
        if (equalsIgnoreCase(name, NATIONALITY_LABELS[index])) {
            return nationalityIdFromIndex(index);
        }
    }
    return NationalityId::None;
}

HeritageId parseHeritageName(std::string_view name) {
    for (int32_t index = 0; index < HERITAGE_LABEL_COUNT; ++index) {
        if (equalsIgnoreCase(name, HERITAGE_LABELS[index])) {
            return heritageIdFromIndex(index);
        }
    }
    return HeritageId::None;
}

GenerationId parseGenerationName(std::string_view name) {
    if (equalsIgnoreCase(name, "immigrant")) {
        return GenerationId::Immigrant;
    }
    if (equalsIgnoreCase(name, "first") || equalsIgnoreCase(name, "firstgen") || equalsIgnoreCase(name, "first generation")) {
        return GenerationId::FirstGeneration;
    }
    if (equalsIgnoreCase(name, "second") || equalsIgnoreCase(name, "secondgen") || equalsIgnoreCase(name, "second generation")) {
        return GenerationId::SecondGeneration;
    }
    if (equalsIgnoreCase(name, "third") || equalsIgnoreCase(name, "thirdgen") || equalsIgnoreCase(name, "third generation")) {
        return GenerationId::ThirdGenerationPlus;
    }
    for (int32_t index = 0; index < GENERATION_LABEL_COUNT; ++index) {
        if (equalsIgnoreCase(name, GENERATION_LABELS[index])) {
            return generationIdFromIndex(index);
        }
    }
    return GenerationId::None;
}

} // namespace Core
