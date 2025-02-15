#pragma once

#include <list>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "RichTextDocument.h"
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

    std::optional<ImU32> ParseHexColorCode(const std::string& code);
    void HandleKeyboardInput();
    void ComputeLineAttributes(float& maxFontSize, float& maxBaseline);
    nlohmann::json CompileLinesToJSON();
    void DrawCursor();
    ImFont* GetBlockFont(RichTextPropertyFlags properties);

    float mDefaultFontSize = 18.0f;
    double mCursorTimeOffset = 0.0;
    int mCursorLine;
    int mCursorColumn;
    ImU32 mCursorColor;
    RichTextDocument mDoc;

    ImFont* mNormalFont;
    ImFont* mBoldFont;
    ImFont* mItalicFont;
    ImFont* mItalicBoldFont;
};
