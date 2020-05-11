#include <ui/entity_editor.h>
#include <managers/ui_man.h>
#include <managers/space_man.h>
#include <ui/keyframe_editor.h>

#include <r2/engine.h>
using namespace r2;

#include <r2/utilities/imgui/imgui.h>
using namespace ImGui;

namespace rosen {
	entity_editor::entity_editor(space_man* smgr, ui_man* mgr) {
		m_mgr = mgr;
		m_smgr = smgr;
		m_last_entity = nullptr;
	}

	entity_editor::~entity_editor() {
	}

	void entity_editor::update(f32 frameDt, f32 updateDt) {
	}

	void entity_editor::render(bool* isOpen) {
		if (!*isOpen || !m_mgr->selectedEntity) return;
		scene_entity* e = m_mgr->selectedEntity;
		if (e != m_last_entity) {
			m_last_entity = e;
		}

		Begin((m_mgr->selectedEntity->name() + "##_eew").c_str(), isOpen);
			ImVec2 cr_mn = GetWindowContentRegionMin();
			ImVec2 cr_mx = GetWindowContentRegionMax();
			ImVec2 cr_sz = ImVec2(cr_mx.x - cr_mn.x, cr_mx.y - cr_mn.y);

			if (CollapsingHeader("Transform")) {
				if (e->transform) {
					render_transform_ui(cr_sz);
				} else {
					if (Button("Add Transform Component", ImVec2(cr_sz.x, 0.0f))) {
						transform_sys::get()->addComponentTo(e);
					}
				}
			}
			if (CollapsingHeader("Camera")) {
				if (e->camera) {
					render_camera_ui(cr_sz);
				} else {
					if (Button("Add Camera Component", ImVec2(cr_sz.x, 0.0f))) {
						camera_sys::get()->addComponentTo(e);
					}
				}
			}
			if (CollapsingHeader("Mesh")) {
				if (e->mesh) {
					render_mesh_ui(cr_sz);
				} else {
					if (Button("Add Mesh Component", ImVec2(cr_sz.x, 0.0f))) {
						mesh_sys::get()->addComponentTo(e);
					}
				}
			}
			if (CollapsingHeader("Physics")) {
				if (e->physics) {
					render_physics_ui(cr_sz);
				} else {
					if (Button("Add Physics Component", ImVec2(cr_sz.x, 0.0f))) {
						physics_sys::get()->addComponentTo(e);
					}
				}
			}
			if (CollapsingHeader("Lighting")) {
				if (e->lighting) {
					render_lighting_ui(cr_sz);
				} else {
					if (Button("Add Lighting Component", ImVec2(cr_sz.x, 0.0f))) {
						lighting_sys::get()->addComponentTo(e);
					}
				}
			}
		End();
	}

	void entity_editor::render_transform_ui(const ImVec2& size) {
		transform_component* t = m_last_entity->transform.get();
		Text("Raw Matrix View");
		InputFloat4("##_ee_m_0", &t->transform[0][0]);
		InputFloat4("##_ee_m_1", &t->transform[1][0]);
		InputFloat4("##_ee_m_2", &t->transform[2][0]);
		InputFloat4("##_ee_m_3", &t->transform[3][0]);
	}

	void entity_editor::render_camera_ui(const ImVec2& size) {
	}

	void entity_editor::render_mesh_ui(const ImVec2& size) {
	}

