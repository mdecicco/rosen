#include <ui/skeletizer.h>
#include <ui/keyframe_editor.h>
#include <managers/source_man.h>
#include <utils/video.h>

#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

using namespace r2;
using namespace kf;

namespace rosen {
	inline GLuint textureId(texture_buffer* tex) {
		return ((gl_render_driver*)r2engine::renderer()->driver())->get_texture_id(tex);
	}
	
	static const char* bone_names[] = {
		"head_top",
		"chin",
		"head_left",
		"head_right",
		"left_hand_bottom",
		"left_hand_top",
		"right_hand_bottom",
		"right_hand_top",
		"left_eye",
		"right_eye",
		"nose",
		"mouth",
		"left_ear",
		"right_ear"
	};




	source_skeletizer::source_skeletizer(source_man* smgr, scene* s) {
		m_scene = s;
		m_mgr = smgr;
		m_selectedSourceIdx = 0;
		m_source = m_mgr->source(m_selectedSourceIdx);
		m_playPos = 0.0f;

		m_audio = new audio_source(m_source->audio());
		m_texture = s->create_texture();
		m_texture->create(1, 1, 3, tt_unsigned_byte);

		for (u8 b = 0;b < bi_bone_count;b++) {
			m_source->bones[b].dragging = false;
			m_source->bones[b].set(0.0f, vec2f(0.0f, 0.1f + ((f32(b) / f32(bi_bone_count)) * 0.8f)));
			m_source->bones[b].set(0.0f, true);
		}
		m_source->save_bones();

		m_keyframes = nullptr;
		init_keyframe_data();
	}

	source_skeletizer::~source_skeletizer() {
		delete m_audio;
		m_scene->destroy(m_texture);
	}

	void source_skeletizer::init_keyframe_data() {
		if (m_keyframes) delete m_keyframes;

		m_keyframes = new KeyframeEditorInterface();
		m_keyframes->Duration = m_source->duration();
		for (u8 b = 0;b < bi_bone_count;b++) {
			m_keyframes->AddTrack<vec2f>(
				bone_names[b],
				m_source->bones[b].position(0.0f),
				ImColor(1.0f, 1.0f, 1.0f),
				KeyframeTrack<vec2f>::DefaultInterpolator,
				&m_source->bones[b]
			);
			KeyframeTrack<vec2f>* track = m_keyframes->Track<vec2f>(bone_names[b]);
			for (auto k = m_source->bones[b].frames.begin();k != m_source->bones[b].frames.end();k++) {
				track->AddKeyframe(k->position, k->time, k->hidden ? (void*)true : nullptr);
			}
		}
	}

	void source_skeletizer::skeletons_modified() {
		m_source->save_snippets();
	}

	void source_skeletizer::update(f32 frameDt, f32 updateDt) {
		if (m_audio->isPlaying()) {
			m_source->frame(m_audio->playPosition(), m_texture);
		}
	}

