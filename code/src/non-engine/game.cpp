#include "game.hpp"
#include <graphics/gengine.hpp>
#include <physics/pengine.hpp>
#include <audio/aengine.hpp>
#include <lua/lua_handler.hpp>
#include <gameobjects/gameobject.hpp>

#include "physics/raycast.hpp"

#include <conglomerates/gui.hpp>
#include <memory>

#include "modifier.hpp"
#include "chunk.hpp"

//#include "noise/noise.h"

//auto n = FastNoise::New<FastNoise::Simplex>();
//auto f = FastNoise::New<FastNoise::Generator>();
//noise::module::Perlin perlinNoiseGenerator;



bool inMainMenu = true;

void MakeMainMenu() {
    inMainMenu = true;

    static std::vector<std::shared_ptr<Gui>> mainMenuGuis;

    auto& GE = GraphicsEngine::Get();
    auto& AE = AudioEngine::Get();
    //auto& CR = ComponentRegistry::Get();
    
    auto ttfParams = TextureCreateParams({ TextureSource("../fonts/arial.ttf"), }, Texture::FontMap);
    ttfParams.fontHeight = 24;
    ttfParams.format = Texture::Grayscale_8Bit;
    auto [arialLayer, arialFont] = Material::New(MaterialCreateParams{ .textureParams = { ttfParams }, .type = Texture::Texture2D, .depthMask = true });

    //// don't take by reference bc vector reallocation invalidates references/reference to temporary
    auto startGame = mainMenuGuis.emplace_back(std::move(std::make_shared<Gui>(true, std::optional(std::make_pair(arialLayer, arialFont )))));

    startGame->rgba = { 0, 0, 1, 1 };
    startGame->zLevel = 0; // TODO FIX Z-LEVEL/DEPTH BUFFER

    startGame->scalePos = { 0.5, 0.5 };
    startGame->scaleSize = { 0, 0 };
    startGame->offsetSize = { 300, 60 };
    startGame->anchorPoint = { 0, 0 };
 
    startGame->GetTextInfo().text = "START GAME";
    startGame->GetTextInfo().rgba = { 1, 0, 0, 1 };
    startGame->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
    startGame->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;

    // note that we aren't passing in the shared_ptr but a normal pointer; if we didn't, the gui's event would store a shared_ptr to itself and it would never get destroyed.
    startGame->onMouseEnter.Connect([p = startGame.get()]() {
        p->GetTextInfo().rgba = { 1, 1, 0, 1 };
        p->UpdateGuiGraphics();
    });
    startGame->onMouseExit.Connect([p = startGame.get()]() {
        p->GetTextInfo().rgba = { 1, 0, 0, 1 };
        p->UpdateGuiGraphics();
    });
    startGame->onInputEnd.Connect([](InputObject input) {
        
        if (input.input == InputObject::LMB) {
            inMainMenu = false;
            mainMenuGuis.clear();
            //DebugLogInfo("STARTING GAME");
            Chunk::LoadWorld(GraphicsEngine::Get().camera.position, 512);
            
        }
    });

    startGame->UpdateGuiGraphics();
    startGame->UpdateGuiTransform();
    startGame->UpdateGuiText();
    
}

