#pragma once

#include <list>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <unordered_map>
#include <string>
#include <variant>

#include "imgui.h"

typedef int RichTextPropertyFlags;
enum RichTextPropertyFlagBits {
    RichTextPropertyFlags_Bold = 0x00000001,
    RichTextPropertyFlags_Italic = 0x00000002,
    RichTextPropertyFlags_Underline = 0x00000004,
};

using RichTextPropertyValue = std::variant<std::string, float, int, bool, ImU32 /* colors */>;

class RichTextEditor {
public:
    RichTextEditor(ImFont* normalFont=nullptr, ImFont* boldFont=nullptr, ImFont* italicFont=nullptr, ImFont* italicBoldFont=nullptr);
    ~RichTextEditor();

    void SetText(nlohmann::json formattedText);
    nlohmann::json GetText();
    std::string GetTextUnformatted();
    void SetCursorLocation(int line, int column);

    void Render();

private:
    struct Block {
        Block()
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
        ImU32 foregroundColor;
        ImU32 backgroundColor;
        std::unordered_map<std::string, RichTextPropertyValue> additionalProperties;
    };

    typedef std::list<Block> Line;
    typedef std::list<Line> Lines;

    std::optional<ImU32> ParseHexColorCode(const std::string& code);
    Block ParseTextBlock(Lines& lines, int currentLine, nlohmann::json formatObject, Block* parent);
    size_t UTF8CharLength(char c);
    void HandleKeyboardInput();

    float mDefaultFontSize = 18.0f;
    double mCursorTimeOffset = 0.0;
    int mCursorLine;
    int mCursorColumn;

    ImFont* mNormalFont;
    ImFont* mBoldFont;
    ImFont* mItalicFont;
    ImFont* mItalicBoldFont;
    Lines mLines;
};
