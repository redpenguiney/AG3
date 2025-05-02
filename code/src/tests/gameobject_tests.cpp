#include "gameobject_tests.hpp"
#include <audio/sound.hpp>
#include <gameobjects/gameobject.hpp>
#include <physics/pengine.hpp>
#include <conglomerates/gui.hpp>


#include "noise/noise.h"

void TestSkybox() {
    auto params = MaterialCreateParams{

        .textureParams = {
            TextureCreateParams({TextureSource {"../textures/sky/top.png"}, TextureSource {"../textures/sky/top.png"}, TextureSource {"../textures/sky/top.png"}, TextureSource {"../textures/sky/top.png"}, TextureSource {"../textures/sky/top.png"}, TextureSource {"../textures/sky/top.png"}}, Texture::ColorMap),
    },
    .type = Texture::TextureType::TextureCubemap,
    .shader = GraphicsEngine::Get().skyboxMaterial->shader,

    };
    GraphicsEngine::Get().skyboxMaterial = Material::New(params).second;
    //GraphicsEngine::Get().skyboxMaterial->textures = TextureCollection::FindCollection(params).first;
}

std::shared_ptr<Mesh> CubeMesh() {
    auto makeMparams = MeshCreateParams{ .textureZ = -1.0, .opacity = 1, .expectedCount = 16384 };
    
    Assert(makeMparams.meshVertexFormat == std::nullopt);
    static auto m = Mesh::MultiFromFile("../models/rainbowcube.obj", makeMparams).at(0).mesh;
    //Assert(m->vertexFormat.primitiveType == GL_POINTS);

    //DebugLogInfo("CUBE HAS MESH ID ", m->meshId);

    return m;
}

std::shared_ptr<Mesh> SphereMesh() {
    auto makeMparams = MeshCreateParams{ .textureZ = -1.0, .opacity = 1, .expectedCount = 16384 };
    auto m = Mesh::MultiFromFile("../models/icosphere.obj", makeMparams).at(0).mesh;

    //DebugLogInfo("CUBE HAS MESH ID ", m->meshId);

    return m;
}

std::pair<float, std::shared_ptr<Material>> GrassMaterial() {
    static auto grass = Material::New(MaterialCreateParams{
        .textureParams = {
             TextureCreateParams({ TextureSource{"../textures/grass.png"},}, Texture::ColorMap),
             TextureCreateParams({TextureSource {"../textures/crate_specular.png"}}, Texture::SpecularMap)
        },
        .type = Texture::Texture2D }
    );

    return grass;
}

std::pair<float, std::shared_ptr<Material>> BrickMaterial() {
    return Material::New(MaterialCreateParams{ .textureParams = {
        TextureCreateParams({ TextureSource{"../textures/ambientcg_bricks085/color.jpg"},}, Texture::ColorMap),
        TextureCreateParams({TextureSource{"../textures/ambientcg_bricks085/roughness.jpg"},}, Texture::SpecularMap),
        TextureCreateParams({TextureSource{"../textures/ambientcg_bricks085/normal_gl.jpg"},}, Texture::NormalMap),
        //TextureCreateParams({TextureSource {"../textures/ambientcg_bricks085/displacement.jpg"},}, Texture::DisplacementMap) // TODO assert format grayscale somehow
   }, .type = Texture::Texture2D });
}

std::pair<float, std::shared_ptr<Material>> ArialFont(int size)
{
    auto ttfParams = TextureCreateParams({ TextureSource{"../fonts/arial.ttf"}, }, Texture::FontMap);
    ttfParams.fontHeight = size;
    ttfParams.format = Texture::Grayscale_8Bit;
    //ttfParams.mipmapBehaviour = Texture::NoMipmaps;
    auto b = Material::New(MaterialCreateParams({ ttfParams }, Texture::Texture2D, GraphicsEngine::Get().defaultGuiMaterial->shader, nullptr, false));
    b.second->ignorePostProc = true;
    b.second->depthMaskEnabled = false;
    b.second->depthTestFunc = DepthTestMode::Disabled;
    return b;
}

