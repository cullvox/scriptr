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
    auto drawCursorStart = ImGui::GetCursorScreenPos();
    auto drawCursor = drawCursorStart;
    auto contentRegion = ImGui::GetContentRegionAvail();
    auto backgroundRect = ImRect(drawCursor.x, drawCursor.y, drawCursor.x + contentRegion.x, drawCursor.y + contentRegion.y);
    auto wrapWidth = contentRegion.x;

    drawList->AddRectFilled(backgroundRect.Min, backgroundRect.Max, 0xFFe0e0e0);
    drawList->PushClipRect(backgroundRect.Min, backgroundRect.Max, false);

    if (!mDoc) return;

    // Loop through all document blocks and render each line of text in a block.
    auto blocks = mDoc->GetBlocks();
    for (auto it = blocks.begin(); it != blocks.end(); ++it)
    {
        const auto& block = (*it);
        // Draw the text blocks.
        int currentColumn = 0;
        
        //auto cursorScreenCoordinates = cursorScreenPos;
        //auto cursorFontHeight = line.size() > 0 ? line.front().fontSize : mDefaultFontSize;
        //auto cursorBaselineDifference = line.size() > 0 ? line.front().fontSize : mDefaultFontSize;

        const char* textStartFirst = block.text.data();
        const char* textStart = block.text.data();
        const char* textEnd = textStart + strlen(textStart);


        auto maxFontSize = 0.0f;
        auto maxBaseline = 0.0f;
        auto getLineData = [&](){
            // Find the maximum font size used in this line.
            maxFontSize = maxBaseline = 0.0f;
            
            for (auto lineIterator = it; lineIterator != blocks.end(); ++lineIterator) 
            {
                auto endLocation = block.text.find_first_of('\n', textStart - textStartFirst);
        
                maxFontSize = std::max(maxFontSize, block.fontSize);
                ImFont* font = GetBlockFont(block.propertyFlags);
                auto mappedDescent = ImLinearRemapClamp(0, font->FontSize, 0, block.fontSize * mDpiScaling, std::abs(font->Descent));
                maxBaseline = std::max(maxBaseline, std::abs(mappedDescent));

                if (endLocation != std::string::npos)
                    break;
            }
        };

        do
        {
            // Find the maximum font size used in this line.
            getLineData();

            const auto& additionalProperties = block.additionalProperties;

            // Determine the font that we're going to use.
            ImFont* font = GetBlockFont(block.propertyFlags);

            float widthRemaining = ImGui::CalcWrapWidthForPos(drawCursor, 0.0f);
            const char* drawEnd = font->CalcWordWrapPositionA(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining);
            
            if (textStart == drawEnd)
            {
                // Start a new line.
                drawCursor.x = drawCursorStart.x;
                drawCursor.y += maxFontSize;

                // widthRemaining = ImGui::CalcWrapWidthForPos(drawCursor, 0.0f);
                drawEnd = font->CalcWordWrapPositionA(1.0f, textStart, textEnd, wrapWidth, wrapWidth - widthRemaining);
            }

            // Calculate the text mathematics for this block.
            auto fontSizeDifference = maxFontSize - block.fontSize * mDpiScaling;
            auto textSize = font->CalcTextSizeA(block.fontSize * mDpiScaling, FLT_MAX, -1.0f, textStart, drawEnd);
            auto textRect = ImRect(drawCursor.x, drawCursor.y, drawCursor.x + textSize.x, drawCursor.y + textSize.y);

            // Consider the difference in baseline of different font sizes.
            auto baselineHeight =  ImLinearRemapClamp(0, font->FontSize, 0, block.fontSize * mDpiScaling, std::abs(font->Descent)); 
            auto baselineDifference = maxBaseline - baselineHeight;

            auto textLocation = ImVec2(textRect.Min.x, textRect.Max.y - (block.fontSize * mDpiScaling) + fontSizeDifference - baselineDifference); //  - fontSizeDifference - baselineDifference

            drawList->AddText(font, block.fontSize * mDpiScaling, textLocation, block.foregroundColor, textStart, textStart==drawEnd ? nullptr : drawEnd, 0.0f, nullptr);
            
            drawCursor = ImVec2{drawCursor.x + textSize.x, drawCursor.y + maxFontSize};
            
            drawList->AddRectFilled(textRect.Min, textRect.Max, block.backgroundColor);

            if (block.propertyFlags & RichTextPropertyFlags_Underline)
            {
                ImVec2 lineEnd = ImGui::GetItemRectMax();
                ImVec2 lineStart = lineEnd;
                lineStart.x = ImGui::GetItemRectMin().x;

                // To compute the underline position and thickness is a well educated guess here.
                // Font files often *do* define an underline position and thickness in their files but it would be hard to obtain here.

                auto thickness = std::round((maxFontSize / 24.0f) * 0.5f) * 2 + 1;
                auto underlineStart = ImVec2(textRect.Min.x, std::round(textLocation.y + block.fontSize * mDpiScaling - baselineHeight + thickness) + 1);
                auto underlineEnd = ImVec2(textRect.Max.x, std::round(textLocation.y + block.fontSize * mDpiScaling - baselineHeight + thickness) + 1);
                drawList->AddLine(underlineStart, underlineEnd, block.foregroundColor, thickness);

                

                //drawList->AddLine(lineStart, lineEnd, block.foregroundColor);

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
                    ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
            }

            if (textStart == drawEnd || drawEnd == textEnd)
            {
                // ImGui::SameLine(0.0f, 0.0f);
                // auto cursorScreenLocation= ImGui::GetCursorScreenPos();
                // cursorScreenLocation.x += textSize.x;
                // ImGui::SetCursorScreenPos(cursorScreenLocation);
                break;
            }

            textStart = drawEnd;

            while (textStart < textEnd)
            {
                const char c = *textStart;
                if (ImCharIsBlankA(c)) { textStart++; }
                else if (c == '\n') { 
                    textStart++; 

                    // Look ahead for the next new line and set the line baselines.
                    //getLineData();

                    drawCursor.x = drawCursorStart.x;
                    drawCursor.y += maxFontSize;
                
                    break; 
                }
                else { break; }
            }
        } while (true);
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