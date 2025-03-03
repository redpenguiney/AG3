#include "construct_game_guis.hpp"
#include "audio/aengine.hpp"
#include "conglomerates/gui.hpp"
#include "tests/gameobject_tests.hpp"
#include "world.hpp"
#include "creature.hpp"
#include "conglomerates/input_stack.hpp"
#include <physics/raycast.hpp>
#include "ai/tasks.hpp"
#include "ai/tasks_impl.hpp"
#include "humanoid.hpp"

template <int fontSize>
std::pair <float, std::shared_ptr<Material>> MenuFont1() {
    static auto font = ArialFont(fontSize);
    return font;
}

int buildUiScrollAmt = 0;
constexpr int GUI_SCROLL_NAME = 100;
constexpr int GuiLMBName = 104;
constexpr int PlaceLMBName = 1200;
int hover = 0;

struct Constructible;

struct ConstructionTab {
    std::vector<Constructible> items;
    std::string name;
    std::string iconPath;

    //ConstructionTab(const ConstructionTab&) = delete;
};



Constructible* currentSelectedConstructible = nullptr;
std::shared_ptr<GameObject> currentConstructibleGhost = nullptr;
std::optional<glm::vec2> currentBuildingP1 = std::nullopt; 

// just a simple raycast
std::optional<glm::vec2> GetCursorTilePos(bool snapToGrid) {

    // find where person clicked by raycasting in screen direction
    CollisionLayerSet layers = 0;
    layers[RenderChunk::COLLISION_LAYER] = true;
    auto dir = GraphicsEngine::Get().GetCurrentCamera().ProjectToWorld(GraphicsEngine::Get().window.MOUSE_POS, { GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height });
    //assert(glm::length2(dir) < 1.01 && glm::length2(dir) > 0.99);
    //DebugLogInfo("DIR ", glm::length(dir));
    auto result = Raycast(
        GraphicsEngine::Get().GetCurrentCamera().position,
        dir,
        layers
    );

    if (result.hitObject) {
        if (snapToGrid) { // we don't care about y coordinate in this case
            result.hitPoint -= result.hitNormal * 0.5; // offset point in normal direction so that if they click on the side of a wall it picks the wall
            result.hitPoint.x = std::round(result.hitPoint.x);
            result.hitPoint.z = std::round(result.hitPoint.z);
        }
        return std::optional<glm::vec2>({ result.hitPoint.x, result.hitPoint.z });
    }
    else {
        //DebugLogInfo("MISS");
        return std::nullopt;
    }
}

struct SetTileApplicatorParams {
    ChangeTileTaskInfo info;
};

std::function<void(glm::vec2, glm::vec2)> MakeConstructibleSetTileApplicator(const SetTileApplicatorParams& params) {
    return [params](glm::vec2 p1, glm::vec2 p2) {
        p1.x = std::round(p1.x);
        p1.y = std::round(p1.y);
        p2.x = std::round(p2.x);
        p2.y = std::round(p2.y);

        if (p1.x > p2.x) {
            std::swap(p1.x, p2.x);
        }
        if (p1.y > p2.y) {
            std::swap(p1.y, p2.y);
        }

        for (int x = p1.x; x <= p2.x; x++) {
            for (int y = p1.y; y <= p2.y; y++) {
                if (World::Loaded()->GetTile(x, y).layers[params.info.layer] == params.info.newType) continue;

                if (TaskScheduler::Get().availableTaskIndices.empty()) {
                    TaskScheduler::Get().tasks.emplace_back(std::make_unique<ChangeTileTask>(glm::ivec2(x, y), params.info));
                }
                else {
                    int i = TaskScheduler::Get().availableTaskIndices.back();
                    TaskScheduler::Get().availableTaskIndices.pop_back();
                    TaskScheduler::Get().tasks[i] = std::make_unique<ChangeTileTask>(glm::ivec2(x, y), params.info);
                }

            }
        }  
    };
}

// Not neccesary actual buildings, could be various orders, just anything a player would need to select a position for
struct Constructible {
    std::string name;
    int placementMode = 2; // 0 = point, 1 = line, 2 = area
    bool snapPlacement = true;