void MakeFPSTracker()
{
    auto font = ArialFont(12);

    auto ui = new Gui(true,
        std::nullopt,
        std::make_optional(font)
        //Gui::BillboardGuiInfo({.scaleWithDistance = false, .rotation = std::nullopt, .followObject = goWeakPtr}), 
        //GraphicsEngine::Get().defaultBillboardGuiShaderProgram);
    );
    ui->scaleSize = { 0.8, 0.15 };
    ui->guiScaleMode = Gui::ScaleXX;
    ui->offsetPos = { 0.0, 0.0 };
    ui->scalePos = { 0.5, 0.5 };
    ui->anchorPoint = { 0.0, 0.0 };
    ui->rgba.a = 0.0;
    ui->GetTextInfo().rgba = { 1.0, 1.0, 1.0, 1.0 };
    ui->GetTextInfo().topMargin = 0;
    ui->GetTextInfo().bottomMargin = 0;
    ui->GetTextInfo().lineHeight = 1.0;
    ui->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
    ui->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
    ui->UpdateGuiGraphics();
    ui->UpdateGuiTransform();

    GraphicsEngine::Get().preRenderEvent->Connect([ui](double t) {
        ui->GetTextInfo().text = "frame: " + std::to_string(t * 1000) + "ms";
        ui->UpdateGuiText();
    });
}

void TestCubeArray(glm::uvec3 stride, glm::uvec3 start, glm::uvec3 dim, bool physics, glm::vec3 size)
{
    auto m = CubeMesh();

    int nObjs = 0;
    for (int x = 0; x < dim.x; x++) {
        for (int y = 0; y < dim.y; y++) {
            for (int z = 0; z < dim.z; z++) {
                GameobjectCreateParams params = physics ? 
                    GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider , ComponentBitIndex::Rigidbody }) : 
                    GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
                params.meshId = m->meshId;
                Assert(params.materialId == 0);
                //params.materialId = brickMaterial->id;
                auto g = GameObject::New(params);
                g->Get<TransformComponent>()->SetPos(glm::dvec3(start + stride * glm::uvec3(x, y, z)));
                g->Get<ColliderComponent>()->elasticity = 0.3;
                g->Get<ColliderComponent>()->friction = 1.0;
                // g->rigidbodyComponent->angularDrag = 1.0;
                // g->rigidbodyComponent->linearDrag = 1.0;
                // g->rigidbodyComponent->velocity = {1.0, 0.0, 1.0};
                // g->rigidbodyComponent->angularVelocity = {0.20, 1.6, 1.0};
                g->Get<TransformComponent>()->SetScl(size);
                g->Get<RenderComponent>()->SetColor(glm::vec4(1, 1, 1, 1));
                g->Get<RenderComponent>()->SetTextureZ(-1);
                g->name = std::string("Gameobject #") + std::to_string(nObjs);
                ////goWeakPtr = g;
                //nObjs++;

                if (physics) {
                    g->RawGet<RigidbodyComponent>()->SetMass(1.0, *g->RawGet<TransformComponent>());
                }

    //            g->rigidbodyComponent->SetMass(5.0, *g->transformComponent);

    //            // for (float i = 0; i < 720; i += 15) {
    //            //     // DebugLogInfo(i, " degrees is ", glm::to_string(glm::quat(glm::vec3(glm::radians(i), glm::radians(0.0), glm::radians(0.0)))));
    //            //     g->Get<TransformComponent>().SetRot(glm::quat(glm::vec3(glm::radians(i), glm::radians(0.0), glm::radians(0.0))));
    //            //     DebugLogInfo("Rotation ", i, " inverse moi around x-axis is ", g->rigidbodyComponent->InverseMomentOfInertiaAroundAxis(*g->transformComponent, {1.0, 0.0, 0.0}));
    //            // }

    //            // GameobjectCreateParams axisMarkerParams({ComponentBitIndex::Transform, ComponentBitIndex::Render});
    //            // axisMarkerParams.meshId= m->meshId;

    //            // auto xAxis = CR.NewGameObject(axisMarkerParams);
    //            // xAxis->Get<TransformComponent>().SetParent(*g->transformComponent);
    //            // xAxis->Get<TransformComponent>().SetPos(g->Get<TransformComponent>().Position() + glm::dvec3(1.0, 0.0, 0.0));
    //            // xAxis->Get<TransformComponent>().SetScl({1.0, 0.1, 0.1});
    //            // xAxis->renderComponent->SetColor({1.0, 0.0, 0.0, 1.0});

    //            // auto yAxis = CR.NewGameObject(axisMarkerParams);
    //            // yAxis->Get<TransformComponent>().SetParent(*g->transformComponent);
    //            // yAxis->Get<TransformComponent>().SetPos(g->Get<TransformComponent>().Position() + glm::dvec3(0.0, 1.0, 0.0));
    //            // yAxis->Get<TransformComponent>().SetScl({0.1, 1.0, 0.1});
    //            // yAxis->renderComponent->SetColor({0.0, 1.0, 0.0, 1.0});

    //            // auto zAxis = CR.NewGameObject(axisMarkerParams);
    //            // zAxis->Get<TransformComponent>().SetParent(*g->transformComponent);
    //            // zAxis->Get<TransformComponent>().SetPos(g->Get<TransformComponent>().Position() + glm::dvec3(0.0, 0.0, 1.0));
    //            // zAxis->Get<TransformComponent>().SetScl({0.1, 0.1, 1.0});
    //            // zAxis->renderComponent->SetColor({0.0, 0.0, 1.0, 1.0});

    //            // g->Get<TransformComponent>().SetRot(glm::quat(glm::vec3(glm::radians(360.0), glm::radians(0.0), glm::radians(0.0))));

            }
        }
    }
}

