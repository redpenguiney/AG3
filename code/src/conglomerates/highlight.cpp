#include "highlight.hpp"
#include "graphics/gengine.hpp"
#include "gameobjects/gameobject.hpp"

HighlightHandler& HighlightHandler::Get() {
	static HighlightHandler handler;
	return handler;
}

void HighlightHandler::AddHighlight(std::shared_ptr<GameObject> object, float outlineWidth, glm::vec3 outlineColor) {
	auto material = Material::Get(object->RawGet<RenderComponent>()->materialId);
	auto vertexSource = material->shader->GetVertexSourcePath();

	std::shared_ptr<Material> highlightMaterial = nullptr;
	for (auto& mat : geometryMaterials) {
		if (mat->shader->GetVertexSourcePath() == vertexSource) {
			highlightMaterial = mat;
			break;
		}
	}

	if (!highlightMaterial) {
		MaterialCreateParams params;
		params.depthMask = false;
		params.depthTestFunc = DepthTestMode::Disabled;
		params.inputProvider = HighlightHandler::GeometryPassInputProviderFunc;
		params.shader = ShaderProgram::New(vertexSource.c_str(), "../shaders/highlight_fragment.glsl", true, false);
	}
}

HighlightHandler::HighlightHandler() {
	

	auto updateFramebuffer = [this](glm::uvec2 _, glm::uvec2 newSize) {
		(void)_;

		if (!highlightFramebuffer || highlightFramebuffer->width != newSize.x || highlightFramebuffer->width != newSize.y) {
			// color
			TextureCreateParams colorTextureParams({}, Texture::ColorMap);
			colorTextureParams.filteringBehaviour = Texture::LinearTextureFiltering;
			colorTextureParams.mipmapBehaviour = Texture::NoMipmaps;
			colorTextureParams.format = Texture::RGBA_16Float;
			colorTextureParams.wrappingBehaviour = Texture::WrapClampToEdge;

			// geometry mask
			TextureCreateParams geometryTextureParams({}, Texture::ColorMap);
			geometryTextureParams.filteringBehaviour = Texture::LinearTextureFiltering;
			geometryTextureParams.mipmapBehaviour = Texture::NoMipmaps;
			geometryTextureParams.format = Texture::Grayscale_8Bit;
			geometryTextureParams.wrappingBehaviour = Texture::WrapClampToEdge;

			highlightFramebuffer.emplace(newSize.x, newSize.y, std::vector{ colorTextureParams, geometryTextureParams }, true);
		}

		};
	updateFramebuffer({ 0, 0 }, { GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height });
	GraphicsEngine::Get().window.onWindowResize->Connect(updateFramebuffer);
}

HighlightHandler::~HighlightHandler() {

}
