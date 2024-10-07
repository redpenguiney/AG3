#include "shader_input_provider.hpp"


void TrivialFunction(Material* m, std::shared_ptr<ShaderProgram> s) {
	return;
}

ShaderInputProvider::ShaderInputProvider():
	onBindingFunc(TrivialFunction)
{

}

ShaderInputProvider::ShaderInputProvider(BindingFunction b):
	onBindingFunc(b)
{

}

void ShaderInputProvider::BindMaterialToShader(Material* material, std::shared_ptr<ShaderProgram> program)
{
	onBindingFunc(material, program);
}
