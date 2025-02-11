#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <regex>
#include <string>

#include "RichTextEditor.h"

RichTextEditor::RichTextEditor(ImFont* normalFont, ImFont* boldFont, ImFont* italicFont, ImFont* italicBoldFont)
    : mNormalFont(normalFont), mBoldFont(boldFont), mItalicFont(italicFont), mItalicBoldFont(italicBoldFont)
{
}

RichTextEditor::~RichTextEditor()
{
}

std::optional<ImU32> RichTextEditor::ParseHexColorCode(const std::string& code)
{
    static std::regex hexColorTest("^#(?:[0-9a-fA-F]{3,4}){1,2}$", std::regex_constants::optimize);
    if (std::regex_search(code, hexColorTest))
        return (ImU32)std::stoul(code.substr(1), nullptr, 16);
    else return std::nullopt;
}


RichTextEditor::Block RichTextEditor::ParseTextBlock(Lines& lines, int currentLine, nlohmann::json formatObject, Block* parent)
{

    Block block;
    if (parent)
        block = *parent;

    block.text = formatObject.value("text", ""); // Text is the only property that is never inherited by parents.

    // Parse through all text properties.
    for (auto property : formatObject.items()) 
    {
        auto& key = property.key();
        auto& value = property.value();

        // Parse through the default values that rich text always have.
        if (key == "bold" && value.is_boolean()) 
        { 
            if (value)  block.propertyFlags |= RichTextPropertyFlags_Bold;
            else        block.propertyFlags &= ~RichTextPropertyFlags_Bold;
        }
        else if (key == "italic" && value.is_boolean())
        {
            if (value)  block.propertyFlags |= RichTextPropertyFlags_Italic;
            else        block.propertyFlags &= ~RichTextPropertyFlags_Italic;
        }
        else if (key == "underline" && value.is_boolean()) 
        {
            if (value)  block.propertyFlags |= RichTextPropertyFlags_Underline;
            else        block.propertyFlags &= ~RichTextPropertyFlags_Underline;
        }
        else if (key == "color" && value.is_string())
        {
            auto color = ParseHexColorCode(value);
            if (color)
                block.foregroundColor = color.value();
        }
        else if (key == "highlight" && value.is_string())
        {
            auto color = ParseHexColorCode(value);
            if (color)
                block.backgroundColor = color.value();
        }
        else if (key == "size" && value.is_number_float())
        {
            block.fontSize = value.get<float>();
        }
        else if (key != "children") 
        {
            // Parse any additional json values that could be more user-defined properties.
            RichTextPropertyValue additional;

            if (value.is_string()) {
                // Test strings for hex color codes.
                auto str = value.get<std::string>();
                auto color = ParseHexColorCode(str);

                if (color.has_value())
                    additional = color.value();
                else
                    additional = str;
            }
            else if (value.is_number_integer())
                additional = value.get<int>();
            else if (value.is_number_float())
                additional = value.get<float>();
            else if (value.is_boolean())
                additional = value.get<bool>();
            else 
                continue; // Skip properties that are invalid.

            block.additionalProperties[key] = additional;
        }
    }

    // Add the block to the line.
    if (mLines.empty()) 
    {
        mLines.push_back(Line{});
    }

    mLines.back().push_back(block);

    // Parse all the children last that way we have all the properties are in the block.
    auto childObject = formatObject.find("children");
    if (childObject != formatObject.end())
    {
        auto children = childObject.value(); 
        
        // Parse through child objects, could be a singluar object or an array of child objects.
        if (children.is_object())
        {
            ParseTextBlock(lines, currentLine, children, &block);
        }
        else if (children.is_array())
        {
            // Ensure all the child values are objects and recursively process them.
            for (auto object : children) {
                if (object.is_object())
                    ParseTextBlock(lines, currentLine, object, &block);
                else continue; // Text child objects must be objects. 
            }
        }
    }    

    return block;
}


void RichTextEditor::SetText(nlohmann::json formatObject)
{
    ParseTextBlock(mLines, 0, formatObject, nullptr);
}


nlohmann::json RichTextEditor::GetText()
{
    return {};
}