void TestSphere(int x, int y, int z, bool physics)
{
    GameobjectCreateParams params = physics ? GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider , ComponentBitIndex::Rigidbody }) : GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
    params.meshId = SphereMesh()->meshId;
    //params.materialId = brickMaterial->id;
    auto g = GameObject::New(params);
    g->Get<TransformComponent>()->SetPos({ x, y, z });
    g->Get<ColliderComponent>()->elasticity = 0.3;
    g->Get<ColliderComponent>()->friction = 1.0;
    // g->rigidbodyComponent->angularDrag = 1.0;
    // g->rigidbodyComponent->linearDrag = 1.0;
    // g->rigidbodyComponent->velocity = {1.0, 0.0, 1.0};
    // g->rigidbodyComponent->angularVelocity = {0.20, 1.6, 1.0};
    g->Get<TransformComponent>()->SetScl(glm::dvec3(1.0, 1.0, 1.0));
    g->Get<RenderComponent>()->SetColor(glm::vec4(1, 1, 1, 1));
    g->Get<RenderComponent>()->SetTextureZ(-1);
    g->name = std::string("Sphereobject") ;
    ////goWeakPtr = g;
    //nObjs++;

    if (physics) {
        g->RawGet<RigidbodyComponent>()->SetMass(1.0, *g->RawGet<TransformComponent>());
    }
}

void TestBrickWall()
{
    auto m = CubeMesh();
    auto [brickTextureZ, brickMaterial] = BrickMaterial();

    GameobjectCreateParams wallParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
    wallParams.meshId = m->meshId;
    wallParams.materialId = brickMaterial->id;
    auto wall1 = GameObject::New(wallParams);
    wall1->Get<TransformComponent>()->SetPos({ 4, 4, 0 });
    wall1->Get<TransformComponent>()->SetRot(glm::vec3{ 0, 0, 0.0 });
    wall1->Get<ColliderComponent>()->elasticity = 1.0;
    wall1->Get<TransformComponent>()->SetScl({ 1, 8, 8 });
    wall1->Get<RenderComponent>()->SetColor({ 0, 1, 1, 1 });
    wall1->Get<RenderComponent>()->SetTextureZ(brickTextureZ);
    wall1->name = "wall";
    //{

    //    // auto wall2 = ComponentRegistry::NewGameObject(wallParams);
    //    // wall2->Get<TransformComponent>().SetPos({-4, 4, 0});
    //    // wall2->Get<TransformComponent>().SetRot(glm::vec3 {0, 0, 0.0});
    //    // wall2->colliderComponent->elasticity = 1.0;
    //    // wall2->Get<TransformComponent>().SetScl({1, 8, 8});
    //    // wall2->renderComponent->SetColor({1, 1, 1, 1.0});
    //    // wall2->renderComponent->SetTextureZ(brickTextureZ);
    //    // wall2->name = "wall";
    //    // auto wall3 = ComponentRegistry::NewGameObject(wallParams);
    //    // wall3->Get<TransformComponent>().SetPos({0, 4, 4});
    //    // wall3->Get<TransformComponent>().SetRot(glm::vec3 {0, 0, 0.0});
    //    // wall3->colliderComponent->elasticity = 1.0;
    //    // wall3->Get<TransformComponent>().SetScl({8, 8, 1});
    //    // wall3->renderComponent->SetColor({1, 1, 1, 1});
    //    // wall3->renderComponent->SetTextureZ(brickTextureZ);
    //    // wall3->name = "wall";
    //    // auto wall4 = ComponentRegistry::NewGameObject(wallParams);
    //    // wall4->Get<TransformComponent>().SetPos({0, 4, -4});
    //    // wall4->Get<TransformComponent>().SetRot(glm::vec3 {0, 0, 0.0});
    //    // wall4->colliderComponent->elasticity = 1.0;
    //    // wall4->Get<TransformComponent>().SetScl({8, 8, 1});
    //    // wall4->renderComponent->SetColor({1, 1, 1, 1.0});
    //    // wall4->renderComponent->SetTextureZ(brickTextureZ);
    //    // wall4->name = "wall";

    //    // wall2->Get<TransformComponent>().SetParent(*wall1->transformComponent);
    //    // wall3->Get<TransformComponent>().SetParent(*wall2->transformComponent);
    //    // wall4->Get<TransformComponent>().SetParent(*wall3->transformComponent);

    //    // wall1->Get<TransformComponent>().SetRot(wall1->Get<TransformComponent>().Rotation() * glm::quat(glm::vec3(0.0, glm::radians(15.0), 0.0)));
    //    // wall1->Get<TransformComponent>().SetScl({1, 1, 3});
    //}

}

