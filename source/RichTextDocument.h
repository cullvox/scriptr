#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <nlohmann/json_fwd.hpp>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

typedef int RichTextPropertyFlags;
enum RichTextPropertyFlagBits {
    RichTextPropertyFlags_Bold = 0x00000001,
    RichTextPropertyFlags_Italic = 0x00000002,
    RichTextPropertyFlags_Underline = 0x00000004,
    RichTextPropertyFlags_Centered = 0x00000008,
    RichTextPropertyFlags_RightAligned = 0x00000010,
    RichTextPropertyFlag_IsLine = 0x00000020
};

using RichTextPropertyValue = std::variant<std::u8string, float, int, bool, uint32_t /* colors */>;

class RichTextBlock {
public:
    RichTextBlock()
        : text()
        , propertyFlags(0)
        , fontSize(18.0f)
        , foregroundColor(0xFF000000)
        , backgroundColor(0x0)
        , additionalProperties()
        , children()
    {
    }

    std::u8string text;
    RichTextPropertyFlags propertyFlags;
    float fontSize;
    uint32_t foregroundColor;
    uint32_t backgroundColor;
    std::unordered_map<std::u8string, RichTextPropertyValue> additionalProperties;
    std::list<RichTextBlock> children;
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
    const RichTextBlock& GetBlocks() const { return mRootBlock; }
    std::size_t GetLineCount();
    RichTextBlock GetLine(int line);
    RichTextBlock AsLines();

    /// EDITING ///
    void Insert(std::size_t characterLocation, std::u8string_view string);
    void Insert(std::size_t characterLocation, std::span<RichTextBlock> blocks);
    void Remove(std::size_t characterStart, std::size_t characterEnd);
    
private:
    RichTextBlock ParseTextBlock(int currentLine, nlohmann::json formatObject, RichTextBlock* parent);
    size_t UTF8CharLength(char c);

    void ImportFromHTML(std::u8string_view string);
    void ImportFromJSON(std::u8string_view string);

    RichTextBlock mRootBlock;
};