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
    struct Constructible {
        std::string name;
        int placementMode = 2; // 0 = point, 1 = line, 2 = area
        bool snapPlacement = true;
    };

    struct ConstructionTab {
        std::vector<Constructible> items;
        std::string name;
        std::string iconPath;
    };
    
    // construction menu
    std::vector<ConstructionTab> constructionTabs = {
        ConstructionTab {
            .items = {
                Constructible {
                    .name = "Clear"
                }
            },
            .name = "Terrain",
        },
        ConstructionTab {
            .name = "Food"
        },
        ConstructionTab {
            .name = "Military"
        }
    };

    

    constexpr int TAB_ICON_WIDTH = 48;
    constexpr int TAB_ICON_SPACING = 8;

    static std::vector<std::shared_ptr<Gui>> constructionGui;
    //static std::vector<std::unique_ptr<Event<InputObject>::Connection>> connections;

    auto constructionFrame = std::make_shared<Gui>(false, std::nullopt);
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
        auto tab = std::make_shared<Gui>(true, std::nullopt, MenuFont1<10>());
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
        tab->UpdateGuiGraphics();

        tab->onMouseEnter->Connect([p = tab.get()]() {
            DebugLogInfo("OO")
            p->GetTextInfo().rgba = { 1, 1, 0, 1 };
            p->UpdateGuiGraphics();
            });
        tab->onMouseExit->Connect([p = tab.get()]() {
            p->GetTextInfo().rgba = { 0, 0, 1, 1 };
            p->UpdateGuiGraphics();
            });

        /*connections.push_back(std::move(*/tab->onInputBegin->Connect([p = tab.get(), tabInfo, tabIndex](const InputObject& input) {
            
            if (input.input != InputObject::LMB) return;

            auto buildingsList = std::make_shared<Gui>(false, std::nullopt);
            constructionGui.push_back(buildingsList);

            buildingsList->scalePos = { 0, 0 };
            buildingsList->offsetPos = { 8, 8 };

            buildingsList->scaleSize = { 0, 0 };
            buildingsList->offsetSize = { 3 * TAB_ICON_WIDTH + 4 * TAB_ICON_SPACING + 8, 3 * TAB_ICON_WIDTH + 4 * TAB_ICON_SPACING };

            buildingsList->anchorPoint = { -0.5, -0.5 };
            buildingsList->rgba = { 1.0, 1.0, 1.0, 0.4 };
            buildingsList->childBehaviour = GuiChildBehaviour::Grid;
            buildingsList->gridInfo = Gui::GridGuiInfo{
                //.gridOffsetPosition = {TAB_ICON_SPACING + TAB_ICON_WIDTH / 2 - constructionFrame->offsetSize.x / 2, 0},
                .gridOffsetSize = {TAB_ICON_SPACING, 0},
                .maxInPixels = false,
                .fillXFirst = true,
                .addGuiLengths = true,
            };


            
        });
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

    //// don't take by reference bc vector reallocation invalidates references/reference to temporary
    auto startGame = std::make_shared<Gui>(true, std::nullopt, MenuFont1<24>());
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
            //World::Generate();
            MakeGameMenu();

            //Creature::New(CubeMesh(), Body::Humanoid());
        }
        });

    startGame->UpdateGuiGraphics();
    startGame->UpdateGuiTransform();
    startGame->UpdateGuiText();

}