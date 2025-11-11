#include "character.hpp"
#include "network/network.hpp"
#include "gameobjects/gameobject.hpp"
#include <conglomerates/basic_renderer.hpp>

std::shared_ptr<GameObject> GenerateCharacterGameObject() {
    auto animShader = ShaderProgram::New("../shaders/world_vertex_animation.glsl", "../shaders/world_fragment.glsl");
    BasicRenderer::Setup().AddShader(animShader);
    auto mp = MeshCreateParams::Default();
    mp.normalizeSize = false;

    auto stuff = Mesh::MultiFromFile("../models/test_anims_2.fbx", mp);
    std::vector<std::shared_ptr<GameObject>> objs;
    for (auto& ret : stuff) {
        ret.material->shader = animShader;
        GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Animation, ComponentBitIndex::Render });
        params.meshId = ret.mesh->meshId;
        params.materialId = ret.material->id;
        auto obj = GameObject::New(params);
        objs.push_back(obj);
        if (!objs.empty()) {
            
        }
    }
}

std::shared_ptr<GameObject> GetCharacterGameObject() {
   

}

Character::Character(std::shared_ptr<Client> client):
client(client),
gameobject(GetCharacterGameObject())
{
	if (client->isLocalMachine) {

	}
}