std::string RichTextEditor::GetTextUnformatted()
{
    return {};
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

void RichTextEditor::Render() 
{
    HandleKeyboardInput();

    auto drawList = ImGui::GetWindowDrawList();

    // Draw the background for the editor.no I know 
    auto cursorScreenPos = ImGui::GetCursorScreenPos();
    auto contentRegion = ImGui::GetContentRegionAvail();
    auto backgroundRect = ImRect(cursorScreenPos.x, cursorScreenPos.y, cursorScreenPos.x + contentRegion.x, cursorScreenPos.y + contentRegion.y);

    drawList->AddRectFilled(backgroundRect.Min, backgroundRect.Max, 0xFFe0e0e0);
    drawList->PushClipRect(backgroundRect.Min, backgroundRect.Max, false);

    int currentLine = 0;
    for (auto& line : mLines)
    {

        // Find the maximum font size used in this line.
        float maxFontSize = 0.0f;
        float maxBaseline = 0.0f;
        for (auto& block : line) 
        {
            maxFontSize = std::max(maxFontSize, block.fontSize);

            ImFont* font = GetBlockFont(block.propertyFlags);
            auto mappedDescent = ImLinearRemapClamp(0, font->FontSize, 0, block.fontSize, std::abs(font->Descent));
            maxBaseline = std::max(maxBaseline, std::abs(mappedDescent));
        }

        // Draw the text blocks.
        int currentColumn = 0;
        
        auto cursorScreenCoordinates = cursorScreenPos;
        auto cursorFontHeight = line.size() > 0 ? line.front().fontSize : mDefaultFontSize;
        auto cursorBaselineDifference = line.size() > 0 ? line.front().fontSize : mDefaultFontSize;
        
        for (auto& block : line) 
        {
            const auto& additionalProperties = block.additionalProperties;

            // Determine the font that we're going to use.
            ImFont* font = GetBlockFont(block.propertyFlags);
            
            // Calculate the text mathematics for this block.
            auto fontSizeDifference = maxFontSize - block.fontSize;
            auto drawCursorLocation = ImGui::GetCursorScreenPos();
            auto textSize = font->CalcTextSizeA(block.fontSize, FLT_MAX, -1.0f, block.text.data());
            auto textScreenRect = ImRect(drawCursorLocation.x, drawCursorLocation.y, drawCursorLocation.x + textSize.x, drawCursorLocation.y + textSize.y);

            // Consider the difference in baseline of different font sizes.
            auto baselineHeight = ImLinearRemapClamp(0, font->FontSize, 0, block.fontSize, std::abs(font->Descent));
            auto baselineDifference = maxBaseline - baselineHeight;

            auto textLocation = ImVec2(drawCursorLocation.x, drawCursorLocation.y + fontSizeDifference - baselineDifference);

            if ((block.backgroundColor & IM_COL32_A_MASK) != 0)
                drawList->AddRectFilled(textScreenRect.Min, textScreenRect.Max, block.backgroundColor);

            drawList->AddText(font, block.fontSize, textLocation, block.foregroundColor, block.text.data());

            if (block.propertyFlags & RichTextPropertyFlags_Underline)
            {
                auto underlineStart = ImVec2(textScreenRect.Min.x, textScreenRect.Min.y);
                auto underlineEnd = ImVec2(textScreenRect.Max.x, textScreenRect.Max.y + fontSizeDifference);
                drawList->AddLine(underlineStart, underlineEnd, block.foregroundColor);
            }

            // Set the cursors new position.
            drawCursorLocation = ImVec2(textScreenRect.Max.x, drawCursorLocation.y);
            ImGui::SetCursorScreenPos(drawCursorLocation);
        }

        currentLine++;
    }

    drawList->PopClipRect();

}

void RichTextEditor::DrawCursor()
{
    

    if (mCursorLine == currentLine) 
        {
            // Draw the cursor every half second.
            if (std::fmod(ImGui::GetTime() + mCursorTimeOffset, 1) < 0.5f)
            {   
/*                 if (mCursorColumn == 0) {
                    cursorBaselineDifference = line.front().
                }

                if (mCursorLine == currentLine && mCursorColumn > currentColumn)
                {
                    for (int i = 0; i < block.text.size(); ) 
                    {
                        if (currentColumn >= mCursorColumn)
                            break;

                        auto characterBytes = UTF8CharLength(block.text[i]);
                        auto characterSize = font->CalcTextSizeA(block.fontSize, FLT_MAX, -1.0f, block.text.data() + i, block.text.data() + i + characterBytes);
                        i += characterBytes;

                        cursorScreenCoordinates.x += characterSize.x;
                        cursorScreenCoordinates.y = drawCursorLocation.y;
                        cursorFontHeight = block.fontSize;
                        cursorBaselineDifference = baselineDifference;

                        currentColumn++;
                    }
                } */

                auto cursorLineDifference = maxFontSize - cursorFontHeight;
                auto lineStart = ImVec2(std::round(cursorScreenCoordinates.x), cursorScreenCoordinates.y + cursorLineDifference - cursorBaselineDifference);
                auto lineEnd = ImVec2(std::round(cursorScreenCoordinates.x), cursorScreenCoordinates.y + cursorLineDifference + cursorFontHeight - cursorBaselineDifference);

                

                drawList->AddLine(lineStart, lineEnd, 0xFF1b1b1b, 1.f);
                //drawList->AddRectFilled(lineStart, lineEnd, 0xFF1b1b1b);
            }
        }

}