void TestGrassFloor()
{
    auto m = CubeMesh();
    auto [grassTextureZ, grassMaterial] = GrassMaterial();

    GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
    params.meshId = m->meshId;
    params.materialId = grassMaterial->id;

    auto floor = GameObject::New(params);
    floor->RawGet<ColliderComponent>()->SetCollisionLayer(1);
    floor->RawGet<TransformComponent>()->SetPos({ 0, 0, 0 });
    floor->RawGet<TransformComponent>()->SetRot(glm::vec3{ 0.0, glm::radians(0.0), glm::radians(0.0) });
    floor->RawGet<ColliderComponent>()->elasticity = 1.0;
    floor->RawGet<TransformComponent>()->SetScl({ 10, 1, 10 });
    floor->RawGet<RenderComponent>()->SetColor({ 0, 1, 0, 1.0 });
    floor->RawGet<RenderComponent>()->SetTextureZ(grassTextureZ);
    floor->name = "ah yes the floor here is made of floor";
}

inline noise::module::Perlin perlinNoiseGenerator;

void TestVoxelTerrain()
{

    // 16^3, r=0.25; 0.37ms for perlin
    for (unsigned int i = 0; i < 3; i++) {
        auto cstart = Time();
        float s = 8;
        DualContouringMeshProvider p;
        p.distanceFunction = [](glm::vec3 pos) {
            //float altitude = pos.y + f->GenSingle2D(pos.x, pos.z, 1234);
            float altitude = 16 * perlinNoiseGenerator.GetValue(pos.x / 128.0f, pos.z / 128.0f, 1234);
            //if (pos.y + altitude > 1) {
            //    return 1.0f;
            //}
            //else if (pos.y + altitude < -1) {
            //    return -1.0f;
            //}
            //else {
            return pos.y + altitude;
            //}
        };

        p.fixVertexCenters = false;
        p.atlas = std::nullopt;
        //p.meshParams = MeshCreateParams::Default();
        p.point1 = { -s + float(i * s * 2), -s, -s };
        p.point2 = { s + i * s * 2, s, s };
        p.resolution = 1.0f;
        auto terrainMesh = Mesh::New(p, false);
        //if (!terrainMesh.has_value()) { continue; }

        GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
        params.meshId = terrainMesh->meshId;
        //params.meshId = SphereMesh()->meshId;
        params.materialId = GrassMaterial().second->id;

        auto chunk = GameObject::New(params);
        chunk->Get<TransformComponent>()->SetPos({ 16 + i * s * 2, 0, 16 });
        chunk->Get<TransformComponent>()->SetScl(terrainMesh->originalSize);
        glm::vec4 color = { 0, 0, 0, 1 };
        color[i] = 1;
        chunk->Get<RenderComponent>()->SetColor(color);

        DebugLogInfo("CHUNK HAS ", glm::to_string(terrainMesh->originalSize), " size, nverts = ", terrainMesh->vertices.size(), " elapsed = ", Time() - cstart);
    }

}

