#pragma once
#include <memory>

class ShaderProgram;
class Material; 

// Each material has it's own ShaderInputProvider.
// These are used set values to uniforms and bind other stuff when a material is bound.
class ShaderInputProvider {
public:
	// sorry it's a function ptr, but neither lambdas nor std::function can be compared
	typedef void (*BindingFunction)(Material* material, std::shared_ptr<ShaderProgram> program);

	// Default constructor sets onBindingFunc to a trivial function. 
	ShaderInputProvider();
	ShaderInputProvider(BindingFunction b);

	void BindMaterialToShader(Material* material, std::shared_ptr<ShaderProgram> program);

	// Runs BEFORE stuff like textures and stuff are bound at material usetime, so changes to that stuff will be overridden.
	BindingFunction onBindingFunc;

	// TODO: version that runs after
};