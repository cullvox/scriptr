#include "RichTextDocument.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>

#include "RichTextEditor.h"

RichTextEditor::RichTextEditor(ImFont* normalFont, ImFont* boldFont, ImFont* italicFont, ImFont* italicBoldFont)
    : mNormalFont(normalFont), mBoldFont(boldFont), mItalicFont(italicFont), mItalicBoldFont(italicBoldFont)
{
}

RichTextEditor::~RichTextEditor()
{
}

void RichTextEditor::SetDocument(RichTextDocument& doc)
{
    mDoc = &doc;
}

size_t RichTextEditor::UTF8CharLength(char c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}

void RichTextEditor::SetCursorLocation(int line, int column)
{
    mCursorLine = line;
    mCursorColumn = column;
    mCursorTimeOffset = 1.0 - std::fmod(ImGui::GetTime(), 1.0);
}

void RichTextEditor::HandleKeyboardInput() {
    ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused())
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		//ImGui::CaptureKeyboardFromApp(true);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;


		if (!alt && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			SetCursorLocation(mCursorLine, mCursorColumn - 1);
		if (!alt && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
            SetCursorLocation(mCursorLine, mCursorColumn + 1);
			

		//if (!IsReadOnly() && !io.InputQueueCharacters.empty())
		//{
		//	for (int i = 0; i < io.InputQueueCharacters.Size; i++)
		//	{
		//		auto c = io.InputQueueCharacters[i];
		//		if (c != 0 && (c == '\n' || c >= 32))
		//			EnterCharacter(c, shift);
		//	}
		//	io.InputQueueCharacters.resize(0);
		//}
	}
}

ImFont* RichTextEditor::GetBlockFont(RichTextPropertyFlags flags)
{
    if (flags & RichTextPropertyFlags_Bold)
        return mBoldFont;
    else if (flags & RichTextPropertyFlags_Italic)
        return mItalicFont;
    else if (flags & RichTextPropertyFlags_Bold && flags & RichTextPropertyFlags_Italic)
        return mItalicBoldFont;
    else
        return mNormalFont;
}

void RichTextEditor::ComputeLineAttributes(std::list<RichTextBlock>& line, float& maxFontSize, float& maxBaseline)
{
    // Find the maximum font size used in this line.
    maxFontSize = maxBaseline = 0.0f;
    
    for (auto& block : line) 
    {
        maxFontSize = std::max(maxFontSize, block.fontSize * mDpiScaling);

        ImFont* font = GetBlockFont(block.propertyFlags);
        auto mappedDescent = ImLinearRemapClamp(0, font->FontSize, 0, block.fontSize * mDpiScaling, std::abs(font->Descent));
        maxBaseline = std::max(maxBaseline, std::abs(mappedDescent));
    }
}

void RichTextEditor::SetDPIScaling(float dpiScaling)
{
    mDpiScaling = dpiScaling;
}

