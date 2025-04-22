#include "indirect_draw_command.hpp"
#include <string>

std::string IndirectDrawCommand::ToString() const
{
    return "IDC: instances " + std::to_string(baseInstance) + " through " + std::to_string(baseInstance + instanceCount - 1);
}
