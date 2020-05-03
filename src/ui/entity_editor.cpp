#include <ui/entity_editor.h>
#include <r2/utilities/imgui/imgui.h>

namespace rosen {
	entity_editor::entity_editor() {
	}

	entity_editor::~entity_editor() {
	}

	void entity_editor::update(r2::f32 frameDt, r2::f32 updateDt) {
	}

	void entity_editor::render(bool* isOpen) {
		if (!*isOpen) return;
		ImGui::Begin("Entity Editor", isOpen);
		ImGui::End();
	}
};