#include "construct_game_guis.hpp"
#include "audio/aengine.hpp"
#include "conglomerates/gui.hpp"
#include "tests/gameobject_tests.hpp"
#include "world.hpp"
#include "creature.hpp"

template <int fontSize>
std::pair <float, std::shared_ptr<Material>> MenuFont1() {
    static auto font = ArialFont(fontSize);
    return font;
}

void MakeGameMenu() {
    struct ConstructionTab {
        // std::vector<> constructibles;
        std::string name;
        std::string iconPath;
    };

    // construction menu
    std::vector<ConstructionTab> constructionTabs = {
        ConstructionTab {
            .name = "Structural",
        },
        ConstructionTab {
            .name = "Violence"
        },
    };


    constexpr int TAB_ICON_WIDTH = 48;
    constexpr int TAB_ICON_SPACING = 8;

    static std::vector<std::shared_ptr<Gui>> constructionGui;
    auto constructionFrame = std::make_shared<Gui>(false);
    constructionGui.push_back(constructionFrame);
    
    constructionFrame->scalePos = { 0, 0 };
    constructionFrame->offsetPos = { 8, 8 };

    constructionFrame->scaleSize = { 0, 0 };
    constructionFrame->offsetSize = { constructionTabs.size() * TAB_ICON_WIDTH + (constructionTabs.size() + 1) * TAB_ICON_SPACING, TAB_ICON_WIDTH + 2 * TAB_ICON_SPACING };

    constructionFrame->anchorPoint = { -0.5, -0.5 };
    constructionFrame->rgba = { 1.0, 1.0, 1.0, 0.4 };
    constructionFrame->childBehaviour = GuiChildBehaviour::Grid;
    constructionFrame->gridInfo = Gui::GridGuiInfo{
        .gridOffsetPosition = {TAB_ICON_SPACING + TAB_ICON_WIDTH/2 - constructionFrame->offsetSize.x/2, 0},
        .gridOffsetSize = {TAB_ICON_SPACING, 0},
        .maxInPixels = false,
        .fillXFirst = true,
        .addGuiLengths = true,
    };
    
    int tabIndex = 0;
    for (auto& tabInfo : constructionTabs) {
        auto tab = std::make_shared<Gui>(true, MenuFont1<10>());
        tab->GetTextInfo().leftMargin = -1000;
        tab->GetTextInfo().rightMargin = 1000;
        tab->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
        tab->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
        tab->GetTextInfo().text = tabInfo.name;

        tab->scaleSize = { 0, 0 };
        tab->offsetSize = { TAB_ICON_WIDTH, TAB_ICON_WIDTH };

        tab->sortValue = tabIndex++;
        tab->SetParent(constructionFrame.get());
        tab->UpdateGuiText();
    }

    

    constructionFrame->SortChildren();

    constructionFrame->UpdateGuiGraphics();
    constructionFrame->UpdateGuiTransform();
}

void MakeMainMenu() {
    inMainMenu = true;

    static std::vector<std::shared_ptr<Gui>> mainMenuGuis;

    auto& GE = GraphicsEngine::Get();
    auto& AE = AudioEngine::Get();
    //auto& CR = ComponentRegistry::Get();

    auto ttfParams = TextureCreateParams({ TextureSource("../fonts/arial.ttf"), }, Texture::FontMap);
    ttfParams.fontHeight = 24;
    ttfParams.format = Texture::Grayscale_8Bit;
    auto [arialLayer, arialFont] = Material::New(MaterialCreateParams{ .textureParams = { ttfParams }, .type = Texture::Texture2D, .depthMask = false });

    //// don't take by reference bc vector reallocation invalidates references/reference to temporary
    auto startGame = std::make_shared<Gui>(true, std::optional(std::make_pair(arialLayer, arialFont)));
    mainMenuGuis.push_back(startGame);

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
    startGame->onMouseEnter->Connect([p = startGame.get()]() {
        p->GetTextInfo().rgba = { 1, 1, 0, 1 };
        p->UpdateGuiGraphics();
        });
    startGame->onMouseExit->Connect([p = startGame.get()]() {
        p->GetTextInfo().rgba = { 1, 0, 0, 1 };
        p->UpdateGuiGraphics();
        });
    startGame->onInputEnd->Connect([](InputObject input) {

        if (input.input == InputObject::LMB) {
            inMainMenu = false;
            auto& duh = mainMenuGuis;

            mainMenuGuis.clear();
            //TestVoxelTerrain();
            //TestSphere(4, 4, 4, false);
            //TestSphere(6, 4, 4, false);
            //TestCubeArray(glm::uvec3(8, 8, 8), glm::uvec3(0, 0, 0), glm::uvec3(16, 1, 16), false, glm::vec3(0.5, 0.5, 0.5));
            //TestCubeArray(1, 2, 1, false);
            DebugLogInfo("STARTING GAME");
            //Chunk::LoadWorld(GraphicsEngine::Get().camera.position, 512);
            World::Generate();
            MakeGameMenu();

            Creature::New(CubeMesh(), Body::Humanoid());
        }
        });

    startGame->UpdateGuiGraphics();
    startGame->UpdateGuiTransform();
    startGame->UpdateGuiText();

}