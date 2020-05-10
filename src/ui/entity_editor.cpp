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
		m_selectedAnimation = 0;
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
			destroy_entity_data();
			init_entity_data();
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
			if (CollapsingHeader("Animation")) {
				if (e->animation) {
					render_animation_ui(cr_sz);
				} else {
					if (Button("Add Animation Component", ImVec2(cr_sz.x, 0.0f))) {
						animation_sys::get()->addComponentTo(e);
					}
				}
			}
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
		lighting_component* c = m_last_entity->lighting.get();
		PushItemWidth(200.0f);
		ImGui::ColorPicker3("Color", &c->color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar);
		PopItemWidth();
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

	void entity_editor::render_animation_ui(const ImVec2& size) {
		if (m_entityAnims.size() == 0) {
			Text("No animations");
			return;
		} else {
			animation_component* comp = m_last_entity->animation.get();
			kf::KeyframeEditorInterface* kfe = m_entityAnims[m_selectedAnimation];
			animation_group* anim = *comp->animations[m_selectedAnimation]; 

			if (BeginChild("##_ee_ac", ImVec2(0.0f, 150.0f))) {
				ImVec2 cp = GetCursorPos();
				ImVec2 c_cr_mn = GetWindowContentRegionMin();
				ImVec2 c_cr_mx = GetWindowContentRegionMax();
				ImVec2 controls_size = ImVec2(c_cr_mx.x - c_cr_mn.x, c_cr_mx.y - c_cr_mn.y);
				if (ListBoxHeader("##_ee_al", ImVec2(100.0f, controls_size.y))) {
						for (u32 a = 0;a < comp->animations.size();a++) {
							PushID(a);
							if (Selectable((*comp->animations[a])->name().c_str(), m_selectedAnimation == a)) {
								m_selectedAnimation = a;
								kfe = m_entityAnims[m_selectedAnimation];
								anim = *comp->animations[m_selectedAnimation];
							}
							PopID();
						}
					ListBoxFooter();
				} 

				mvector<mstring> props;
				m_last_entity->animatable_props(props);

				SetCursorPos(ImVec2(cp.x + 105.0f, cp.y));
				if (BeginChild("##_ee_al", ImVec2(450.0f, controls_size.y))) {
					char btn_buf[6] = { 0 };
					for (u32 p = 0;p < props.size();p++) {
						bool is_in_anim = anim->track(props[p]) != nullptr;
						Text(props[p].c_str());
						if (is_in_anim) {
							SameLine(450.0f - 160.0f);
							memset(btn_buf, 0, 6);
							snprintf(btn_buf, 6, "kf_%d", p);
							PushID(btn_buf);
							if (Button("Key", ImVec2(75.0f, 20.0f))) {
								kfe->Track(props[p])->AddKeyframe(
									anim->current_time(), 
									m_last_entity->create_keyframe(props[p], anim, interpolate::itm_easeInOutCubic)
								);
							}
							PopID();
						}

						SameLine(450.0f - 80.0f);
						memset(btn_buf, 0, 6);
						snprintf(btn_buf, 6, "ar_%d", p);
						PushID(btn_buf);
						if (Button(is_in_anim ? "Remove" : "Add", ImVec2(75.0f, 20.0f))) {
							if (is_in_anim) {
								anim->remove_track(props[p]);
								kfe->RemoveTrack(props[p]);
							}
							else {
								m_last_entity->animate_prop(props[p], anim);
								kfe->AddTrack(props[p], ImColor(1.0f, 1.0f, 1.0f, 0.7f), anim->track(props[p]));
							}
						}
						PopID();
					}

					EndChild();
				}
				EndChild();
			}

			rosen_space* cspace = m_smgr->get_current();
			if (cspace && Button("Save")) {
				data_container* out = r2engine::files()->create(DM_BINARY, m_last_entity->name() + ".anim");
				if (out) {
					char hdr[4] = { 'A', 'N', 'I', 'M' };
					bool failed = false;
					if (failed || !out->write_data(hdr, 4)) failed = true;

					u8 anim_count = comp->animations.size();
					if (failed || !out->write(anim_count)) failed = true;

					for (u8 i = 0;i < anim_count && !failed;i++) {
						if (failed || !(*comp->animations[i])->serialize(out)) failed = true;
					}

					if (!failed) {
						if (!r2engine::files()->save(out, "./resources/space/" + cspace->name() + "/anim/" + out->name())) {
							r2Error("Failed to save animation");
						}
					}

					r2engine::files()->destroy(out);
				} else {
					r2Error("Failed to create output data");
				}
			}

			SameLine(0.0f, 10.0f);
			if (Button(anim->playing() ? "Pause" : "Play")) {
				if (anim->playing()) anim->pause();
				else anim->play();
			}

			bool loops = anim->loops();
			SameLine(0.0f, 10.0f);
			if (Checkbox("Loops", &loops)) anim->loops(loops);

			f32 dur = anim->duration();
			SameLine(0.0f, 10.0f);
			if (DragFloat("Duration", &dur, 0.1f)) {
				anim->duration(dur);
				kfe->Duration = anim->duration();
			}

			kfe->CurrentTime = anim->current_time();

			if (kf::KeyframeEditor(kfe, ImVec2(0.0f, 0.0f))) {
				for (u32 i = 0;i < kfe->TrackCount();i++) {
					kf::KeyframeTrackBase* track = kfe->Track(i);
					animation_track_base* a_track = (animation_track_base*)track->user_pointer;
					mlist<keyframe_base*> old_keys = a_track->keyframes;
					a_track->keyframes.clear();
					for (auto k = track->keyframes.begin();k != track->keyframes.end();k++) {
						keyframe_base* kb = (keyframe_base*)(*k)->user_pointer;
						kb->time = (*k)->time;
						a_track->keyframes.push_back(kb);
					}

					for (auto ok = old_keys.begin();ok != old_keys.end();ok++) {
						bool found = false;
						for (auto nk = a_track->keyframes.begin();nk != a_track->keyframes.end();nk++) {
							if (*nk == *ok) {
								found = true;
								break;
							}
						}

						if (!found) delete *ok;
					}
					a_track->last_keyframe = a_track->keyframes.end();
					a_track->last_time = 0.0f;
				}
			}
			if (kfe->CurrentTime != anim->current_time()) {
				anim->set_time(kfe->CurrentTime);
				anim->play();
				anim->update(0.0f, m_last_entity);
				anim->pause();
			}
		}
	}

	void entity_editor::init_entity_data() {
		if (!m_last_entity) return;
		animation_component* anim = m_last_entity->animation.get();
		if (anim) {
			m_selectedAnimation = 0;
			anim->animations.for_each([this](animation_group** _anim) {
				animation_group* anim = *_anim;
				kf::KeyframeEditorInterface* kei = new kf::KeyframeEditorInterface();
				kei->Duration = anim->duration();

				for (u32 i = 0;i < anim->track_count();i++) {
					animation_track_base* track = anim->track(i);
					kei->AddTrack(track->name, ImColor(1.0f, 1.0f, 1.0f, 0.7f), track);
					kf::KeyframeTrackBase* ke_track = kei->Track(i);
					for (auto it = track->keyframes.begin();it != track->keyframes.end();it++) {
						ke_track->AddKeyframe((*it)->time, *it);
					}
				}

				m_entityAnims.push_back(kei);
				return true;
			});
		}
	}

	void entity_editor::destroy_entity_data() {
		for (auto it = m_entityAnims.begin();it != m_entityAnims.end();it++) delete *it;
		m_entityAnims.clear();
	}
};