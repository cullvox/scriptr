#pragma once

#include <list>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <unordered_map>
#include <string>
#include <variant>

#include "imgui.h"


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
    

    typedef std::list<Block> Document;

    std::optional<ImU32> ParseHexColorCode(const std::string& code);
    Block ParseTextBlock(Document& doc, int currentLine, nlohmann::json formatObject, Block* parent);
    size_t UTF8CharLength(char c);
    ImFont* GetBlockFont(RichTextPropertyFlags properties);
    void HandleKeyboardInput();
    void ComputeLineAttributes(Document& doc, float& maxFontSize, float& maxBaseline);
    nlohmann::json CompileLinesToJSON();
    void DrawCursor();

    float mDefaultFontSize = 18.0f;
    double mCursorTimeOffset = 0.0;
    int mCursorLine;
    int mCursorColumn;
    ImU32 mCursorColor;

    ImFont* mNormalFont;
    ImFont* mBoldFont;
    ImFont* mItalicFont;
    ImFont* mItalicBoldFont;
    Document mDoc;
};
