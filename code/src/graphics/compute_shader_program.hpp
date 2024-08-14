#pragma once
#include "base_shader_program.hpp"
#include <memory>

class ComputeShaderProgram: public BaseShaderProgram {
public:
    // Returns id of generated program.
    // additionalIncludedFiles is an optional vector of filepaths to files included by the compute shaer.
    static std::shared_ptr<ComputeShaderProgram> New(const char* computePath, const std::vector<const char*>& additionalIncludedFiles = {});

private:
    ComputeShaderProgram();
};