	void source_skeletizer::render(bool* isOpen) {
		if (!isOpen || !(*isOpen)) return;

		bool prevValue = *isOpen;
		if (ImGui::Begin("Skeletizer", isOpen)) {
			ImVec2 cr_mn = ImGui::GetWindowContentRegionMin();
			ImVec2 cr_mx = ImGui::GetWindowContentRegionMax();
			ImVec2 cr_sz = ImVec2(cr_mx.x - cr_mn.x, cr_mx.y - cr_mn.y);

			f32 list_width = 250.0f;		
			ImVec2 imgSize = ImVec2(cr_sz.x - list_width, (cr_sz.x - list_width) * 0.55384615384f);
			ImVec2 keyframe_editor_size = ImVec2(cr_sz.x, cr_sz.y - imgSize.y);
			ImGui::ListBoxHeader("##src", ImVec2(list_width, imgSize.y));
			for (u8 s = 0;s < m_mgr->source_count();s++) {
				auto src = m_mgr->source(s);
				bool selected = m_selectedSourceIdx == s;
				if (ImGui::Selectable(src->cname(), &selected) && !selected) {
					m_selectedSourceIdx = s;
					m_source = src;
					m_audio->stop();
					m_audio->buffer(m_source->audio());
					m_audio->play();
					for (u8 b = 0;b < bi_bone_count;b++) {
						m_source->bones[b].dragging = false;
						m_source->bones[b].set(0.0f, vec2f(0.0f, 0.1f + ((f32(b) / f32(bi_bone_count)) * 0.8f)));
						m_source->bones[b].set(0.0f, true);
					}
					m_source->save_bones();
					init_keyframe_data();
				}
			}
			ImGui::ListBoxFooter();


			if (m_audio->isPlaying()) m_playPos = m_audio->playPosition();
			ImGui::SetCursorPos(ImVec2(cr_mn.x + list_width, cr_mn.y));
			ImGui::BeginChild("##sk_im", imgSize, false, ImGuiWindowFlags_NoMove);
				auto dl = ImGui::GetWindowDrawList();
				ImVec2 windowTL = ImGui::GetWindowPos();
				ImGui::Image((void*)textureId(m_texture), imgSize);

				ImVec2 mp = ImGui::GetMousePos();
				f32 mw = ImGui::GetIO().MouseWheel;
				if (ImGui::IsWindowHovered() && mw != 0.0f) {
					m_playPos += mw * (1.0f / f32(m_source->video()->info.framerate));
					if (m_playPos < 0.0f) m_playPos = 0.0f;
					if (m_playPos > m_audio->duration()) m_playPos = m_audio->duration();
					m_audio->setPlayPosition(m_playPos);
					if (!m_audio->isPlaying()) m_source->frame(m_playPos, m_texture);
				}

				f32 frameDur = 1.0f / f32(m_source->video()->info.framerate);
				u32 frameIdx = floor(m_playPos / frameDur);
				f32 frameTime = f32(frameIdx) * frameDur;

				bool foundActive = false;
				for (u8 b = 0;b < bi_bone_count;b++) {
					ImVec2 c = ImVec2(
						windowTL.x + (m_source->bones[b].position(frameTime).x * imgSize.x),
						windowTL.y + (m_source->bones[b].position(frameTime).y * imgSize.y)
					);
					dl->AddCircle(c, 10.0f, m_source->bones[b].hidden(frameTime) ? ImColor(1.0f, 0.0f, 0.0f, 0.75f) : ImColor(0.0f, 1.0f, 0.0f, 0.75f));
					dl->AddText(ImVec2(c.x + 15.0f, c.y), ImColor(1.0f, 1.0f, 1.0f, 0.75f), bone_names[b]);

					if (ImGui::IsMouseReleased(0)) {
						if (m_source->bones[b].dragging) m_source->save_bones();
						m_source->bones[b].dragging = false;
						foundActive = true;
					}

					if (!foundActive) {
						vec2f cursor = *(vec2f*)&mp;
						vec2f bone = *(vec2f*)&c;

						if (glm::length(bone - cursor) <= 10.0f) {
							if (ImGui::IsMouseClicked(0)) {
								m_source->bones[b].dragging = true;
								foundActive = true;
							}
							if (ImGui::IsMouseClicked(1)) {
								bool hide = !m_source->bones[b].hidden(frameTime);
								m_source->bones[b].set(frameTime, hide);
								m_keyframes->SetKeyframe<vec2f>(bone_names[b], m_source->bones[b].position(frameTime), frameTime, hide ? (void*)true : nullptr);
								m_source->save_bones();
								foundActive = true;
							}
						}

						if (m_source->bones[b].dragging) {
							ImVec2 del = ImGui::GetMouseDragDelta(0);
							if (del.x != 0.0f || del.y != 0.0f) {
								bool hide = m_source->bones[b].hidden(frameTime);
								vec2f pos = vec2f(
									(mp.x - windowTL.x) / imgSize.x,
									(mp.y - windowTL.y) / imgSize.y
								);
								m_source->bones[b].set(frameTime, pos);
								m_keyframes->SetKeyframe<vec2f>(bone_names[b], pos, frameTime, hide ? (void*)true : nullptr);
							}
							foundActive = true;
						}
					}
				}
			ImGui::EndChild();
			ImGui::SetCursorPos(ImVec2(cr_mn.x, cr_mn.y + imgSize.y));

			/*
			if (ImGui::Button(m_audio->isPlaying() ? "Pause" : "Play")) {
				if (m_audio->isPlaying()) m_audio->pause();
				else m_audio->play();
			}
			f32 pitch = m_audio->pitch();
			if (ImGui::DragFloat("pitch", &pitch, 0.01f)) m_audio->setPitch(pitch);

			bool changed = false;
			if (ImGui::SliderFloat("##times0", &m_playPos, 0.0f, m_audio->duration())) {
				m_audio->setPlayPosition(m_playPos);
				if (!m_audio->isPlaying()) m_source->frame(m_playPos, m_texture);
			}

			f32 offset = 0.0f;
			if (ImGui::DragFloat("##times1", &offset, 0.001f, -1.0f, 1.0f)) {
				m_audio->setPlayPosition(m_playPos + offset);
				if (!m_audio->isPlaying()) m_source->frame(m_playPos + offset, m_texture);
			}
			*/

			m_keyframes->CurrentTime = m_playPos;
			if (KeyframeEditor(m_keyframes, keyframe_editor_size)) {
				for (u32 t = 0;t < m_keyframes->TrackCount();t++) {
					KeyframeTrack<vec2f>* track = m_keyframes->Track<vec2f>(t);
					source_content::bone* bone = (source_content::bone*)track->user_pointer;
					bone->frames.clear();
					for (auto k = track->keyframes.begin();k != track->keyframes.end();k++) {
						bone->frames.push_back({
							k->time,
							((Keyframe<vec2f>*)&(*k))->value,
							k->user_pointer != nullptr
						});
					}
				}

				m_source->save_bones();
			}

			if (m_keyframes->CurrentTime != m_playPos) {
				m_playPos = m_keyframes->CurrentTime;
				m_audio->setPlayPosition(m_playPos);
				if (!m_audio->isPlaying()) m_source->frame(m_playPos, m_texture);
			}
		}

		if (prevValue && !(*isOpen)) {
			if (m_audio->isPlaying()) m_audio->stop();
		}
		ImGui::End();
	}
};