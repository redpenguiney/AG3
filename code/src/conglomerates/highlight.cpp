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
		//DebugLogInfo("Creating new outline material for vertex shader ", vertexSource.c_str());

		MaterialCreateParams params;
		params.depthMask = false;
		params.depthTestFunc = DepthTestMode::Disabled;
		params.inputProvider = HighlightHandler::GeometryPassInputProviderFunc;
		params.shader = ShaderProgram::New(vertexSource.c_str(), "../shaders/highlight_fragment.glsl", true, false);
		params.allowAppendaton = false;
		params.requireUniqueTextureCollection = true;
		highlightMaterial = Material::New(params).second;
		geometryMaterials.push_back(highlightMaterial);
	}

	// create outline object 
	auto meshId = object->RawGet<RenderComponent>()->meshId;
	auto objParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	objParams.meshId = meshId;
	objParams.materialId = highlightMaterial->id;

	auto obj = GameObject::New(objParams);
	obj->name = "__HIGHLIGHT__";
	obj->RawGet<TransformComponent>()->SetParent(*object->RawGet<TransformComponent>());
	obj->RawGet<TransformComponent>()->SetPos(object->RawGet<TransformComponent>()->Position());
	obj->RawGet<TransformComponent>()->SetScl(object->RawGet<TransformComponent>()->Scale());
	obj->RawGet<TransformComponent>()->SetRot(object->RawGet<TransformComponent>()->Rotation());
	obj->Destroy();
	highlights[object] = obj;
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

	GraphicsEngine::Get().preRenderEvent->Connect([this](float dt) {
		(void)dt;

		for (auto it = highlights.begin(); it != highlights.end(); it++) {

			auto& [obj, highlight] = *it;

			if (obj.expired()) // then the object we were highlighting no longer exists
				it = highlights.erase(it);
			else {

			}
		}
	});
}

HighlightHandler::~HighlightHandler() {

}

void HighlightHandler::GeometryPassInputProviderFunc(Material*, std::shared_ptr<ShaderProgram>) {
	HighlightHandler::Get().highlightFramebuffer->Bind({0, 1});
}