	void entity_editor::render_physics_ui(const ImVec2& size) {
		scene_entity* e = m_mgr->selectedEntity;
		btRigidBody* rb = e->physics->rigidBody();
		if (rb) {
			btCollisionShape* shape = rb->getCollisionShape();
			btTransform trans;
			rb->getMotionState()->getWorldTransform(trans);

			auto& ps = physics_sys::get()->physState();
			ps.enable();
			ps->world->debugDrawObject(trans, shape, btVector3(1.0f, 1.0f, 1.0f));
			ps.disable();

			f32 mass = rb->getMass();
			if (DragFloat("Mass", &mass, 0.1f)) {
				if (mass < 0.0f) mass = 0.0f;
				e->physics->set_mass(mass);
			}

			f32 restitution = rb->getRestitution();
			if (DragFloat("Restitution", &restitution, 0.1f)) {
				if (restitution < 0.0f) restitution = 0.0f;
				rb->setRestitution(restitution);
			}

			f32 fric = rb->getFriction();
			if (DragFloat("Friction", &fric, 0.1f)) {
				rb->setFriction(fric);
			}

			f32 rfric = rb->getRollingFriction();
			if (DragFloat("Rolling Friction", &rfric, 0.1f)) {
				if (rfric < 0.0f) rfric = 0.0f;
				rb->setRollingFriction(rfric);
			}

			f32 ldamp = rb->getLinearDamping();
			f32 adamp = rb->getAngularDamping();
			if (DragFloat("Linear Damping", &ldamp, 0.1f)) {
				if (ldamp < 0.0f) ldamp = 0.0f;
				rb->setDamping(ldamp, adamp);
			}

			if (DragFloat("Angular Damping", &adamp, 0.1f)) {
				if (adamp < 0.0f) adamp = 0.0f;
				rb->setDamping(ldamp, adamp);
			}

			btVector3 lf = rb->getLinearFactor();
			if (DragFloat3("Linear Factor", lf.m_floats, 0.1f)) {
				if (lf[0] < 0.0f) lf[0] = 0.0f;
				if (lf[0] > 1.0f) lf[0] = 1.0f;
				if (lf[1] < 0.0f) lf[1] = 0.0f;
				if (lf[1] > 1.0f) lf[1] = 1.0f;
				if (lf[2] < 0.0f) lf[2] = 0.0f;
				if (lf[2] > 1.0f) lf[2] = 1.0f;
				rb->setLinearFactor(lf);
			}

			btVector3 af = rb->getAngularFactor();
			if (DragFloat3("Angular Factor", af.m_floats, 0.1f)) {
				if (af[0] < 0.0f) af[0] = 0.0f;
				if (af[0] > 1.0f) af[0] = 1.0f;
				if (af[1] < 0.0f) af[1] = 0.0f;
				if (af[1] > 1.0f) af[1] = 1.0f;
				if (af[2] < 0.0f) af[2] = 0.0f;
				if (af[2] > 1.0f) af[2] = 1.0f;
				rb->setAngularFactor(af);
			}
		}
	}

	void entity_editor::render_lighting_ui(const ImVec2& size) {
		lighting_component* c = m_last_entity->lighting.get();
		SetNextItemWidth(200.0f);
		ColorPicker3("Color", &c->color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar);
		static const char* type_names[] = {
			"None",
			"Point",
			"Spot",
			"Directional"
		};
		if (BeginCombo("Type", type_names[c->type])) {
			if (Selectable(type_names[lt_none], c->type == lt_none)) c->type = lt_none;
			if (Selectable(type_names[lt_point], c->type == lt_point)) c->type = lt_point;
			if (Selectable(type_names[lt_spot], c->type == lt_spot)) c->type = lt_spot;
			if (Selectable(type_names[lt_directional], c->type == lt_directional)) c->type = lt_directional;
			EndCombo();
		}

		if (DragFloat("Inner Cone", &c->coneInnerAngle, 0.5f, 0.0f, 0.0f, "%.2fº")) {
			if (c->coneInnerAngle < 0.0f) c->coneInnerAngle = 0.0f;
			else if (c->coneInnerAngle > c->coneOuterAngle) c->coneOuterAngle = c->coneInnerAngle;
		}
		if (DragFloat("Outer Cone", &c->coneOuterAngle, 0.5f, 0.0f, 0.0f, "%.2fº")) {
			if (c->coneOuterAngle < 0.0f) c->coneOuterAngle = 0.0f;
			else if (c->coneOuterAngle > 180.0f) c->coneOuterAngle = 180.0f;
			else if (c->coneOuterAngle < c->coneInnerAngle) c->coneInnerAngle = c->coneOuterAngle;
		}
		if (DragFloat("Constant Att.", &c->constantAttenuation, 0.001f, 0.0f, 0.0f, "%.4f")) {
			if (c->constantAttenuation < 0.0f) c->constantAttenuation = 0.0f;
		}
		if (DragFloat("Linear Att.", &c->linearAttenuation, 0.001f, 0.0f, 0.0f, "%.4f")) {
			if (c->linearAttenuation < 0.0f) c->linearAttenuation = 0.0f;
		}
		if (DragFloat("Quadratic Att.", &c->quadraticAttenuation, 0.001f, 0.0f, 0.0f, "%.4f")) {
			if (c->quadraticAttenuation < 0.0f) c->quadraticAttenuation = 0.0f;
		}
	}
};