    // function called when user tries to place a constructible on the given position.  (in charge of deciding whether that's okay and stuff)
    // For placementMode == 0, the two positions are equivalent and the position the player selected.
    // For placementMode == 1 or 2, the two positions describe a line or rectange the player selected.
    // Positions are not rounded even if snapPlacement == true.
    // trivial default function.
    std::function<void(glm::vec2, glm::vec2)> apply = [](glm::vec2, glm::vec2) {};

    // called every frame for cuurentSelectedConstructible.
    std::function<void(Constructible&)> applyGraphic = [](Constructible& c) {
        if (!currentConstructibleGhost) {
            auto params = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
            params.meshId = CubeMesh()->meshId;
            currentConstructibleGhost = GameObject::New(params);
        }

        auto transform = currentConstructibleGhost->RawGet<TransformComponent>();
        auto maybePos = GetCursorTilePos(c.snapPlacement);
        if (maybePos.has_value()) {
            transform->SetPos(glm::dvec3(maybePos->x, transform->Scale().y / 2, maybePos->y));
        }
        else {
            transform->SetPos({-1000000, -100000, -100000}); // mwahahahaha
        }
        
    };
};

void ClearCurrentConstructionGhost()
{
    if (currentConstructibleGhost) {
        currentConstructibleGhost->Destroy();
        currentConstructibleGhost = nullptr;
    }
}

void GhostBuildOnLMBDown(InputObject input) {
    if (hover != 0) return;
    if (GraphicsEngine::Get().debugFreecamEnabled) return;
    if (input.input != InputObject::LMB) return;
    if (!currentConstructibleGhost) return;

    if (currentSelectedConstructible->placementMode == 0) {
        // place the thing
        auto pos = GetCursorTilePos(false);
        if (pos.has_value())
            currentSelectedConstructible->apply(*pos, *pos);
    }
    else {
        currentBuildingP1 = GetCursorTilePos(false);
    }
    
}



std::optional<glm::vec2> GetLinearPlacementPos2(glm::vec2 p1, bool snapToGrid) {
    auto p2 = GetCursorTilePos(false);
    if (!p2.has_value()) return std::nullopt;

    // snap p2 to be on an axis-aligned line with p1 
    if (std::abs(p2->x - currentBuildingP1->x) > std::abs(p2->y - currentBuildingP1->y)) {
        p2->y = currentBuildingP1->y;
    }
    else {
        p2->x = currentBuildingP1->x;
    }
    if (currentSelectedConstructible->snapPlacement) {
        p2->x = std::round(p2->x);
        p2->y = std::round(p2->y);
    }

    return p2;
}

void GhostBuildOnLMBUp(InputObject input) {
    if (hover != 0) return;
    if (GraphicsEngine::Get().debugFreecamEnabled) return;
    if (input.input != InputObject::LMB) return;
    if (!currentConstructibleGhost) return;

    if (currentSelectedConstructible->placementMode == 1) {
        auto p2 = GetCursorTilePos(false);

        if (currentBuildingP1.has_value() && p2.has_value()) {
            if (currentSelectedConstructible->placementMode == 1) {
                currentSelectedConstructible->apply(*currentBuildingP1, *p2);
            }
            else {
                currentSelectedConstructible->apply(*currentBuildingP1, *p2);
            }
        }

        currentBuildingP1 = std::nullopt;
        //ClearCurrentConstructionGhost();
    }
}

