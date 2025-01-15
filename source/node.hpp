#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <Poco/UUID.h>

enum class NodeAttachmentType {
    Flow
};

enum class NodeType {
    Script,
    Option,
};

class Node {
public:
    void LoadFromJSON();
    void AddInputAttachment(std::string_view name, int attachmentID, NodeAttachmentType type = NodeAttachmentType::Flow);
    void AddOutputAttachment(std::string_view name, int attachmentID, NodeAttachmentType type = NodeAttachmentType::Flow);
    void RemoveAttachemnt(int attachmentID);
    void Link(int localAttachment, int toAttachment);
    void SetName(std::string_view name);
    void SetUUID(Poco::UUID uuid);
    Poco::UUID GetUUID();
    void Draw();

private:
    struct Attachment {
        int32_t id;
        std::string name;
        NodeAttachmentType type;
    };

    std::string _name;
    std::vector<Attachment> _inputAttachments;
    std::vector<Attachment> _outputAttachments;
};