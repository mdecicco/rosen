#include <ui/animation_editor.h>
#include <managers/ui_man.h>
#include <managers/space_man.h>
#include <ui/keyframe_editor.h>

#include <r2/engine.h>
using namespace r2;

#include <r2/utilities/imgui/imgui.h>
using namespace ImGui;

namespace rosen {
	animation_editor::animation_editor(space_man* smgr, ui_man* mgr) {
		m_mgr = mgr;
		m_smgr = smgr;
		m_last_entity = nullptr;
		m_keyframeOptions = nullptr;
		m_lastKeyframeOptions = nullptr;
		m_selectedAnimation = 0;
		memset(m_animNameBuf, 0, 64);
	}

	animation_editor::~animation_editor() {
	}

	void animation_editor::update(f32 frameDt, f32 updateDt) {
	}

	void animation_editor::render(bool* isOpen) {
		if (!*isOpen || !m_mgr->selectedEntity) return;
		scene_entity* e = m_mgr->selectedEntity;
		animation_component* comp = e->animation.get();
		if (!comp) {
			PushID("_eaew");
				Begin(m_mgr->selectedEntity->name().c_str(), isOpen);
					ImVec2 cr_mn = GetWindowContentRegionMin();
					ImVec2 cr_mx = GetWindowContentRegionMax();
					ImVec2 cr_sz = ImVec2(cr_mx.x - cr_mn.x, cr_mx.y - cr_mn.y);
					if (Button("Add Animation Component", ImVec2(cr_sz.x, 0.0f))) {
						animation_sys::get()->addComponentTo(e);
					}
				End();
			PopID();
			return;
		}


		if (e != m_last_entity) {
			m_last_entity = e;
			destroy_entity_data();
			init_entity_data();
		}

		Begin((m_mgr->selectedEntity->name() + "##_eaew").c_str(), isOpen);
		ImVec2 cr_mn = GetWindowContentRegionMin();
		ImVec2 cr_mx = GetWindowContentRegionMax();
		ImVec2 cr_sz = ImVec2(cr_mx.x - cr_mn.x, cr_mx.y - cr_mn.y);


		if (m_entityAnims.size() == 0) {
			Text("No animations");
			InputText("##_eae_an", m_animNameBuf, 64);
			SameLine(0.0f, 5.0f);
			if (Button("Add Animation", ImVec2(0.0f, 0.0f)) && m_animNameBuf[0]) {
				animation_group* anim = new animation_group(m_animNameBuf, 5.0f);
				kf::KeyframeEditorInterface* kei = new kf::KeyframeEditorInterface();
				kei->Duration = 5.0f;
				m_entityAnims.push_back(kei);
				comp->animations.push(anim);
				memset(m_animNameBuf, 0, 64);
			}
		} else {
			kf::KeyframeEditorInterface* kfe = m_entityAnims[m_selectedAnimation];
			animation_group* anim = *comp->animations[m_selectedAnimation]; 

			f32 anim_list_width = 150.0f;
			f32 anim_prop_list_width = 300.0f;
			f32 padding = 5.0f;
			f32 control_row_height = GetFont()->FontSize + (GetStyle().FramePadding.y * 2.0f);
			f32 footer_height = control_row_height;
			ImVec2 cp = GetCursorPos();

			// Animation list
			SetNextItemWidth(anim_list_width - (CalcTextSize("Add").x + (GetStyle().FramePadding.x * 2.0f)) - 5.0f);
			InputText("##_eae_an", m_animNameBuf, 64);
			SameLine(0.0f, 5.0f);
			if (Button("Add") && m_animNameBuf[0]) {
				comp->animations.push(new animation_group(m_animNameBuf, 5.0f));
				memset(m_animNameBuf, 0, 64);
			}

			SetCursorPos(ImVec2(cp.x, cp.y + control_row_height + padding));
			if (ListBoxHeader("##_ee_al", ImVec2(anim_list_width, cr_sz.y - (footer_height + padding + control_row_height + padding)))) {
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

			// Animatable Property List
			SetCursorPos(ImVec2(cp.x + anim_list_width + padding, cp.y));
			if (BeginChild("##_ee_pl", ImVec2(anim_prop_list_width, cr_sz.y - (footer_height + padding)), true)) {
				mvector<mstring> props;
				m_last_entity->animatable_props(props);

				char btn_buf[6] = { 0 };
				for (u32 p = 0;p < props.size();p++) {
					bool is_in_anim = anim->track(props[p]) != nullptr;
					Text(props[p].c_str());

					SameLine(anim_prop_list_width - 80.0f);
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

			// Keyframe editor
			kfe->CurrentTime = anim->current_time();
			SetCursorPos(ImVec2(cp.x + anim_list_width + padding + anim_prop_list_width + padding, cp.y));
			kf::KeyframeEditor(kfe, ImVec2(0.0f, cr_sz.y - (footer_height + padding)));

			// Footer
			SetCursorPos(ImVec2(cp.x, cp.y + cr_sz.y - footer_height));

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

			SameLine(cr_sz.x - 80.0f);
			rosen_space* cspace = m_smgr->get_current();
			if (cspace && Button("Save", ImVec2(80.0f, 0.0f))) save_anims();
		}

		End();

		if (m_keyframeOptions != m_lastKeyframeOptions) {
			OpenPopup("Keyframe Options");
			m_lastKeyframeOptions = m_keyframeOptions;
		}

		bool kf_options_open = IsPopupOpen("Keyframe Options");
		if (BeginPopup("Keyframe Options", ImGuiWindowFlags_NoTitleBar)) {
			static const char* itm[14] = {
				"No Interpolation",
				"Linear",
				"Ease In (quad)",
				"Ease Out (quad)",
				"Ease In/Out (quad)",
				"Ease In (cubic)",
				"Ease Out (cubic)",
				"Ease In/Out (cubic)",
				"Ease In (quart)",
				"Ease Out (quart)",
				"Ease In/Out (quart)",
				"Ease In (quint)",
				"Ease Out (quint)",
				"Ease In/Out (quint)"
			};
			if (BeginCombo("##_eae_kf_it", itm[m_keyframeOptions->interpolation_mode])) {
				for (u32 i = 0;i < 14;i++) {
					PushID(i);
					if (Selectable(itm[i], m_keyframeOptions->interpolation_mode == i)) {
						m_keyframeOptions->interpolation_mode = (interpolate::interpolation_transition_mode)i;
						m_keyframeOptions->interpolation_factor_cb = interpolate::from_enum((interpolate::interpolation_transition_mode)i);
					}
					PopID();
				}
				EndCombo();
			}

			EndPopup();
		} else m_keyframeOptions = m_lastKeyframeOptions = nullptr;
	}

	void animation_editor::save_anims() {
		data_container* out = r2engine::files()->create(DM_BINARY, m_last_entity->name() + ".anim");
		if (out) {
			animation_component* comp = m_last_entity->animation.get();
			rosen_space* cspace = m_smgr->get_current();
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

	void animation_editor::init_entity_data() {
		if (!m_last_entity) return;
		animation_component* anim = m_last_entity->animation.get();
		if (anim) {
			m_selectedAnimation = 0;
			anim->animations.for_each([this](animation_group** _anim) {
				animation_group* anim = *_anim;
				kf::KeyframeEditorInterface* kei = new kf::KeyframeEditorInterface();
				kei->callback_userdata = this;
				kei->Duration = anim->duration();

				for (u32 i = 0;i < anim->track_count();i++) {
					animation_track_base* track = anim->track(i);
					kei->AddTrack(track->name, ImColor(1.0f, 1.0f, 1.0f, 0.7f), track);
					kf::KeyframeTrackBase* ke_track = kei->Track(i);
					for (auto it = track->keyframes.begin();it != track->keyframes.end();it++) {
						ke_track->AddKeyframe((*it)->time, *it);
					}
				}

				kei->on_create_keyframe = [](void* _self, void* _track, f32 time, void*& out_keyframe) {
					animation_editor* self = (animation_editor*)_self;
					animation_component* comp = self->m_last_entity->animation.get();
					animation_group* anim = *comp->animations[self->m_selectedAnimation];
					animation_track_base* track = (animation_track_base*)_track;
					keyframe_base* kf = self->m_last_entity->create_keyframe(track->name, anim, interpolate::itm_easeInOutCubic);
					
					if (kf) out_keyframe = kf;
					else return false;

					return true;
				};

				kei->on_delete_keyframe = [](void* _self, void* _track, void* _keyframe) {
					animation_track_base* track = (animation_track_base*)_track;
					keyframe_base* kf = (keyframe_base*)_keyframe;

					for (auto it = track->keyframes.begin();it != track->keyframes.end();it++) {
						if ((*it) == kf) {
							if (track->last_keyframe == it) track->last_keyframe = track->keyframes.end();
							track->keyframes.erase(it);
							return true;
						}
					}

					return false;
				};

				kei->on_set_keyframe_time = [](void* _self, void* _track, void* _keyframe, f32 time) {
					animation_track_base* track = (animation_track_base*)_track;

					keyframe_base* kf = (keyframe_base*)_keyframe;
					kf->time = time;

					// delete overlapped keyframe, if one exists
					for (auto it = track->keyframes.begin();it != track->keyframes.end();it++) {
						if ((*it) == _keyframe) continue;
						if (time > (*it)->time - 0.0001f && time < (*it)->time + 0.0001f) {
							if (track->last_keyframe == it) track->last_keyframe = track->keyframes.end();
							delete *it;
							track->keyframes.erase(it);
							break;
						}
					}

					return true;
				};

				kei->on_keyframe_reorder = [](void* _self, void* _track, const vector<void*>& _keyframes) {
					animation_track_base* track = (animation_track_base*)_track;
					track->keyframes.clear();
					track->last_keyframe = track->keyframes.end();

					for (u32 i = 0;i < _keyframes.size();i++) {
						track->keyframes.push_back((keyframe_base*)_keyframes[i]);
					}
				};

				kei->on_seek = [](void* _self, f32 time) {
					animation_editor* self = (animation_editor*)_self;
					animation_component* comp = self->m_last_entity->animation.get();
					animation_group* anim = *comp->animations[self->m_selectedAnimation];
					anim->set_time(time);
					anim->play();
					anim->update(0.0f, self->m_last_entity);
					anim->pause();
				};

				kei->keyframe_context_menu_items.push_back({
					"Options",
					[](void* _self, void* _track, void* _keyframe) {
						animation_editor* self = (animation_editor*)_self;
						keyframe_base* kf = (keyframe_base*)_keyframe;
						self->m_keyframeOptions = kf;
					}
				});

				m_entityAnims.push_back(kei);
				return true;
			});
		}
	}

	void animation_editor::destroy_entity_data() {
		for (auto it = m_entityAnims.begin();it != m_entityAnims.end();it++) delete *it;
		m_entityAnims.clear();
	}
};