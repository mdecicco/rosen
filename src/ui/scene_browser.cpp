#include <ui/scene_browser.h>
#include <managers/ui_man.h>
#include <r2/engine.h>
#include <r2/utilities/imgui/imgui.h>

using namespace r2;
using namespace ImGui;

namespace rosen {
	scene_browser::scene_browser(ui_man* ui) {
		m_ui = ui;
	}

	scene_browser::~scene_browser() {
	}

	void scene_browser::update(r2::f32 frameDt, r2::f32 updateDt) {
	}

	void scene_browser::render(bool* isOpen) {
		if (!*isOpen) return;
		Begin("Scene Browser", isOpen);
		mvector<scene_entity*> entities;
		r2engine::entities(entities);

		ImVec2 cr_mn = GetWindowContentRegionMin();
		ImVec2 cr_mx = GetWindowContentRegionMax();
		ImVec2 cr_sz = ImVec2(cr_mx.x - cr_mn.x, cr_mx.y - cr_mn.y);
		if (ListBoxHeader("##_sb_el", cr_sz)) {
			for (u32 i = 0;i < entities.size();i++) {
				PushID(entities[i]->id());
				if (Selectable(entities[i]->name().c_str(), entities[i] == m_ui->selectedEntity)) {
					m_ui->selectedEntity = entities[i];
				}
				PopID();
			}
			ListBoxFooter();
		}
		End();
	}
};