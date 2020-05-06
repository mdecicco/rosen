#include <ui/space_browser.h>
#include <managers/space_man.h>
#include <entities/space_collision_element.h>
#include <entities/space_element.h>
#include <entities/space_light_element.h>
#include <entities/rosen_cam.h>

#include <r2/utilities/imgui/imgui.h>
using namespace r2;

namespace rosen {
	space_browser::space_browser(space_man* smgr) {
		m_mgr = smgr;
		m_selectedSpaceIdx = -1;
	}

	space_browser::~space_browser() {
	}

	void space_browser::update(r2::f32 frameDt, r2::f32 updateDt) {
	}

	void space_browser::render(bool* isOpen) {
		if (!*isOpen) return;
		u32 sc = m_mgr->space_count();

		ImGui::Begin("Space Browser", isOpen);
			ImVec2 ws = ImGui::GetWindowSize();
			for (u32 i = 0;i < sc;i++) {
				rosen_space* space = m_mgr->space(i);
				const mstring& name = space->name();
				ImGui::SetNextTreeNodeOpen(i == m_selectedSpaceIdx);
				if (ImGui::CollapsingHeader(name.c_str())) {
					if (m_selectedSpaceIdx != i) m_mgr->load_space(space);
					m_selectedSpaceIdx = i;
					ImGui::Indent(10.0f);

					if (ImGui::CollapsingHeader("Lights")) {
						ImGui::Indent(20.0f);
						u32 c = space->light_count();
						ImGui::PushID("lt");
						for (u32 j = 0;j < c;j++) {
							space_light_element_entity* light = space->light(j);
							if (ImGui::CollapsingHeader(light->name().c_str())) {
								ImGui::PushID(j);
								light_ui(light);
								ImGui::PopID();
							}
						}
						ImGui::PopID();
						ImGui::Unindent(20.0f);
					}

					if (ImGui::CollapsingHeader("Cameras")) {
						ImGui::Indent(20.0f);
						u32 c = space->camera_count();
						ImGui::PushID("cam");
						for (u32 j = 0;j < c;j++) {
							if (ImGui::CollapsingHeader(space->camera_name(j).c_str())) {
								ImGui::PushID(j);
								camera_ui(space->camera(j), j);
								ImGui::PopID();
							}
						}
						ImGui::PopID();
						ImGui::Unindent(20.0f);
					}

					if (ImGui::CollapsingHeader("Elements")) {
						ImGui::Indent(20.0f);
						ImGui::PushID("ele");
						u32 c = space->element_count();
						for (u32 j = 0;j < c;j++) {
							space_element_entity* ele = space->element(j);
							if (ImGui::CollapsingHeader(ele->name().c_str())) {
								ImGui::PushID(j);
								element_ui(ele);
								ImGui::PopID();
							}
						}
						ImGui::PopID();
						ImGui::Unindent(20.0f);
					}

					if (ImGui::CollapsingHeader("Colliders")) {
						ImGui::Indent(20.0f);
						ImGui::PushID("col");
						u32 c = space->collider_count();
						for (u32 j = 0;j < c;j++) {
							space_collision_element_entity* col = space->collider(j);
							if (ImGui::CollapsingHeader(col->name().c_str())) {
								ImGui::PushID(j);
								collider_ui(col);
								ImGui::PopID();
							}
						}
						ImGui::PopID();
						ImGui::Unindent(20.0f);
					}

					if (ImGui::CollapsingHeader("Points of Interest")) {
						ImGui::Indent(20.0f);
						ImGui::PushID("poi");
						auto poi_names = space->point_of_interest_names();
						for (u32 j = 0;j < poi_names.size();j++) {
							bool dummy = false;
							if (ImGui::Selectable(poi_names[j].c_str(), &dummy)) poi_clicked(poi_names[j]);
						}
						ImGui::PopID();
						ImGui::Unindent(20.0f);
					}

					ImGui::Unindent(10.0f);
				}
			}
		ImGui::End();
	}

	void space_browser::element_ui(space_element_entity* element) {
	}

	void space_browser::collider_ui(space_collision_element_entity* collider) {
	}

	void space_browser::light_ui(space_light_element_entity* light) {
		lighting_component* c = light->lighting.get();
		ImGui::ColorPicker3("Color", &c->color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar);
		if (ImGui::DragFloat("Inner Cone", &c->coneInnerAngle, 0.5f, 0.0f, 0.0f, "%.2fº")) {
			if (c->coneInnerAngle < 0.0f) c->coneInnerAngle = 0.0f;
			else if (c->coneInnerAngle > c->coneOuterAngle) c->coneOuterAngle = c->coneInnerAngle;
		}
		if (ImGui::DragFloat("Outer Cone", &c->coneOuterAngle, 0.5f, 0.0f, 0.0f, "%.2fº")) {
			if (c->coneOuterAngle < 0.0f) c->coneOuterAngle = 0.0f;
			else if (c->coneOuterAngle > 180.0f) c->coneOuterAngle = 180.0f;
			else if (c->coneOuterAngle < c->coneInnerAngle) c->coneInnerAngle = c->coneOuterAngle;
		}
		if (ImGui::DragFloat("Constant Att.", &c->constantAttenuation, 0.001f, 0.0f, 0.0f, "%.4f")) {
			if (c->constantAttenuation < 0.0f) c->constantAttenuation = 0.0f;
		}
		if (ImGui::DragFloat("Linear Att.", &c->linearAttenuation, 0.001f, 0.0f, 0.0f, "%.4f")) {
			if (c->linearAttenuation < 0.0f) c->linearAttenuation = 0.0f;
		}
		if (ImGui::DragFloat("Quadratic Att.", &c->quadraticAttenuation, 0.001f, 0.0f, 0.0f, "%.4f")) {
			if (c->quadraticAttenuation < 0.0f) c->quadraticAttenuation = 0.0f;
		}
	}

	void space_browser::camera_ui(rosen_space::camera_node* cam, u32 idx) {
		if (ImGui::Button("Activate")) m_mgr->get_current()->set_current_camera(idx);
		bool updated = false;

		if (ImGui::Checkbox("Orthographic", &cam->isOrthographic)) {
			updated = true;
		}

		if (cam->isOrthographic) {
			if (ImGui::DragFloat("Width", &cam->width, 1.0f, 0.0f, 0.0f, "%.0f")) {
				if (cam->width < 0.0f) cam->width = 0.0f;
				updated = true;
			}
			if (ImGui::DragFloat("Height", &cam->height, 1.0f, 0.0f, 0.0f, "%.0f")) {
				if (cam->height < 0.0f) cam->height = 0.0f;
				updated = true;
			}
		} else {
			if (ImGui::DragFloat("Field of View", &cam->fieldOfView, 0.5f, 0.0f, 0.0f, "%.2fº")) {
				if (cam->fieldOfView < 0.0f) cam->fieldOfView = 0.0f;
				else if (cam->fieldOfView > 90.0f) cam->fieldOfView = 90.0f;
				updated = true;
			}
		}

		if (updated) m_mgr->get_current()->camera()->cameraProjection = cam->projection();
	}

	void space_browser::poi_clicked(const mstring& name) {
		printf("%s clicked\n", name.c_str());
	}
};