#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include <nlohmann/json_fwd.hpp>

typedef int RichTextPropertyFlags;
enum RichTextPropertyFlagBits {
    RichTextPropertyFlags_Bold = 0x00000001,
    RichTextPropertyFlags_Italic = 0x00000002,
    RichTextPropertyFlags_Underline = 0x00000004,
    RichTextPropertyFlags_Centered = 0x00000008,
    RichTextPropertyFlags_RightAligned = 0x00000010,
    RichTextPropertyFlag_IsLine = 0x00000020
};

using RichTextPropertyValue = std::variant<std::string, float, int, bool, uint32_t /* colors */>;

class RichTextBlock {
public:
    RichTextBlock()
        : text()
        , propertyFlags(0)
        , fontSize(18.0f)
        , foregroundColor(0xFF000000)
        , backgroundColor(0x0)
        , additionalProperties()
    {
    }

    std::string text;
    RichTextPropertyFlags propertyFlags;
    float fontSize;
    uint32_t foregroundColor;
    uint32_t backgroundColor;
    std::unordered_map<std::string, RichTextPropertyValue> additionalProperties;
};

class RichTextDocument {
public:
    RichTextDocument();
    RichTextDocument(nlohmann::json jsonDocument);
    ~RichTextDocument() = default;

    /// META ///
    std::size_t GetDocumentCharacterLength();
    std::string ExportToJSON();
    std::string ExportToHTML();
    auto& GetBlocks() const { return mBlocks; }
    std::size_t GetLineCount() const;
    std::list<RichTextBlock> GetLine(int line) const;

    /// EDITING ///
    void Insert(std::size_t characterLocation, std::string_view string);
    void Insert(std::size_t characterLocation, const std::list<RichTextBlock>& blocks);
    void Remove(std::size_t characterStart, std::size_t characterEnd);
    
private:
    void ParseTextBlock(std::list<RichTextBlock>& blocks, int currentLine, nlohmann::json formatObject, RichTextBlock* parent);
    size_t UTF8CharLength(char c);
    std::optional<uint32_t> ParseHexColorCode(const std::string& code);

    void ImportFromHTML(std::string_view string);
    void ImportFromJSON(std::string_view string);

    std::list<RichTextBlock> mBlocks;
};