void GameInit()
{
    auto& GE = GraphicsEngine::Get();
    auto& PE = PhysicsEngine::Get();
    //auto& AE = AudioEngine::Get();
    //auto& LUA = LuaHandler::Get();
    //auto& CR = ComponentRegistry::Get();

    //MakeMainMenu();

    GE.SetDebugFreecamEnabled(true);

    auto ttfParams = TextureCreateParams({ TextureSource("../fonts/arial.ttf"), }, Texture::FontMap);
    ttfParams.fontHeight = 16;
    ttfParams.format = Texture::Grayscale_8Bit;
    auto [arialLayer, arialFont] = Material::New({ .textureParams = { ttfParams }, .type = Texture::Texture2D, .depthMask = true });

    //auto debugText = new Gui(true, std::optional(std::make_pair(arialLayer, arialFont))); // idc if leaked

    //debugText->rgba = { 1, 1, 1, 0 };
    //debugText->zLevel = 0; // TODO FIX Z-LEVEL/DEPTH BUFFER

    //debugText->scalePos = { 0, 1 };
    //debugText->scaleSize = { 0, 0 };
    //debugText->offsetSize = { 300, 60 };
    //debugText->offsetPos = { 200, -200 };
    //debugText->anchorPoint = { 0, 0 };

    //debugText->GetTextInfo().text = "---";
    //debugText->GetTextInfo().rgba = { 1, 1, 1, 0.6 };
    //debugText->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
    //debugText->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;

    //debugText->UpdateGuiGraphics();
    //debugText->UpdateGuiTransform();

    //GE.preRenderEvent.Connect([&GE, debugText](float dt) {

    //    debugText->GetTextInfo().text = glm::to_string(GE.GetCurrentCamera().position);
    //    debugText->UpdateGuiText();

    //    if (!inMainMenu) {
    //        //Chunk::LoadWorld(GraphicsEngine::Get().camera.position, 512);
    //    }
    //});

    // TODO BROKEN, prob just dll outdated
    //Module::LoadModule("..\\modules\\libtest_module.dll");

    // Gui* ui;
    //std::weak_ptr<GameObject> goWeakPtr;

    //auto garticSound = Sound::New("../sounds/garticphone.wav");
    //{
    //    auto params = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentRegistry::AudioPlayerComponentBitIndex });
    //    params.sound = garticSound;
    //    auto soundSounder = ComponentRegistry::Get().NewGameObject(params);
    //    soundSounder->audioPlayerComponent->looped = true;
    //    soundSounder->audioPlayerComponent->positional = true;
    //    soundSounder->audioPlayerComponent->volume = 0.15;
    //    soundSounder->audioPlayerComponent->pitch = 0.5;
    //    //soundSounder->audioPlayerComponent->Play();
    //}

    auto makeMparams = MeshCreateParams{ .textureZ = -1.0, .opacity = 1, .expectedCount = 16384 };
    //// auto m2 = Mesh::FromFile("../models/rainbowcube.obj", makeMparams);
    auto [m, mat, tz, offest] = Mesh::MultiFromFile("../models/rainbowcube.obj", makeMparams).at(0);

    GE.defaultShaderProgram->Uniform("envLightDiffuse", 0.0f);

    auto [grassTextureZ, grassMaterial] = Material::New(MaterialCreateParams{ 
        .textureParams = {
             TextureCreateParams({ TextureSource{"../textures/grass.png"},}, Texture::ColorMap),
             TextureCreateParams({TextureSource {"../textures/crate_specular.png"}}, Texture::SpecularMap)
        },
        .type = Texture::Texture2D }
    );

    GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Pointlight});
    params.meshId = m->meshId;
    params.materialId = grassMaterial->id;
    auto coolerLight = GameObject::New(params);
    coolerLight->RawGet<TransformComponent>()->SetPos({ 8, 5, 0 });
    coolerLight->RawGet<RenderComponent>()->SetColor({ 1, 0.5, 0, 1.0 });
    coolerLight->RawGet<PointLightComponent>()->SetRange(20);
    coolerLight->RawGet<PointLightComponent>()->SetColor({ 1, 0.3, 0.7 });
    coolerLight->RawGet<TransformComponent>()->SetRot(glm::quatLookAt(glm::vec3(0, -0.7, -0.7), glm::vec3(0, 0, 1)));

    auto [brickTextureZ, brickMaterial] = Material::New(MaterialCreateParams{ .textureParams = {
        TextureCreateParams({ TextureSource{"../textures/ambientcg_bricks085/color.jpg"},}, Texture::ColorMap),
        TextureCreateParams({TextureSource{"../textures/ambientcg_bricks085/roughness.jpg"},}, Texture::SpecularMap),
        TextureCreateParams({TextureSource{"../textures/ambientcg_bricks085/normal_gl.jpg"},}, Texture::NormalMap),
         //TextureCreateParams({TextureSource {"../textures/ambientcg_bricks085/displacement.jpg"},}, Texture::DisplacementMap) // TODO assert format grayscale somehow
        }, .type =Texture::Texture2D });

    //{
        /*GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentRegistry::ColliderComponentBitIndex });
        params.meshId = m->meshId;
        params.materialId = grassMaterial->id;

        auto floor = CR.NewGameObject(params);
        floor->Get<TransformComponent>().SetPos({ 0, 0, 0 });
        floor->Get<TransformComponent>().SetRot(glm::vec3{ 0.0, glm::radians(0.0), glm::radians(0.0) });
        floor->colliderComponent->elasticity = 1.0;
        floor->Get<TransformComponent>().SetScl({ 10, 1, 10 });
        floor->renderComponent->SetColor({ 0, 1, 0, 1.0 });
        floor->renderComponent->SetTextureZ(grassTextureZ);
        floor->name = "ah yes the floor here is made of floor";*/
    //}

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

    //{

    //    // 16^3, r=0.25; 0.37ms for perlin
    //    for (unsigned int i = 0; i < 3; i++) {
    //        auto cstart = Time();
    //        float s = 8;
    //        auto terrainMesh = Mesh::FromVoxels(MeshCreateParams::Default(), { -s + float(i * s * 2), -s, -s }, { s + i * s * 2, s, s }, 1.0f,
    //            [](glm::vec3 pos) {
    //                //float altitude = pos.y + f->GenSingle2D(pos.x, pos.z, 1234);
    //                float altitude = 16 * perlinNoiseGenerator.GetValue(pos.x / 128.0f, pos.z / 128.0f, 1234);
    //                //if (pos.y + altitude > 1) {
    //                //    return 1.0f;
    //                //}
    //                //else if (pos.y + altitude < -1) {
    //                //    return -1.0f;
    //                //}
    //                //else {
    //                return pos.y + altitude;
    //                //}
    //            }, std::nullopt, false
    //        );
    //        if (!terrainMesh.has_value()) { continue; }

    //        GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    //        params.meshId = terrainMesh.value()->meshId;
    //        params.materialId = grassMaterial->id;

    //        auto chunk = CR.NewGameObject(params);
    //        chunk->Get<TransformComponent>().SetPos({ 16 + i * s * 2, 0, 16 });
    //        chunk->Get<TransformComponent>().SetScl(terrainMesh.value()->originalSize);
    //        glm::vec4 color = { 0, 0, 0, 1 };
    //        color[i] = 1;
    //        //chunk->renderComponent->SetColor(color);

    //        DebugLogInfo("CHUNK HAS ", glm::to_string(terrainMesh.value()->originalSize), " size, nverts = ", terrainMesh.value()->vertices.size(), " elapsed = ", Time() - cstart);
    //    }

    //}

    auto skyboxFaces = std::vector<TextureSource>(
        {
            TextureSource("../textures/sky/right.png"),
            TextureSource("../textures/sky/left.png"),
            TextureSource("../textures/sky/top.png"),
            TextureSource("../textures/sky/bottom.png"),
            TextureSource("../textures/sky/back.png"),
            TextureSource("../textures/sky/front.png")
        });

    {
        auto [index, sky_m_ptr] = Material::New(MaterialCreateParams{ .textureParams = { TextureCreateParams {skyboxFaces, Texture::ColorMap} }, .type = Texture::TextureCubemap });
        GE.skyboxMaterial = sky_m_ptr;
        GE.skyboxMaterialLayer = index;
    }
    GE.GetDebugFreecamCamera().position = glm::dvec3(0, 15, 0);


    int nObjs = 0;
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < 2; z++) {
                GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
                params.meshId = m->meshId;
                //params.materialId = brickMaterial->id;
                auto g = GameObject::New(params);
                g->Get<TransformComponent>()->SetPos({ 0 + x * 3, 2 + y * 2, 0 + z * 3 });
                g->Get<ColliderComponent>()->elasticity = 0.3;
                g->Get<ColliderComponent>()->friction = 2.0;
                // g->rigidbodyComponent->angularDrag = 1.0;
                // g->rigidbodyComponent->linearDrag = 1.0;
                // g->rigidbodyComponent->velocity = {1.0, 0.0, 1.0};
                // g->rigidbodyComponent->angularVelocity = {0.20, 1.6, 1.0};
                g->Get<TransformComponent>()->SetScl(glm::dvec3(1.0, 1.0, 1.0));
                g->Get<RenderComponent>()->SetColor(glm::vec4(1, 1, 1, 1));
                //g->renderComponent->SetTextureZ(brickTextureZ);
                g->name = std::string("Gameobject #") + std::to_string(nObjs);
                ////goWeakPtr = g;
                //nObjs++;

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

    //// animation test
    ///*auto animShader = ShaderProgram::New("../shaders/world_vertex_animation.glsl", "../shaders/world_fragment.glsl");
    //auto stuff = Mesh::MultiFromFile("../models/test_anims.fbx");
    //for (auto & [mesh, mmat, matTexZ, offset] : stuff) {
    //    GameobjectCreateParams params({ComponentBitIndex::Transform, ComponentRegistry::AnimationComponentBitIndex, ComponentBitIndex::Render});
    //    params.meshId = mesh->meshId;
    //    params.materialId = mmat != nullptr ? mmat->id : brickMaterial->id;
    //    params.shaderId = animShader->shaderProgramId;
    //    auto obj = CR.NewGameObject(params);
    //    obj->renderComponent->SetTextureZ(mmat != nullptr ? matTexZ : brickTextureZ);
    //    obj->Get<TransformComponent>().SetPos(glm::vec3(5, 3, 5) + offset);
    //    obj->Get<TransformComponent>().SetScl(mesh->originalSize);

    //    obj->animationComponent->PlayAnimation(mesh->animations->front().name);
    //}*/

    //// make light
    {
        GameobjectCreateParams params({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Spotlight, ComponentBitIndex::Collider});
        params.meshId = m->meshId;
        params.materialId = 0;
        auto coolLight = GameObject::New(params);
        coolLight->Get<RenderComponent>()->SetTextureZ(-1);
        coolLight->Get<TransformComponent>()->SetPos({ 30, 5, 0 });
        coolLight->Get<SpotLightComponent>()->SetRange(100);
        coolLight->Get<SpotLightComponent>()->SetColor({ 1, 1, 1 });
        coolLight->Get<RenderComponent>()->SetColor({ 0, 1, 0.5, 1 });

        PE.prePhysicsEvent.Connect([coolLight](float dt) {
            coolLight->Get<TransformComponent>()->SetPos({ cos(Time()) * 10, 5.0, sin(Time()) * 10 });
            coolLight->Get<TransformComponent>()->SetRot(glm::quatLookAt(glm::vec3(cos(Time()), 0.0, sin(Time())), glm::vec3(0, 1, 0)));
        });
    }
    //{
    //    GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentRegistry::PointlightComponentBitIndex, ComponentBitIndex::Render, ComponentRegistry::ColliderComponentBitIndex });
    //    params.meshId = m->meshId;
    //    params.materialId = grassMaterial->id;
    //    auto coolLight = ComponentRegistry::Get().NewGameObject(params);
    //    coolLight->renderComponent->SetTextureZ(-1);
    //    coolLight->Get<TransformComponent>().SetPos({ 40, 5, 40 });
    //    coolLight->pointLightComponent->SetRange(1000);
    //    coolLight->pointLightComponent->SetColor({ 0.0, 1.0, 0.0 });
    //}

    //// { // text rendering stuff
    ////     auto ttfParams = TextureCreateParams({"../fonts/arial.ttf",}, Texture::FontMap);
    ////     ttfParams.fontHeight = 60;
    ////     ttfParams.format = Texture::Grayscale_8Bit;
    ////     auto [arialLayer, arialFont] = Material::New({ttfParams}, Texture::Texture2D);
    ////     auto textMesh = Mesh::FromText(
    ////         // "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ",
    ////         "Honey is a free browser add-on available on Google, Oprah, Firefox, Safari, if it's a browser it has Honey. All you have to do is when you're checking out on one of these major sites, just click that little orange button, and it will scan the entire internet and find discount codes for you. As you see right here, I'm on Hanes, y'know, ordering some shirts because who doesn't like ordering shirts; We saved 11 dollars! Dude our total is 55 dollars, and after Honey, it's 44 dollars. Boom. I clicked once and I saved 11 dollars. There's literally no reason not to install Honey. It takes two clicks, 10 million people use it, 100,000 five star reviews, unless you hate money, you should install Honey. ",
    ////         arialFont->fontMapConstAccess.value());
    ////     std::cout << "Textmesh id = "  << textMesh->meshId << ".\n";  
    ////     GameobjectCreateParams params({ComponentBitIndex::Transform, ComponentBitIndex::Render});
    ////     params.meshId = textMesh->meshId;
    ////     params.shaderId = GE.defaultGuiShaderProgram->shaderProgramId;
    ////     params.materialId = arialFont->id;
    ////     auto text = ComponentRegistry::Get().NewGameObject(params);
    ////     text->renderComponent->SetTextureZ(arialLayer);
    ////     text->Get<TransformComponent>().SetPos({0, 0, 0});
    ////     text->Get<TransformComponent>().SetScl(textMesh->originalSize * 0.01f);
    ////     text->Get<TransformComponent>().SetRot(glm::quat(glm::vec3(0.0, 0.0, glm::radians(180.0))));

    //// }


    //GE.debugFreecamEnabled = true;
    //GE.window.SetMouseLocked(true);

    //// glPointSize(8.0); // debug thing, ignore
    //// glLineWidth(2.0);


     //{
     //    auto ttfParams = TextureCreateParams({"../fonts/arial.ttf",}, Texture::FontMap);
     //    ttfParams.fontHeight = 16;
     //    ttfParams.format = Texture::Grayscale_8Bit;
     //    auto [arialLayer, arialFont] = Material::New({ttfParams}, Texture::Texture2D, true);

     //    auto ui = new Gui(true,
     //        std::make_optional(std::make_pair(arialLayer, arialFont)),
     //        std::nullopt
     //        //Gui::BillboardGuiInfo({.scaleWithDistance = false, .rotation = std::nullopt, .followObject = goWeakPtr}), 
     //        //GraphicsEngine::Get().defaultBillboardGuiShaderProgram);
     //     );
     //    ui->scaleSize = {0.5, 0.15};
     //    ui->guiScaleMode = Gui::ScaleXX;
     //    ui->offsetPos = {0.0, 0.0};
     //     ui->scalePos = {0.5, 0.5};
     //    ui->anchorPoint = {0.0, 0.0};
     //    ui->rgba.a = 0.0;
     //    ui->GetTextInfo().rgba = {1.0, 1.0, 1.0, 1.0};

     //     ui->GetTextInfo().text = "Honey is a free browser add-on available on Google, Oprah, Firefox, Safari, if it's a browser it has Honey. All you have to do is when you're checking out on one of these major sites, just click that little orange button, and it will scan the entire internet and find discount codes for you. As you see right here, I'm on Hanes, y'know, ordering some shirts because who doesn't like ordering shirts; We saved 11 dollars! Dude our total is 55 dollars, and after Honey, it's 44 dollars. Boom. I clicked once and I saved 11 dollars. There's literally no reason not to install Honey. It takes two clicks, 10 million people use it, 100,000 five star reviews, unless you hate money, you should install Honey. ";
     //     //ui->GetTextInfo().text = "Tga appbHb kok wjijj wa abcdefghijk eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
     //    ui->GetTextInfo().topMargin = 0;
     //    ui->GetTextInfo().bottomMargin = 0;
     //    ui->GetTextInfo().lineHeight = 1.0;
     //    ui->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
     //    ui->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
     //    ui->UpdateGuiText();
     //    ui->UpdateGuiGraphics();
     //    ui->UpdateGuiTransform();

     //     Gui ui2(false, std::make_optional(std::make_pair(arialLayer, arialFont)));
     //     ui2.scaleSize = {0.25, 0.05};
     //     ui2.guiScaleMode = Gui::ScaleXX;
     //     ui2.offsetPos = {0.0, 0.0};
     //     ui2.scalePos = {0.75, 0.0};
     //     ui2.anchorPoint = {0.0, -1.0};   

     //     ui2.rgba = {1.0, 0.5, 0.0, 1.0};
     //     ui2.UpdateGuiGraphics();
     //     ui2.UpdateGuiTransform();
     //}

    //// LUA.RunString("print(\"help from lua\")");
    //// LUA.RunFile("../scripts/test.lua");

    ///*GE.window.InputDown.Connect([](InputObject input) {
    //    DebugLogInfo("Input = ", input.input);
    //});*/

    GE.window.InputDown.Connect([m](InputObject input) {
        if (input.input == InputObject::LMB) {
            auto castResult = Raycast(GraphicsEngine::Get().GetDebugFreecamCamera().position, LookVector(glm::radians(GraphicsEngine::Get().debugFreecamPitch), glm::radians(GraphicsEngine::Get().debugFreecamYaw)));

            if (castResult.hitObject != nullptr) {

                GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
                params.meshId = m->meshId;
                //params.materialId = brickMaterial->id;
                auto g = GameObject::New(params);
                g->Get<TransformComponent>()->SetPos(castResult.hitPoint + castResult.hitNormal * 0.5);

                //if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {

                    //castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
                    //castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
                //}    
            }

            //        if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {

            //            castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
            //            //castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
            //        }
        }
    });

    //PE.prePhysicsEvent.Connect([&GE](float dt) {
    //    if (GE.window.LMB_DOWN) {
    //        auto castResult = Raycast(GE.GetDebugFreecamCamera().position, LookVector(glm::radians(GE.debugFreecamPitch), glm::radians(GE.debugFreecamYaw)));

    //        if (castResult.hitObject != nullptr) {
    //            // std::cout << "Hit object " << castResult.hitObject->name << ", normal is " << glm::to_string(castResult.hitNormal) << " \n";
    //        }

    //        if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {

    //            castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
    //            //castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
    //        }
    //        else {
    //            // std::cout << "LMB_DOWN but not hitting anything.\n";
    //        }

    //    }
    //});
}


// ui->GetTextInfo().text = glm::to_string(goWeakPtr.lock()->rigidbodyComponent->velocity) + "\n"s + glm::to_string(goWeakPtr.lock()->rigidbodyComponent->angularVelocity);
// ui->UpdateGuiText();

