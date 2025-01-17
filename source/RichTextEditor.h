#pragma once

#include "imgui.h"
#include <string>


typedef int RichTextModifierFlags;  
enum ImRichTextModifierFlagBits {
    ImRichTextModifierFlags_Bold = 0x00000001,
    ImRichTextModifierFlags_Italic = 0x00000002,
    ImRichTextModifierFlags_Underline = 0x00000004,
    ImRichTextModifierFlags_Color = 0x00000008,
    ImRichTextModifierFlags_Highlight = 0x0000010,
};


namespace ImRichText {



    struct Block {
        std::string text;
        ImRichTextModifierFlagBits modifiers;
        ImVec4 color;
        ImVec4 highlight;
        std::vector<Block> children;
    };


}