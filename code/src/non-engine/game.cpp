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

#include "tests/gameobject_tests.hpp"
#include "world.hpp"
#include "creature.hpp"

#include "construct_game_guis.hpp"
#include "conglomerates/input_stack.hpp"

//#include "noise/noise.h"

//auto n = FastNoise::New<FastNoise::Simplex>();
//auto f = FastNoise::New<FastNoise::Generator>();
//noise::module::Perlin perlinNoiseGenerator;





constexpr int WORLD_ZOOM_NAME = 2000;

void GameInit()
{
    auto& GE = GraphicsEngine::Get();
    auto& PE = PhysicsEngine::Get();
    //auto& AE = AudioEngine::Get();
    //auto& LUA = LuaHandler::Get();
    //auto& CR = ComponentRegistry::Get();

    MakeMainMenu();
    MakeFPSTracker();

    //TestGrassFloor();
    //TestCubeArray({ 2, 2, 2 }, {4, 4, 4}, {2, 2, 2}, true);
    //PE.SetCollisionLayers(0, 0, false);

    GE.skyboxMaterial->shader = ShaderProgram::New("../shaders/skybox_vertex.glsl", "../shaders/skybox_fragment_static.glsl");
    //return;

    //World::Generate();

    //GE.SetDebugFreecamEnabled(true);
    GE.camera.rotation = glm::quatLookAt(glm::vec3(0, -1, 0), glm::vec3(0, 0, 1));
    GE.camera.position = glm::vec3(0, 1, 0);
    //

    InputStack::Get().PushBegin(InputObject::Scroll, WORLD_ZOOM_NAME, [](InputObject inputObject) {
        double x = inputObject.direction.x;
        double y = inputObject.direction.y;

        auto& GE = GraphicsEngine::Get();

        if (y != 0) {
            glm::vec3 mouseDirection = GE.camera.ProjectToWorld(GE.window.MOUSE_POS, glm::vec2(GE.window.width, GE.window.height)) * y;
            mouseDirection *= abs(1.0 / mouseDirection.y);
            GE.camera.position += glm::normalize(mouseDirection) * std::sqrt(std::abs(GE.camera.position.y));
        }
        //else {
            //GE.camera.position.x += x;
            //GE.camera.position.z += y;
        //}

    });

    //GE.window.onScroll->Connect();

    GE.window.postInputProccessing->Connect([]() {
        if (GraphicsEngine::Get().debugFreecamEnabled) { return; }

        auto& GE = GraphicsEngine::Get();

        // cast to int bc otherwise subtracting unsigned ints
        int right = GE.window.PRESSED_KEYS.count(InputObject::D) || GE.window.PRESSED_KEYS.count(InputObject::RightArrow);
        int left = GE.window.PRESSED_KEYS.count(InputObject::A) || GE.window.PRESSED_KEYS.count(InputObject::LeftArrow);

        int up = GE.window.PRESSED_KEYS.count(InputObject::W) || GE.window.PRESSED_KEYS.count(InputObject::UpArrow);
        int down = GE.window.PRESSED_KEYS.count(InputObject::S) || GE.window.PRESSED_KEYS.count(InputObject::DownArrow);

        

        float dx = left - right;
        float dz = up - down;

        float panSpeed = GE.camera.position.y / 50.0f;

        if (GE.window.PRESSED_KEYS.count(InputObject::MMB)) {
            dx = 0.25 * GE.window.MOUSE_DELTA.x;
            dz = 0.25 * GE.window.MOUSE_DELTA.y;
        }

        

        GE.camera.position.x += dx * panSpeed;
        GE.camera.position.z += dz * panSpeed;
    });

     {
    /*    auto ttfParams = TextureCreateParams({ TextureSource("../fonts/arial.ttf"), }, Texture::FontMap);
        ttfParams.fontHeight = 16;
        ttfParams.format = Texture::Grayscale_8Bit;
        auto [arialLayer, arialFont] = Material::New({ .textureParams = { ttfParams }, .type = Texture::Texture2D, .depthMask = false });
        arialFont->depthMaskEnabled = false; why doesn't this one work??? */ 

        auto debugText = new Gui(true, std::nullopt, std::optional(/*std::make_pair(arialLayer, arialFont)*/ ArialFont(12)), std::nullopt); // idc if leaked

        debugText->rgba = { 0, 0, 0, 0};
        debugText->zLevel = 0; // TODO FIX Z-LEVEL/DEPTH BUFFER

        debugText->scalePos = { 0, 1 };
        debugText->scaleSize = { 0, 0 };
        debugText->offsetSize = { 600, 60 };
        debugText->offsetPos = { 200, -200 };
        debugText->anchorPoint = { 0, 0 };

        debugText->GetTextInfo().text = "abcdefghijklmnopqrstuvwxyz";
        debugText->GetTextInfo().rgba = { 1, 1, 1, 1.0 };
        debugText->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
        debugText->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;

        debugText->GetTextInfo().bottomMargin = 0;
        debugText->GetTextInfo().topMargin = 0;

        debugText->UpdateGuiGraphics();
        debugText->UpdateGuiTransform();
        debugText->UpdateGuiText();

        GE.preRenderEvent->Connect([&GE, debugText](float dt) {
            //DebugLogInfo("e");
            debugText->GetTextInfo().text = glm::to_string(GE.GetCurrentCamera().position);
            debugText->UpdateGuiText();
        });
    }

    GE.preRenderEvent->Connect([](float dt) {
        if (!inMainMenu) {
            //Chunk::LoadWorld(GraphicsEngine::Get().camera.position, 512);
        }

      
    });
    
    // TODO 
    //Module::LoadModule("..\\modules\\libtest_module.dll");

    // Gui* ui;
    //std::weak_ptr<GameObject> goWeakPtr;

    

    //// auto m2 = Mesh::FromFile("../models/rainbowcube.obj", makeMparams);
    
    //GE.defaultShaderProgram->Uniform("envLightDiffuse", 0.0f);

    /*auto skyboxFaces = std::vector<TextureSource>(
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
        GE.skyboxMaterial = sky_m_ptr; WON'T WORK W/O SHADER SET
        GE.skyboxMaterialLayer = index;
    }*/

    GE.window.inputDown->Connect([](InputObject input) {
        if (input.input == InputObject::F) {
            GraphicsEngine::Get().SetDebugFreecamEnabled(!GraphicsEngine::Get().debugFreecamEnabled);
            //GraphicsEngine::Get().GetDebugFreecamCamera().position = GraphicsEngine::Get().camera.position;
            //GraphicsEngine::Get().GetDebugFreecamCamera().rotation = GraphicsEngine::Get().camera.rotation;
        }
        else if (input.input == InputObject::T) {
            /*static int y = 0;

            for (int i = 0; i < 1; i++) {
                TestSphere(-2 - i * 2, -2, y += 2, false);
            }*/
        }
    });

    //GE.GetDebugFreecamCamera().position = glm::dvec3(0, 15, 0);

    //TestCubeArray(glm::uvec3(16, 16, 16), glm::uvec3(0, 0, 0), glm::uvec3(16, 1, 16), false);
    
    //TestCubeArray(glm::uvec3(4, 4, 4), glm::uvec3(0, 0, 0), glm::uvec3(16, 1, 16), false, glm::vec3(0.25, 0.25, 0.25));
    //TestCubeArray(glm::uvec3(1, 1, 1), glm::uvec3(0, 0, 0), glm::uvec3(64, 1, 64), false, glm::vec3(0.05, 0.05, 0.05));
    //TestUi();
    //
    //TestStationaryPointlight();
    //TestVoxelTerrain();
    //TestSpinningSpotlight();
    //TestGrassFloor();

    //TestGarticMusic(); 

    

    //GE.window.inputDown->Connect([](InputObject input) {
    //    auto m = CubeMesh();
    //    if (input.input == InputObject::LMB) {
    //        auto castResult = Raycast(GraphicsEngine::Get().GetDebugFreecamCamera().position, LookVector(glm::radians(GraphicsEngine::Get().debugFreecamPitch), glm::radians(GraphicsEngine::Get().debugFreecamYaw)));

    //        if (castResult.hitObject != nullptr) {

    //            GameobjectCreateParams params({ ComponentBitIndex::Transform, ComponentBitIndex::Render, ComponentBitIndex::Collider });
    //            params.meshId = m->meshId;
    //            //params.materialId = brickMaterial->id;
    //            for (unsigned int i = 0; i < 1; i++) {
    //                //auto g = GameObject::New(params);
    //                //g->Get<TransformComponent>()->SetPos(glm::vec3(10, -5, 10));
    //            }
    //            

    //            if (castResult.hitObject != nullptr && castResult.hitObject->MaybeRawGet<RigidbodyComponent>()) {

    //                castResult.hitObject->RawGet<RigidbodyComponent>()->velocity += castResult.hitNormal * 2.0;
    //                //castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
    //            }

    //            else if (castResult.hitObject != nullptr && castResult.hitObject->MaybeRawGet<RenderComponent>()) {
    //                //castResult.hitObject->RawGet<RenderComponent>()->SetColor({ 1.0, 1.0, 1.0, 0.0 });
    //            }
    //        }

    //        //        if (castResult.hitObject != nullptr && castResult.hitObject->rigidbodyComponent) {

    //        //            castResult.hitObject->rigidbodyComponent->velocity += castResult.hitNormal * 0.4;
    //        //            //castResult.hitObject->Get<TransformComponent>().SetPos(castResult.hitObject->Get<TransformComponent>().position + castResult.hitNormal * 0.02);
    //        //        }
    //    }
    //    });

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



    //// LUA.RunString("print(\"help from lua\")");
    //// LUA.RunFile("../scripts/test.lua");

    ///*GE.window.InputDown.Connect([](InputObject input) {
    //    DebugLogInfo("Input = ", input.input);
    //});*/


}

void GameClose() {
    CleanupMenu();
    World::Unload();
}

// ui->GetTextInfo().text = glm::to_string(goWeakPtr.lock()->rigidbodyComponent->velocity) + "\n"s + glm::to_string(goWeakPtr.lock()->rigidbodyComponent->angularVelocity);
// ui->UpdateGuiText();

