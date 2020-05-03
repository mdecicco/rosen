#include <ui/scene_browser.h>
#include <r2/utilities/imgui/imgui.h>

namespace rosen {
	scene_browser::scene_browser() {
		m_selectedEntityIdx = 0;
	}

	scene_browser::~scene_browser() {
	}

	void scene_browser::update(r2::f32 frameDt, r2::f32 updateDt) {
	}

	void scene_browser::render(bool* isOpen) {
		if (!*isOpen) return;
		ImGui::Begin("Scene Browser", isOpen);
		ImGui::End();
	}
};