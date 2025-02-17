#pragma once

#include "imgui.h"
#include "RichTextDocument.h"

class RichTextEditor {
public:
    RichTextEditor(ImFont* normalFont=nullptr, ImFont* boldFont=nullptr, ImFont* italicFont=nullptr, ImFont* italicBoldFont=nullptr);
    ~RichTextEditor();

    void SetCursorLocation(int line, int column);
    void SetDocument(RichTextDocument& doc);
    void Render();

private:
    void HandleKeyboardInput();
    void ComputeLineAttributes(std::list<RichTextBlock>& block, float& maxFontSize, float& maxBaseline);
    void DrawCursor();
    ImFont* GetBlockFont(RichTextPropertyFlags properties);
    std::size_t UTF8CharLength(char c);

    float mDefaultFontSize = 18.0f;
    double mCursorTimeOffset = 0.0;
    int mCursorLine;
    int mCursorColumn;
    ImU32 mCursorColor;
    RichTextDocument* mDoc;

    ImFont* mNormalFont;
    ImFont* mBoldFont;
    ImFont* mItalicFont;
    ImFont* mItalicBoldFont;
};
