#include "character/character_name.h"
#include <cctype>
#include <cstdio>
#include <cstring>

namespace Core {

namespace {

bool isAllowedNameCharacter(char character) {
    return std::isalpha(static_cast<unsigned char>(character)) != 0 || character == '\'' || character == '-';
}

void trimEdges(char* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return;
    }
    size_t start = 0;
    while (start + 1 < bufferSize && buffer[start] == ' ') {
        ++start;
    }
    size_t end = std::strlen(buffer);
    while (end > start && buffer[end - 1] == ' ') {
        --end;
    }
    if (start > 0) {
        std::memmove(buffer, buffer + start, end - start + 1);
    }
    buffer[end - start] = '\0';
}

void collapseInternalSpaces(char* buffer) {
    if (buffer == nullptr) {
        return;
    }
    size_t writeIndex = 0;
    bool isPreviousSpace = true;
    for (size_t readIndex = 0; buffer[readIndex] != '\0'; ++readIndex) {
        const char character = buffer[readIndex];
        if (character == ' ') {
            if (!isPreviousSpace && writeIndex > 0) {
                buffer[writeIndex++] = ' ';
            }
            isPreviousSpace = true;
            continue;
        }
        buffer[writeIndex++] = character;
        isPreviousSpace = false;
    }
    if (writeIndex > 0 && buffer[writeIndex - 1] == ' ') {
        --writeIndex;
    }
    buffer[writeIndex] = '\0';
}

void capitalizeWord(char* buffer) {
    if (buffer == nullptr || buffer[0] == '\0') {
        return;
    }
    bool isWordStart = true;
    for (size_t index = 0; buffer[index] != '\0'; ++index) {
        char& character = buffer[index];
        if (character == ' ' || character == '-' || character == '\'') {
            isWordStart = true;
            continue;
        }
        if (isWordStart) {
            character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
            isWordStart = false;
            continue;
        }
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
}

} // namespace

void sanitizeCharacterNamePart(char* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return;
    }
    size_t writeIndex = 0;
    for (size_t readIndex = 0; buffer[readIndex] != '\0' && writeIndex + 1 < bufferSize; ++readIndex) {
        const char character = buffer[readIndex];
        if (character == ' ') {
            if (writeIndex > 0 && buffer[writeIndex - 1] != ' ') {
                buffer[writeIndex++] = ' ';
            }
            continue;
        }
        if (!isAllowedNameCharacter(character)) {
            continue;
        }
        buffer[writeIndex++] = character;
    }
    buffer[writeIndex] = '\0';
    trimEdges(buffer, bufferSize);
    collapseInternalSpaces(buffer);
    capitalizeWord(buffer);
}

void rebuildCharacterFullName(CharacterDraft& draft) {
    sanitizeCharacterNamePart(draft.firstName, sizeof(draft.firstName));
    sanitizeCharacterNamePart(draft.middleName, sizeof(draft.middleName));
    sanitizeCharacterNamePart(draft.lastName, sizeof(draft.lastName));
    draft.nameBuffer[0] = '\0';
    if (draft.firstName[0] == '\0' && draft.lastName[0] == '\0') {
        return;
    }
    if (draft.middleName[0] != '\0') {
        std::snprintf(draft.nameBuffer, sizeof(draft.nameBuffer), "%s %s %s", draft.firstName, draft.middleName, draft.lastName);
        return;
    }
    std::snprintf(draft.nameBuffer, sizeof(draft.nameBuffer), "%s %s", draft.firstName, draft.lastName);
}

bool isCharacterNameValid(const CharacterDraft& draft) {
    return draft.firstName[0] != '\0' && draft.lastName[0] != '\0';
}

void normalizeCharacterDraftNames(CharacterDraft& draft) {
    rebuildCharacterFullName(draft);
}

} // namespace Core