std::vector<ConstructionTab>& GetConstructionTabs() {
    // construction menu
    static std::vector<ConstructionTab> constructionTabs = {
        ConstructionTab {
            .items = {
                Constructible {
                    .name = "Clear",
                    .placementMode = 2,

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
                    .placementMode = 1,
                    .apply = MakeConstructibleSetTileApplicator(SetTileApplicatorParams {.info = ChangeTileTaskInfo {
                            .layer = TileLayer::Furniture,
                            .newType = World::TERRAIN_IDS().TREE,
                            .baseTimeToComplete = 1.0
                    }}),
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
    return constructionTabs;
}

void MakeGameMenu() {

    
    auto& constructionTabs = GetConstructionTabs();

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
            hover++;
            p->GetTextInfo().rgba = { 1, 1, 0, 1 };
            p->UpdateGuiGraphics();
            GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemSelectionCursor);
            });
        tab->onMouseExit->Connect([p = tab.get()]() {
            hover--;
            p->GetTextInfo().rgba = { 0, 0, 1, 1 };
            p->UpdateGuiGraphics();
            GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemPointerCursor);
            });

        tab->onInputBegin->Connect([p = tab.get(), tabInfo = &tabInfo, tabIndex](const InputObject& input) mutable {
            

            if (input.input != InputObject::LMB) return;

            if (currentConstructionTabIndex == tabIndex) {
                currentConstructionTabIndex = -1;
                currentConstructionTab = nullptr;
                currentSelectedConstructible = nullptr;
                ClearCurrentConstructionGhost();
                return;
            }
            else {
                if (currentConstructionTab) {
                    currentConstructionTab = nullptr;
                    currentSelectedConstructible = nullptr;
                    ClearCurrentConstructionGhost();
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
                for (auto& item : tabInfo->items) {
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
                        hover++;
                        p->rgba = { 0.3, 0.3, 0.3, 1.0 };
                        p->UpdateGuiGraphics();
                        GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemSelectionCursor);
                    });
                    construction->onMouseExit->Connect([p = construction.get()]() {
                        hover--;
                        p->rgba = { 0.4, 0.4, 0.4, 1.0 };
                        p->UpdateGuiGraphics();
                        GraphicsEngine::Get().window.UseCursor(GraphicsEngine::Get().window.systemPointerCursor);
                    });

                    construction->onInputBegin->Connect([item = &item](InputObject input) mutable {
                        if (input.input == InputObject::LMB) {
                            if (currentSelectedConstructible) {
                                if (item == currentSelectedConstructible) {
                                    currentSelectedConstructible = nullptr;
                                    ClearCurrentConstructionGhost();
                                    InputStack::Get().PopBegin(InputObject::LMB, PlaceLMBName);
                                    InputStack::Get().PopEnd(InputObject::LMB, PlaceLMBName);
                                    return;
                                }
                                currentSelectedConstructible = nullptr;
                            }

                            ClearCurrentConstructionGhost();

                            currentSelectedConstructible = item;
                            InputStack::Get().PushBegin(InputObject::LMB, PlaceLMBName, GhostBuildOnLMBDown);
                            InputStack::Get().PushEnd(InputObject::LMB, PlaceLMBName, GhostBuildOnLMBUp);
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
                    hover++;
                    InputStack::Get().PopBegin(InputObject::Scroll, GUI_SCROLL_NAME);

                    InputStack::Get().PushBegin(InputObject::Scroll, GUI_SCROLL_NAME, [p2](InputObject input) {
                        if (input.input == InputObject::Scroll) {
                            buildUiScrollAmt = std::min(input.direction.y * 15 + buildUiScrollAmt, 0.0f);
                            p2->offsetPos.y = buildUiScrollAmt;
                            p2->UpdateGuiTransform();
                        }
                    });
                });

                buildingsList->onMouseExit->Connect([]() {
                    hover--;
                    InputStack::Get().PopBegin(InputObject::Scroll, GUI_SCROLL_NAME);
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



    // building placement
    GraphicsEngine::Get().preRenderEvent->Connect([](float dt) {
        if (currentSelectedConstructible && hover == 0) {
            currentSelectedConstructible->applyGraphic(*currentSelectedConstructible);
        }
    });
}

void CleanupMenu()
{
    ClearCurrentConstructionGhost();
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
            //auto cre = Creature::New(CubeMesh(), Body::Humanoid());
            auto cre = Humanoid::New();
            cre->gameObject->RawGet<TransformComponent>()->SetPos({ -13, 1.0, 10 });
            //cre->MoveTo({ -13, -13 });
        }
        });

    startGame->UpdateGuiGraphics();
    startGame->UpdateGuiTransform();
    startGame->UpdateGuiText();

}