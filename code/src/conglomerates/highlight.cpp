#include "highlight.hpp"
#include "graphics/gengine.hpp"
#include "gameobjects/gameobject.hpp"

void HighlightHandler::SetTargetFramebuffer(Framebuffer* target) {
	presentationTarget = target;
}

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
		params.inputProvider = ShaderInputProvider(HighlightHandler::GeometryPassInputProviderFunc);
		params.shader = ShaderProgram::New(vertexSource.c_str(), "../shaders/highlight_fragment.glsl", true, false);
		params.blendingEnabled = false;
		params.allowAppendaton = false;
		params.requireUniqueTextureCollection = true;
		params.drawOrder = GEOMETRY_DRAW_ORDER;
		highlightMaterial = Material::New(params).second;
		geometryMaterials.push_back(highlightMaterial);
	}

	// create outline object 
	auto meshId = object->RawGet<RenderComponent>()->meshId;
	auto objParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	objParams.meshId = meshId;
	objParams.materialId = highlightMaterial->id;

	auto obj = GameObject::New(objParams);
	//obj->name = "__HIGHLIGHT__";
	obj->RawGet<TransformComponent>()->SetParent(*object->RawGet<TransformComponent>());
	obj->RawGet<TransformComponent>()->SetPos(object->RawGet<TransformComponent>()->Position());
	obj->RawGet<TransformComponent>()->SetScl(object->RawGet<TransformComponent>()->Scale());
	obj->RawGet<TransformComponent>()->SetRot(object->RawGet<TransformComponent>()->Rotation());
	obj->Destroy();
	highlights[object] = obj;
}

void HighlightHandler::RemoveHightlight(std::shared_ptr<GameObject> object){
	Assert(highlights.contains(object));
	highlights.erase(object);
}

template <float stepSize, bool writeSecond, bool clear>
std::shared_ptr<Material> HighlightHandler::MakeJumpFloodPass(int passIndex) {
	MaterialCreateParams params{
		.depthMask = false,
		.requireUniqueTextureCollection = true,
		.allowAppendaton = false,
		.inputProvider = ShaderInputProvider(HighlightHandler::JumpFloodPassInputProviderFunc<stepSize, writeSecond, clear>),
		.depthTestFunc = DepthTestMode::Disabled,
		.drawOrder = HighlightHandler::JUMP0_DRAW_ORDER + passIndex,
	};
	params.textureParams = {};
	params.shader = ShaderProgram::New("../shaders/jump_flood_vertex.glsl", "../shaders/jump_flood_fragment.glsl");
	
	auto mat = Material::New(params);

	GameobjectCreateParams goParams({ ComponentBitIndex::Transform, ComponentBitIndex::RenderNoFO });
	goParams.materialId = mat.second->id;
	goParams.meshId = Mesh::ScreenQuad()->meshId;
	GameObject::New(goParams);

	return mat.second;
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

			highlightFramebuffer.emplace(newSize.x, newSize.y, std::vector{ colorTextureParams, geometryTextureParams }, false);
			highlightFramebuffer2.emplace(newSize.x, newSize.y, std::vector{ colorTextureParams, }, false);
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

	preJumpflood = Material::New(MaterialCreateParams{
		.inputProvider = ShaderInputProvider(PreJumpFloodInputProviderFunc),
		.drawOrder = GEOMETRY_DRAW_ORDER-1
		}).second;
	preJumpflood->abstract = true;
	auto goparams = GameobjectCreateParams ({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
	goparams.materialId = preJumpflood->id;
	goparams.meshId = Mesh::ScreenQuad()->meshId;
	GameObject::New(goparams);

	// create jump flood passes
	jumpFloodMaterials.push_back(MakeJumpFloodPass<16.0f, true, true>(0));
	jumpFloodMaterials.push_back(MakeJumpFloodPass<8.0f, false>(1));
	jumpFloodMaterials.push_back(MakeJumpFloodPass<4.0f, true>(2));
	jumpFloodMaterials.push_back(MakeJumpFloodPass<2.0f, false>(3));
	jumpFloodMaterials.push_back(MakeJumpFloodPass<1.0f, true>(4));
	jumpFloodMaterials.push_back(MakeJumpFloodPass<1.0f, false>(5)); 
	// NOTE: last pass should be false (write output to first framebuffer). presentation pass takes final output from there.

	// create presentation pass
	MaterialCreateParams presentationParams = {
		.shader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/highlight_present_fragment.glsl"),
		.requireUniqueTextureCollection = false,
		.allowAppendaton = false,
		.inputProvider = ShaderInputProvider(HighlightHandler::PresentationPassInputProvider),
		.drawOrder = PRESENTATION_DRAW_ORDER,
		
	};

	presentationMaterial = Material::New(presentationParams).second;

	GameobjectCreateParams goParams({ ComponentBitIndex::Transform, ComponentBitIndex::RenderNoFO });
	goParams.materialId = presentationMaterial->id;
	goParams.meshId = Mesh::ScreenQuad()->meshId;

	auto presentationQuad = GameObject::New(goParams);
}

HighlightHandler::~HighlightHandler() {

}

void HighlightHandler::GeometryPassInputProviderFunc(Material*, std::shared_ptr<ShaderProgram> shader) {
	HighlightHandler::Get().highlightFramebuffer->Bind({0, 1});
	shader->Uniform("screenSize", glm::vec2(HighlightHandler::Get().highlightFramebuffer->width, HighlightHandler::Get().highlightFramebuffer->height));
}

void HighlightHandler::PreJumpFloodInputProviderFunc(Material*, std::shared_ptr<ShaderProgram>) {
	HighlightHandler::Get().highlightFramebuffer->Bind();
	HighlightHandler::Get().highlightFramebuffer->Clear({ {0, 0, 0, 0}, {0, 0, 0, 0} });
}

template<float stepSize, bool useSecond, bool clear>
void HighlightHandler::JumpFloodPassInputProviderFunc(Material*, std::shared_ptr<ShaderProgram> shader) {
	auto& writeFramebuffer = useSecond ? HighlightHandler::Get().highlightFramebuffer2 : HighlightHandler::Get().highlightFramebuffer;
	auto& readFramebuffer = useSecond ? HighlightHandler::Get().highlightFramebuffer : HighlightHandler::Get().highlightFramebuffer2;
	
	shader->Uniform("stepSize", stepSize);
	shader->Uniform("screenSize", glm::vec2(HighlightHandler::Get().highlightFramebuffer->width, HighlightHandler::Get().highlightFramebuffer->height));

	if (clear) {
		writeFramebuffer->Bind({ 0 });
		if (useSecond) {
			writeFramebuffer->Clear({ {0, 0, 0, 0 }, });
		}
		else {
			writeFramebuffer->Clear({ {0, 0, 0, 0}, });
		}
	}
	else {
		writeFramebuffer->Bind({ 0 });
	}
	readFramebuffer->textureAttachments[0].Use();	
}

void HighlightHandler::PresentationPassInputProvider(Material*, std::shared_ptr<ShaderProgram> shader) {
	if (HighlightHandler::Get().presentationTarget) {
		HighlightHandler::Get().presentationTarget->Bind();
	}
	else {
		Framebuffer::Unbind();
	}
	HighlightHandler::Get().highlightFramebuffer->textureAttachments[0].Use();
}