#include "ui_helpers.hpp"
#include "conglomerates/gui.hpp"

std::shared_ptr<WorldProgressBar> WorldProgressBar::New(glm::dvec3 initialPos) {
	return std::shared_ptr<WorldProgressBar>(new WorldProgressBar(initialPos));
}

void WorldProgressBar::SetPos(glm::dvec3 newPos) {
	pos = newPos;
}

void WorldProgressBar::SetProgress(float newProgress) {
	progress = newProgress;
}

WorldProgressBar::WorldProgressBar(glm::dvec3 initialPos) {
	root = std::make_shared<Gui>(false, std::nullopt, std::nullopt, Gui::BillboardGuiInfo{ .scaleWithDistance = true }, false);

	SetPos(initialPos);
	SetProgress(0);
}