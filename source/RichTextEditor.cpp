#include "RichTextEditor.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <regex>
#include <string>


RichTextEditor::RichTextEditor(ImFont* normalFont, ImFont* boldFont, ImFont* italicFont, ImFont* italicBoldFont)
    : mNormalFont(normalFont), mBoldFont(boldFont), mItalicFont(italicFont), mItalicBoldFont(italicBoldFont)
{
}

RichTextEditor::~RichTextEditor()
{
}

// trim from start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

std::optional<ImU32> RichTextEditor::ParseHexColorCode(const std::string& code)
{
    static std::regex hexColorTest("^#(?:[0-9a-fA-F]{3,4}){1,2}$", std::regex_constants::optimize);
    if (std::regex_search(code, hexColorTest))
        return (ImU32)std::stoul(code.substr(1), nullptr, 16);
    else return std::nullopt;
}


RichTextEditor::Block RichTextEditor::ParseTextBlock(Lines& lines, int currentLine, nlohmann::json formatObject, Block* parent) {

    Block block;
    if (parent)
        block = *parent;

    block.text = formatObject.value("text", ""); // Text is the only property that is never inherited by parents.

    // Parse through all text properties.
    for (auto property : formatObject.items()) {
        
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
        else if (key != "children") 
        {
            // Parse any additional json values that could be more user-defined properties.
            RichTextPropertyValue additional;

            if (value.is_string()) {
                // Test strings for hex color codes.
                auto str = property.value().get<std::string>();
                auto color = ParseHexColorCode(str);
                if (color.has_value()) additional = color.value();
                else additional = str;
            }
            else if (value.is_number_integer())
                additional = value.get<int>();
            else if (value.is_number_float())
                additional = value.get<float>();
            else if (value.is_boolean())         
                additional = value.get<bool>();
            else 
                continue; // Skip properties that are invalid.

            block.additionalProperties.emplace(std::make_pair(key, additional));
        }
    }

    // Add the block to the line.
    if (mLines.empty()) {
        mLines.push_back(Line{});
    }

    mLines.back().push_back(block);

    // Parse all the children last that way we have all the properties are in the block.
    auto childObject = formatObject.find("children");
    if (childObject != formatObject.end()) {
        auto children = childObject.value(); 
        // Parse through child objects, could be a singluar object or an array of child objects.
        if (children.is_object())
            ParseTextBlock(lines, currentLine, children, &block);
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

void RichTextEditor::Render() {

    auto drawList = ImGui::GetWindowDrawList();

    ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();


    // Draw the background for the editor.
    auto contentRegion = ImGui::GetContentRegionAvail();
    auto backgroundRect = ImRect(cursorScreenPos.x, cursorScreenPos.y, cursorScreenPos.x + contentRegion.x, cursorScreenPos.y + contentRegion.y);

    drawList->AddRectFilled(backgroundRect.Min, backgroundRect.Max, 0xFFe0e0e0);

    for (auto& line : mLines) {


        // Find the maximum font size used in this line.
        float maxFontSize = 0.0f;
        for (auto& block : line) {
            maxFontSize = std::max(maxFontSize, block.fontSize);
        }

        for (auto& block : line) {
            const auto& additionalProperties = block.additionalProperties; 

            // Determine the font that we're going to use.
            ImFont* font = mNormalFont ? mNormalFont : ImGui::GetFont();
            float fontSize = 18.0f;

            if (block.propertyFlags & RichTextPropertyFlags_Bold)
                font = mBoldFont;
            if (block.propertyFlags & RichTextPropertyFlags_Italic)
                font = mItalicFont;
            if (block.propertyFlags & RichTextPropertyFlags_Bold && block.propertyFlags & RichTextPropertyFlags_Italic)
                font = mItalicBoldFont;

            auto itFontSize = additionalProperties.find("size"); 
            if (itFontSize != additionalProperties.end())
                fontSize = std::get<float>(itFontSize->second);
            
            // Calculate the text mathematics for this block.
            auto lineFontSizeDelta = maxFontSize - fontSize;
            auto cursorLocation = ImGui::GetCursorScreenPos();
            auto textLocation = ImVec2(cursorLocation.x, cursorLocation.y + lineFontSizeDelta);

            auto textSize = font->CalcTextSizeA(fontSize, FLT_MAX, -1.0f, block.text.data());
            auto textScreenRect = ImRect(cursorLocation.x, cursorLocation.y, cursorLocation.x + textSize.x, cursorLocation.y + textSize.y);

            // Draw the highlight/background rect.
            if ((block.backgroundColor & IM_COL32_A_MASK) != 0)
                drawList->AddRectFilled(textScreenRect.Min, textScreenRect.Max, block.backgroundColor);

            // Draw the rich text.
            drawList->AddText(font, 18.0f, textLocation, block.foregroundColor, block.text.data());

            // Set the cursors new position.
            cursorLocation = ImVec2(textScreenRect.Max.x, cursorLocation.y);
            ImGui::SetCursorScreenPos(cursorLocation);

            // Draw the cursor every half second.
            if (std::fmod(ImGui::GetTime(), 1) < 0.5f) {
                drawList->AddLine(ImVec2(cursorScreenPos.x, cursorScreenPos.y + fontSize), ImVec2(cursorScreenPos.x, cursorScreenPos.y), 0xFFb6b6b6, 2.0f);
            }
        }
    }

    

}