void TestSpinningSpotlight()
{
    auto m = CubeMesh();

    GameobjectCreateParams params({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Spotlight, ComponentBitIndex::Collider });
    params.meshId = m->meshId;
    params.materialId = 0;
    auto coolLight = GameObject::New(params);
    coolLight->Get<RenderComponent>()->SetTextureZ(-1);
    coolLight->Get<TransformComponent>()->SetPos({ 30, 5, 0 });
    coolLight->Get<SpotLightComponent>()->SetRange(100);
    coolLight->Get<SpotLightComponent>()->SetColor({ 1, 1, 1 });
    coolLight->Get<RenderComponent>()->SetColor({ 0, 1, 0.5, 1 });

    PhysicsEngine::Get().prePhysicsEvent->Connect([coolLight](float dt) {
        coolLight->Get<TransformComponent>()->SetPos({ cos(Time()) * 10, 5.0, sin(Time()) * 10 });
        coolLight->Get<TransformComponent>()->SetRot(glm::quatLookAt(glm::vec3(cos(Time()), 0.0, sin(Time())), glm::vec3(0, 1, 0)));
    });
}

void TestStationaryPointlight()
{
    auto m = CubeMesh();
    auto [grassTextureZ, grassMaterial] = GrassMaterial();

    GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Pointlight });
    params.meshId = m->meshId;
    params.materialId = grassMaterial->id;

    auto coolLight = GameObject::New(params);
    coolLight->RawGet<RenderComponent>()->SetTextureZ(-1);
    coolLight->RawGet<RenderComponent>()->SetColor({ 0.5, 1.0, 1.0, 1.0 });
    coolLight->RawGet<TransformComponent>()->SetPos({ 40, 5, 40 });
    coolLight->RawGet<PointLightComponent>()->SetRange(1000);
    coolLight->RawGet<PointLightComponent>()->SetColor({ 0.8, 1.0, 0.8 });

    
    auto coolerLight = GameObject::New(params);
    coolerLight->RawGet<TransformComponent>()->SetPos({ 0, 0, 0 });
    coolerLight->RawGet<RenderComponent>()->SetColor({ 1, 0.5, 0, 1.0 });
    coolerLight->RawGet<PointLightComponent>()->SetRange(20);
    coolerLight->RawGet<PointLightComponent>()->SetColor({ 1, 0.3, 0.7 });
    coolerLight->RawGet<TransformComponent>()->SetRot(glm::quatLookAt(glm::vec3(0, -0.7, -0.7), glm::vec3(0, 0, 1)));

    auto coolestLight = GameObject::New(params);
    coolestLight->RawGet<RenderComponent>()->SetTextureZ(-1);
    coolestLight->RawGet<RenderComponent>()->SetColor({ 0.5, 1.0, 1.0, 1.0 });
    coolestLight->RawGet<TransformComponent>()->SetPos({ 0, 500, 0 });
    coolestLight->RawGet<PointLightComponent>()->SetRange(1000000);
    coolestLight->RawGet<PointLightComponent>()->SetColor({ 0.8, 1.0, 0.6 });
}

void TestGarticMusic()
{
    auto garticSound = Sound::New("../sounds/garticphone.wav");

    auto params = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::AudioPlayer});
    params.sound = garticSound;
    auto soundSounder = GameObject::New(params);
    soundSounder->RawGet<AudioPlayerComponent>()->looped = true;
    soundSounder->RawGet<AudioPlayerComponent>()->positional = false;
    soundSounder->RawGet<AudioPlayerComponent>()->volume = 0.15;
    soundSounder->RawGet<AudioPlayerComponent>()->pitch = 0.5;
    soundSounder->RawGet<AudioPlayerComponent>()->Play();
}

