#include "construct_game_guis.hpp"
#include "audio/aengine.hpp"
#include "conglomerates/gui.hpp"
#include "tests/gameobject_tests.hpp"
#include "world.hpp"
#include "creature.hpp"
#include "conglomerates/input_stack.hpp"

template <int fontSize>
std::pair <float, std::shared_ptr<Material>> MenuFont1() {
    static auto font = ArialFont(fontSize);
    return font;
}

int buildUiScrollAmt = 0;
InputStack::StackId guiScrollId = nullptr;


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

Constructible* currentSelectedConstructible = nullptr;

void MakeGameMenu() {
    
    // construction menu
    std::vector<ConstructionTab> constructionTabs = {
        ConstructionTab {
            .items = {
                Constructible {
                    .name = "Clear",
                },
        Constructible {
                    .name = "Deforest",
                },
            },
            .name = "Terrain",
        },
        ConstructionTab {
            .items = {
                Constructible {
                    .name = "Wood wall",
                    .placementMode = 1
                }
            },
            .name = "Base",
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
    static int currentConstructionTabIndex = -1;
    static std::shared_ptr<Gui> currentConstructionTab = nullptr;
    //static std::vector<std::unique_ptr<Event<InputObject>::Connection>> connections;

    auto constructionFrame = std::make_shared<Gui>(false, std::nullopt);
    constructionGui.push_back(constructionFrame);
    
    constructionFrame->scalePos = { 0, 0 };
    constructionFrame->offsetPos = { 8, 8 };

    constructionFrame->scaleSize = { 0, 0 };
    constructionFrame->offsetSize = { constructionTabs.size() * TAB_ICON_WIDTH + (constructionTabs.size() + 1) * TAB_ICON_SPACING, TAB_ICON_WIDTH + 2 * TAB_ICON_SPACING };
    constructionFrame->zLevel = -0.01;

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
        auto tab = std::make_shared<Gui>(true, std::nullopt, MenuFont1<12>());
        tab->GetTextInfo().leftMargin = -1000;
        tab->GetTextInfo().rightMargin = 1000;
        tab->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
        tab->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
        tab->GetTextInfo().text = tabInfo.name;

        tab->scaleSize = { 0, 0 };
        tab->offsetSize = { TAB_ICON_WIDTH, TAB_ICON_WIDTH };
        tab->zLevel = -0.02;

        tab->sortValue = tabIndex++;
        tab->SetParent(constructionFrame.get());

        tab->UpdateGuiText();
        tab->UpdateGuiGraphics();

        tab->onMouseEnter->Connect([p = tab.get()]() {
            p->GetTextInfo().rgba = { 1, 1, 0, 1 };
            p->UpdateGuiGraphics();
            GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemSelectionCursor);
            });
        tab->onMouseExit->Connect([p = tab.get()]() {
            p->GetTextInfo().rgba = { 0, 0, 1, 1 };
            p->UpdateGuiGraphics();
            GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemPointerCursor);
            });

        tab->onInputBegin->Connect([p = tab.get(), tabInfo, tabIndex](const InputObject& input) mutable {
            

            if (input.input != InputObject::LMB) return;

            if (currentConstructionTabIndex == tabIndex) {
                currentConstructionTabIndex = -1;
                currentConstructionTab = nullptr;
                return;
            }
            else {
                if (currentConstructionTab) {
                    currentConstructionTab = nullptr;
                }

                auto buildingsList = std::make_shared<Gui>(false, std::nullopt);

                buildUiScrollAmt = 0;

                buildingsList->scalePos = { 0, 0 };
                buildingsList->offsetPos = { 8, 8 + p->GetPixelSize().y/2 + p->GetPixelPos().y + TAB_ICON_SPACING};

                buildingsList->scaleSize = { 0, 0 };
                buildingsList->offsetSize = { 3 * TAB_ICON_WIDTH + 4 * TAB_ICON_SPACING, 3 * TAB_ICON_WIDTH + 4 * TAB_ICON_SPACING };

                buildingsList->zLevel = -0.03;

                buildingsList->anchorPoint = { -0.5, -0.5 };
                buildingsList->rgba = { 1.0, 1.0, 1.0, 0.4 };
                

                // To have scrolling, we clip all construction options against the rendered frame, but parent those options to an invisible gui we move up and down.
                auto contents = std::make_shared<Gui>(false, std::nullopt);  
                contents->rgba = { 0, 0, 0, 0 }; // TODO: might be good to just have an option to make the gui not have a rendercomponent at all
                contents->SetParent(buildingsList.get());
                contents->scaleSize = buildingsList->scaleSize;
                contents->offsetSize = buildingsList->offsetSize;
                contents->childBehaviour = GuiChildBehaviour::Grid;
                contents->gridInfo = Gui::GridGuiInfo{
                    .gridOffsetPosition = { -contents->GetPixelSize().x / 2 + TAB_ICON_SPACING,  contents->GetPixelSize().y / 2 - TAB_ICON_SPACING},
                    .gridOffsetSize = {TAB_ICON_SPACING, -TAB_ICON_SPACING},
                    .maxInFillDirection = 2,//(int)buildingsList->GetPixelSize().x,
                    .maxInPixels = false,
                    .fillXFirst = true,
                    .addGuiLengths = true,

                };

                //contents->UpdateGuiTransform();
                contents->UpdateGuiGraphics();

                int itemI = 0;
                for (auto& item : tabInfo.items) {
                    auto construction = std::make_shared<Gui>(true, std::nullopt, MenuFont1<12>(), std::nullopt, true);

                    construction->scalePos = { 0, 0 };
                    //construction->offsetPos = { 8, 8 + p->GetPixelSize().y / 2 + p->GetPixelPos().y + TAB_ICON_SPACING };

                    construction->scaleSize = { 0, 0 };
                    construction->offsetSize = { TAB_ICON_WIDTH, TAB_ICON_WIDTH };
                    construction->anchorPoint = { -0.5, 0.5 };

                    construction->zLevel = -0.04;

                    construction->GetTextInfo().leftMargin = -1000;
                    construction->GetTextInfo().rightMargin = 1000;
                    construction->GetTextInfo().horizontalAlignment = HorizontalAlignMode::Center;
                    construction->GetTextInfo().verticalAlignment = VerticalAlignMode::Center;
                    construction->GetTextInfo().text = item.name;
                    construction->rgba = { 0.4, 0.4, 0.4, 1.0 };
                    construction->GetTextInfo().rgba = {1.0, 1.0, 1.0, 1.0};
                    construction->clipTarget = buildingsList;
                    construction->SetParent(contents.get());

                    construction->onMouseEnter->Connect([p = construction.get()]() {
                        p->rgba = { 0.3, 0.3, 0.3, 1.0 };
                        p->UpdateGuiGraphics();
                        GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemSelectionCursor);
                    });
                    construction->onMouseExit->Connect([p = construction.get()]() {
                        p->rgba = { 0.4, 0.4, 0.4, 1.0 };
                        p->UpdateGuiGraphics();
                        GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemPointerCursor);
                    });

                    construction->onInputBegin->Connect([item](InputObject input) mutable {
                        if (input.input == InputObject::LMB) {
                            if (currentSelectedConstructible) {
                                if (&item == currentSelectedConstructible) {
                                    currentSelectedConstructible = nullptr;
                                    return;
                                }
                                currentSelectedConstructible = nullptr;
                            }

                            currentSelectedConstructible = &item;
                        }
                    });

                    construction->UpdateGuiGraphics();
                    construction->UpdateGuiText();


                }

                contents->SortChildren();
                buildingsList->UpdateGuiTransform();
                buildingsList->UpdateGuiGraphics();

                /*buildingsList->onInputBegin->Connect([p2 = contents.get()](InputObject input) {
                    if (input.input == InputObject::Scroll) {
                        buildUiScrollAmt = std::min(input.direction.y * 15 + buildUiScrollAmt, 0.0f);
                        p2->offsetPos.y = buildUiScrollAmt;
                        p2->UpdateGuiTransform();
                    }
                    });*/

                buildingsList->onMouseEnter->Connect([p2 = contents.get()]() {
                    if (guiScrollId) {
                        DebugLogError("UH IS THIS A PROBLEM??!?!");
                        InputStack::Get().PopBegin(InputObject::Scroll, guiScrollId);
                        guiScrollId = nullptr;
                    }
                    guiScrollId = InputStack::Get().PushBegin(InputObject::Scroll, [p2](InputObject input) {
                        if (input.input == InputObject::Scroll) {
                            buildUiScrollAmt = std::min(input.direction.y * 15 + buildUiScrollAmt, 0.0f);
                            p2->offsetPos.y = buildUiScrollAmt;
                            p2->UpdateGuiTransform();
                        }
                    });
                });

                buildingsList->onMouseExit->Connect([]() {
                    if (!guiScrollId) {
                        DebugLogError("WHY NO ID?!?!");
                    }
                    else {
                        InputStack::Get().PopBegin(InputObject::Scroll, guiScrollId);
                        guiScrollId = nullptr;
                    }
                });

                std::swap(buildingsList, currentConstructionTab);
                currentConstructionTabIndex = tabIndex;
            }

            //auto buildingsList = std::make_shared<Gui>(false, std::nullopt);
            //constructionGui.push_back(buildingsList);

            


            
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
        GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemSelectionCursor);
        });
    startGame->onMouseExit->Connect([p = startGame.get()]() {
        p->GetTextInfo().rgba = { 1, 0, 0, 1 };
        p->UpdateGuiGraphics();
        GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemPointerCursor);
        });
    startGame->onInputEnd->Connect([](InputObject input) {

        if (input.input == InputObject::LMB) {
            inMainMenu = false;
            auto& duh = mainMenuGuis;

            TestBillboardUi({ -13, 2.0, -13 }, "bob");

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
            GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemPointerCursor); // TODO: this pattern is dumb and WILL  cause (minor) bugs
            auto cre = Creature::New(CubeMesh(), Body::Humanoid());
            cre->gameObject->RawGet<TransformComponent>()->SetPos({ -13, 1.0, 10 });
            cre->MoveTo({ -13, -13 });
        }
        });

    startGame->UpdateGuiGraphics();
    startGame->UpdateGuiTransform();
    startGame->UpdateGuiText();

}