void RichTextEditor::Render() 
{
    HandleKeyboardInput();
    

    auto drawList = ImGui::GetWindowDrawList();

    // Draw the background for the editor.
    auto cursorScreenPos = ImGui::GetCursorScreenPos();
    auto contentRegion = ImGui::GetContentRegionAvail();
    auto backgroundRect = ImRect(cursorScreenPos.x, cursorScreenPos.y, cursorScreenPos.x + contentRegion.x, cursorScreenPos.y + contentRegion.y);

    drawList->AddRectFilled(backgroundRect.Min, backgroundRect.Max, 0xFFe0e0e0);
    drawList->PushClipRect(backgroundRect.Min, backgroundRect.Max, false);

    if (!mDoc) return;

    // Loop through all document blocks and render each line of text in a block.
    for (int i = 0; i < mDoc->GetLineCount(); i++)
    {
        auto line = mDoc->GetLine(i);

        // Draw the text blocks.
        int currentColumn = 0;
        
        auto cursorScreenCoordinates = cursorScreenPos;
        auto cursorFontHeight = line.size() > 0 ? line.front().fontSize : mDefaultFontSize;
        auto cursorBaselineDifference = line.size() > 0 ? line.front().fontSize : mDefaultFontSize;

        float maxFontSize = 0.0f;
        for (auto& block : line)
        {
            // Find the maximum font size used in this line.
            float maxBaseline = 0.0f;
            ComputeLineAttributes(line, maxFontSize, maxBaseline);

            const auto& additionalProperties = block.additionalProperties;

            // Determine the font that we're going to use.
            ImFont* font = GetBlockFont(block.propertyFlags);
            
            // Calculate the text mathematics for this block.
            auto fontSizeDifference = maxFontSize - block.fontSize * mDpiScaling;
            auto drawCursorLocation = ImGui::GetCursorScreenPos();
            auto textSize = font->CalcTextSizeA(block.fontSize * mDpiScaling, FLT_MAX, -1.0f, block.text.data());
            auto textRect = ImRect(drawCursorLocation.x, drawCursorLocation.y, drawCursorLocation.x + textSize.x, drawCursorLocation.y + textSize.y);

            // Consider the difference in baseline of different font sizes.
            auto baselineHeight =  ImLinearRemapClamp(0, font->FontSize, 0, block.fontSize * mDpiScaling, std::abs(font->Descent)); 
            auto baselineDifference = maxBaseline - baselineHeight;

            auto textLocation = ImVec2(textRect.Min.x, textRect.Max.y - (block.fontSize * mDpiScaling) + fontSizeDifference - baselineDifference); //  - fontSizeDifference - baselineDifference

            drawList->AddRectFilled(textRect.Min, textRect.Max, block.backgroundColor);
            drawList->AddText(font, block.fontSize * mDpiScaling, textLocation, block.foregroundColor, block.text.data());

            if (block.propertyFlags & RichTextPropertyFlags_Underline)
            {
                // To compute the underline position and thickness is a well educated guess here.
                // Font files often *do* define an underline position and thickness in their files but it would be hard to obtain here.
 
                auto thickness = std::round((maxFontSize / 24.0f) * 0.5f) * 2 + 1;
                auto underlineStart = ImVec2(textRect.Min.x, std::round(textLocation.y + block.fontSize * mDpiScaling - baselineHeight + thickness) + 1);
                auto underlineEnd = ImVec2(textRect.Max.x, std::round(textLocation.y + block.fontSize * mDpiScaling - baselineHeight + thickness) + 1);
                drawList->AddLine(underlineStart, underlineEnd, block.foregroundColor, thickness);
            }

            // Set the cursors new position.
            drawCursorLocation = ImVec2(textRect.Max.x, drawCursorLocation.y);
            ImGui::SetCursorScreenPos(drawCursorLocation);
        }

        auto cursorScreenLocation= ImGui::GetCursorScreenPos();
        cursorScreenLocation.x = cursorScreenPos.x;
        cursorScreenLocation.y += maxFontSize;
        ImGui::SetCursorScreenPos(cursorScreenLocation);
    }

    drawList->PopClipRect();

}

void RichTextEditor::DrawCursor()
{

    // Draw the cursor every half second.
    if (std::fmod(ImGui::GetTime() + mCursorTimeOffset, 1) > 0.5f)
        return;

//    int currentLine = 0;
//    for (auto& line : mLines)
//    {
//        if (mCursorLine == currentLine) 
//        {
//
//            if (mCursorColumn == 0) {
//                cursorBaselineDifference = line.front().
//            }
//
//            if (mCursorLine == currentLine && mCursorColumn > currentColumn)
//            {
//                for (int i = 0; i < block.text.size(); ) 
//                {
//                    if (currentColumn >= mCursorColumn)
//                        break;
//
//                    auto characterBytes = UTF8CharLength(block.text[i]);
//                    auto characterSize = font->CalcTextSizeA(block.fontSize, FLT_MAX, -1.0f, block.text.data() + i, block.text.data() + i + characterBytes);
//                    i += characterBytes;
//
//                    cursorScreenCoordinates.x += characterSize.x;
//                    cursorScreenCoordinates.y = drawCursorLocation.y;
//                    cursorFontHeight = block.fontSize;
//                    cursorBaselineDifference = baselineDifference;
//
//                    currentColumn++;
//                }
//            }
//
//            auto cursorLineDifference = maxFontSize - cursorFontHeight;
//            auto lineStart = ImVec2(std::round(cursorScreenCoordinates.x), cursorScreenCoordinates.y + cursorLineDifference - cursorBaselineDifference);
//            auto lineEnd = ImVec2(std::round(cursorScreenCoordinates.x), cursorScreenCoordinates.y + cursorLineDifference + cursorFontHeight - cursorBaselineDifference);
//
//            
//
//            drawList->AddLine(lineStart, lineEnd, 0xFF1b1b1b, 1.f);
//            //drawList->AddRectFilled(lineStart, lineEnd, 0xFF1b1b1b);
//        }
//    }
}