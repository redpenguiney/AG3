#include "character.hpp"
#include "network/network.hpp"
#include "gameobjects/gameobject.hpp"
#include <conglomerates/basic_renderer.hpp>

std::shared_ptr<GameObject> GenerateCharacterVisualGameObject() {
    auto animShader = ShaderProgram::New("../shaders/world_vertex_animation.glsl", "../shaders/world_fragment.glsl");
    BasicRenderer::Setup().AddShader(animShader);
    auto mp = MeshCreateParams::Default();
    mp.normalizeSize = false;

    MeshImportParams mi{};
    mi.combineMeshes = true;

    auto stuff = Mesh::MultiFromFile("../models/test_anims_2.fbx", mp);
    Assert(stuff.size() == 1);
    std::vector<std::shared_ptr<GameObject>> objs;

    stuff[0].material->shader = animShader;
    GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Animation, ComponentBitIndex::Render});
    params.meshId = stuff[0].mesh->meshId;
    params.materialId = stuff[0].material->id;
    auto obj = GameObject::New(params);
    
    return obj;
}

std::shared_ptr<GameObject> GetCharacterVisualGameObject() {
    return GenerateCharacterVisualGameObject();
}

//std::shared_ptr<GameObject> GetCharacterCollider() {
    //GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Collider, ComponentBitIndex::Rigidbody });
    //params.
//}

Character::Character(std::shared_ptr<Client> client):
client(client),
graphicalObject(GetCharacterVisualGameObject()),
collider()
{
	if (client->isLocalMachine) {

	}
}

void Character::Update() {
    if (client->isLocalMachine) {
        // handle input

        // 
    }
}