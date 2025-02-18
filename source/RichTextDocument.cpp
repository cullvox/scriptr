#include <algorithm>
#include <cstddef>
#include <nlohmann/json_fwd.hpp>
#include <regex>

#include <nlohmann/json.hpp>

#include "RichTextDocument.h"

RichTextDocument::RichTextDocument()
{
}

RichTextDocument::RichTextDocument(nlohmann::json json)
{
    ParseTextBlock(mBlocks, 0, json, nullptr);
}

std::size_t RichTextDocument::GetLineCount() const
{
    if (mBlocks.empty()) return 0;
    std::size_t count = 1;
    for (const auto& block : mBlocks)
    {
        count += std::count_if(block.text.begin(), block.text.end(), [](char c){
            return c == '\n';
        });
    }

    return count;
}

std::list<RichTextBlock> RichTextDocument::GetLine(int line) const
{
    if (line > GetLineCount()-1)
        return {};

    std::list<RichTextBlock> out;

    // Offset the iterator to the block containing the line.
    auto firstBlock = mBlocks.begin();
    auto currentLine = (std::size_t)0;
    auto startOffset = (std::size_t)0; 
    while(firstBlock != mBlocks.end() && currentLine < line) 
    {
        // Search for the next line.
        startOffset = 0;
        while ((startOffset = firstBlock->text.find_first_of('\n', startOffset)) != std::string::npos
                && currentLine < line)
        {
            startOffset++;
            currentLine++;
            break;
        }

        if (currentLine < line)
            firstBlock++;
    }

    // Find the final characters index in the line.
    auto endOffset = std::string::npos;
    auto foundFinal = false;
    for (auto lastBlock = firstBlock; lastBlock != mBlocks.end() && !foundFinal; lastBlock++) 
    {
        // Search for the next line.
        if (lastBlock != firstBlock)
            startOffset = 0;

        if ((endOffset = lastBlock->text.find_first_of('\n', startOffset)) != std::string::npos)
            foundFinal = true;

        if (!startOffset && !endOffset) 
            continue;

        // Add this block to the final line out.
        auto newBlock = *lastBlock;
        newBlock.text = lastBlock->text.substr(lastBlock == firstBlock ? startOffset : 0, endOffset);
        out.emplace_back(newBlock);
    }

    return out;
}

std::optional<uint32_t> RichTextDocument::ParseHexColorCode(const std::string& code)
{
    static std::regex hexColorTest("^#(?:[0-9a-fA-F]{3,4}){1,2}$", std::regex_constants::optimize);
    if (std::regex_search(code, hexColorTest))
        return static_cast<uint32_t>(std::stoul(code.substr(1).data(), nullptr, 16));
    else return std::nullopt;
}


void RichTextDocument::ParseTextBlock(std::list<RichTextBlock>& blocks, int currentLine, nlohmann::json formatObject, RichTextBlock* parent)
{
    RichTextBlock block;
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

    blocks.push_back(block);

    // Parse all the children last that way we have all the properties are in the block.
    auto childObject = formatObject.find("children");
    if (childObject != formatObject.end())
    {
        auto children = childObject.value(); 

        // Parse through child objects, could be a singluar object or an array of child objects.
        if (children.is_object())
        {
            ParseTextBlock(blocks, currentLine, children, &block);
        }
        else if (children.is_array())
        {
            // Ensure all the child values are objects and recursively process them.
            for (auto object : children) {
                if (object.is_object())
                    ParseTextBlock(blocks, currentLine, object, &block);
                else continue; // Text child objects must be objects. 
            }
        }
    }
}