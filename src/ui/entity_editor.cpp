#include <ui/entity_editor.h>
#include <managers/ui_man.h>

#include <r2/engine.h>
using namespace r2;

#include <r2/utilities/imgui/imgui.h>
using namespace ImGui;

namespace rosen {
	entity_editor::entity_editor(ui_man* mgr) {
		m_mgr = mgr;
		m_last_entity = nullptr;
		m_selectedComponent = 0;
	}

	entity_editor::~entity_editor() {
	}

	void entity_editor::update(f32 frameDt, f32 updateDt) {
	}

	void entity_editor::render(bool* isOpen) {
		if (!*isOpen || !m_mgr->selectedEntity) return;
		scene_entity* e = m_mgr->selectedEntity;
		if (e != m_last_entity) {
			m_selectedComponent = 0;
			m_last_entity = e;
		}

		Begin((m_mgr->selectedEntity->name() + "##_eew").c_str(), isOpen);
			ImVec2 cr_min = GetWindowContentRegionMin();
			ImVec2 cr_max = GetWindowContentRegionMax();
			ImVec2 cr_tl = GetCursorPos();

			ListBoxHeader("##_ee_comps", ImVec2(100.0f, cr_max.y - cr_min.y));
				if (e->transform && Selectable("Transform", m_selectedComponent == 1)) m_selectedComponent = 1;
				if (e->camera && Selectable("Camera", m_selectedComponent == 2)) m_selectedComponent = 2;
				if (e->mesh && Selectable("Render", m_selectedComponent == 3)) m_selectedComponent = 3;
				if (e->physics && Selectable("Physics", m_selectedComponent == 4)) m_selectedComponent = 4;
				if (e->lighting && Selectable("Lighting", m_selectedComponent == 5)) m_selectedComponent = 5;
				if (e->animation && Selectable("Animation", m_selectedComponent == 6)) m_selectedComponent = 6;
			ListBoxFooter();

			SetCursorPos(ImVec2(cr_tl.x + 105.0f, cr_tl.y));
			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImVec2 csz = ImVec2((cr_max.x - cr_min.x) - 105.0f, cr_max.y - cr_min.y);
			BeginChild("##_ee_co", csz, true);
				switch (m_selectedComponent) {
					case 1: render_transform_ui(csz); break;
					case 2: render_camera_ui(csz); break;
					case 3: render_mesh_ui(csz); break;
					case 4: render_physics_ui(csz); break;
					case 5: render_lighting_ui(csz); break;
					case 6: render_animation_ui(csz); break;
					default: break;
				}
			EndChild();
			PopStyleVar();
		End();
	}

	void entity_editor::render_transform_ui(const ImVec2& size) {
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
	}

	void entity_editor::render_animation_ui(const ImVec2& size) {
	}
};