void TestUi()
{

    auto [arialLayer, arialFont] = ArialFont();

    auto ui = new Gui(true,
        std::make_optional(std::make_pair(arialLayer, arialFont)),
        std::nullopt
        //Gui::BillboardGuiInfo({.scaleWithDistance = false, .rotation = std::nullopt, .followObject = goWeakPtr}), 
        //GraphicsEngine::Get().defaultBillboardGuiShaderProgram);
        );
    ui->scaleSize = {0.5, 0.15};
    ui->guiScaleMode = Gui::ScaleXX;
    ui->offsetPos = {0.0, 0.0};
        ui->scalePos = {0.5, 0.5};
    ui->anchorPoint = {0.0, 0.0};
    ui->rgba.a = 0.0;
    ui->GetTextInfo().rgba = {1.0, 1.0, 1.0, 1.0};

        ui->GetTextInfo().text = "Honey is a free browser add-on available on Google, Oprah, Firefox, Safari, if it's a browser it has Honey. All you have to do is when you're checking out on one of these major sites, just click that little orange button, and it will scan the entire internet and find discount codes for you. As you see right here, I'm on Hanes, y'know, ordering some shirts because who doesn't like ordering shirts; We saved 11 dollars! Dude our total is 55 dollars, and after Honey, it's 44 dollars. Boom. I clicked once and I saved 11 dollars. There's literally no reason not to install Honey. It takes two clicks, 10 million people use it, 100,000 five star reviews, unless you hate money, you should install Honey. ";
        //ui->GetTextInfo().text = "Tga appbHb kok wjijj wa abcdefghijk eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    ui->GetTextInfo().topMargin = 0;
    ui->GetTextInfo().bottomMargin = 0;
    ui->GetTextInfo().lineHeight = 1.0;
    ui->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
    ui->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
    ui->UpdateGuiText();
    ui->UpdateGuiGraphics();
    ui->UpdateGuiTransform();

    Gui* ui2 = new Gui(false, std::make_optional(std::make_pair(arialLayer, arialFont)));
    ui2->scaleSize = {0.25, 0.05};
    ui2->guiScaleMode = Gui::ScaleXX;
    ui2->offsetPos = {0.0, 0.0};
    ui2->scalePos = {0.75, 0.0};
    ui2->anchorPoint = {0.0, -1.0};

    ui2->rgba = {1.0, 0.5, 0.0, 1.0};
    ui2->UpdateGuiGraphics();
    ui2->UpdateGuiTransform();
}

void TestBillboardUi(glm::dvec3 pos, std::string text)
{
   static  auto [arialLayer, arialFont] = ArialFont();

    //auto go = GameObject::New(GameobjectCreateParams({ ComponentBitIndex::Transform }));
    //go->RawGet<TransformComponent>()->SetPos(pos);

    auto billboardMat = Material::Copy(arialFont);
    billboardMat->shader = GraphicsEngine::Get().defaultBillboardGuiMaterial->shader;

    auto ui = new Gui(
        true,
        std::nullopt,
        std::make_pair(arialLayer, billboardMat),
        Gui::BillboardGuiInfo {.scaleWithDistance = true, .rotation = std::nullopt, .worldPosition = pos }     
    );
    ui->rgba = { 1, 0.5, 0, 0.5 };
    ui->scaleSize = { 0, 0 };
    ui->offsetSize = { 128, 32 };
    ui->anchorPoint = {0.0, 0.0};
    ui->offsetPos = { 0, 0 };
    ui->scalePos = { 0, 0 };
    //ui->zLevel = -0.99;
    
    ui->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
    ui->GetTextInfo().leftMargin = -1000;
    ui->GetTextInfo().rightMargin = 1000;
    ui->GetTextInfo().text = text;
    
    ui->UpdateGuiText();
    ui->UpdateGuiTransform();
    ui->UpdateGuiGraphics();
}

void TestAnimation()
{
    Assert(false); // TODO
    auto [brickTextureZ, brickMaterial] = BrickMaterial();
    auto animShader = ShaderProgram::New("../shaders/world_vertex_animation.glsl", "../shaders/world_fragment.glsl");
    auto stuff = Mesh::MultiFromFile("../models/test_anims.fbx");
    for (auto & ret : stuff) {
        GameobjectCreateParams params({ComponentBitIndex::Transform, ComponentBitIndex::Animation, ComponentBitIndex::Render});
        params.meshId = ret.mesh->meshId;
        params.materialId = ret.material != nullptr ? ret.material->id : brickMaterial->id;
        //params.shaderId = animShader->shaderProgramId;
        auto obj = GameObject::New(params);
        obj->RawGet<RenderComponent>()->SetTextureZ(ret.material != nullptr ? ret.materialZ : brickTextureZ);
        obj->RawGet<TransformComponent>()->SetPos(glm::vec3(5, 3, 5) + ret.posOffset);
        obj->RawGet<TransformComponent>()->SetScl(ret.mesh->originalSize);

        obj->RawGet<AnimationComponent>()->PlayAnimation(ret.mesh->animations->front().name);
    }
}
