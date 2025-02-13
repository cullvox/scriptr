#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <span>
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
};

using RichTextPropertyValue = std::variant<std::string, float, int, bool, uint32_t /* colors */>;

class RichTextDocument {
public:
    RichTextDocument();
    RichTextDocument(nlohmann::json jsonDocument);
    ~RichTextDocument() = default;

    struct Block {
        Block()
            : text()
            , propertyFlags(0)
            , fontSize(18.0f)
            , foregroundColor(0xFF000000)
            , backgroundColor(0x0)
            , additionalProperties()
            , children()
        {
        }

        std::string text;
        RichTextPropertyFlags propertyFlags;
        float fontSize;
        uint32_t foregroundColor;
        uint32_t backgroundColor;
        std::unordered_map<std::string, RichTextPropertyValue> additionalProperties;
        std::vector<Block> children;
    };

    /// META ///
    std::size_t GetDocumentCharacterLength();
    std::string ExportToJSON();
    std::string ExportToHTML();


    /// CURSOR EDITING ///
    void Insert(std::size_t characterLocation, std::string_view string);
    void Insert(std::size_t characterLocation, std::span<Block> blocks);

private:
    void ImportFromHTML(std::string_view string);
    void ImportFromJSON(std::string_view string);

    std::list<Block> mBlocks;
};