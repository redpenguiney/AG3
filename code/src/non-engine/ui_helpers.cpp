#include "ui_helpers.hpp"
#include "conglomerates/gui.hpp"

std::shared_ptr<WorldProgressBar> WorldProgressBar::New(glm::dvec3 initialPos) {
	return std::shared_ptr<WorldProgressBar>(new WorldProgressBar(initialPos));
}

void WorldProgressBar::SetPos(glm::dvec3 newPos) {
	pos = newPos;
	root->GetBillboardInfo().worldPosition = newPos; // no need to call UpdateGuiTransform(), billboard guis are auto-updated.
}

void WorldProgressBar::SetProgress(float newProgress) {
	progress = newProgress;
	//float percent = newProgress / (1/1);
	//bar->scaleSize = { percent, 1 };
	bar->offsetSize.x = 98 * progress;
}

WorldProgressBar::WorldProgressBar(glm::dvec3 initialPos) {
	root = std::make_shared<Gui>(false, std::make_pair(-1, GraphicsEngine::Get().defaultBillboardGuiMaterial), std::nullopt, Gui::BillboardGuiInfo{ .scaleWithDistance = true }, false);
	root->rgba = { 0, 0, 0, 1 };

	root->offsetPos = { 0, 0 };
	root->offsetSize = { 100, 20};
	//root->anchorPoint = { 0.5, 0.5 };
	root->scalePos = { 0.0, 0.0 };
	root->scaleSize = { 0.0, 0.0 };
	root->zLevel = 0;

	root->childBehaviour = GuiChildBehaviour::Relative;

	bar = std::make_shared<Gui>(false, std::make_pair(-1, GraphicsEngine::Get().defaultBillboardGuiMaterial), std::nullopt, std::nullopt, false);
	bar->rgba = { 0, 1, 0, 1 };
	bar->zLevel = 0.0;
	 
	bar->offsetPos = { -48, 0 };
	bar->offsetSize = { 0, 16 };
	bar->anchorPoint = { -0.5, 0 };
	bar->scalePos = { 0.0, 0.0 };
	bar->scaleSize = { 0.0, 0 };
	bar->guiScaleMode = Gui::ScaleXY;
	bar->SetParent(root.get());

	root->UpdateGuiGraphics();
	bar->UpdateGuiGraphics();

	SetPos(initialPos);
	SetProgress(0);
}