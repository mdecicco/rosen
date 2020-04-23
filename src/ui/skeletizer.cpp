#include <ui/skeletizer.h>
#include <managers/source_man.h>
#include <utils/video.h>

#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

using namespace r2;

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
	}

	source_skeletizer::~source_skeletizer() {
		delete m_audio;
		m_scene->destroy(m_texture);
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
			ImGui::BeginChild("##sk_top", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 250.0f));
			{
				ImGui::Columns(2);
				f32 width = ImGui::GetColumnWidth();
				f32 height = ImGui::GetWindowHeight();
				ImGui::PushItemWidth(width - 10.0f);
				auto get_name = [](void* data, i32 idx, const char** out) {
					source_man* sources = (source_man*)data;
					*out = sources->source(idx)->cname();
					return true;
				};
				if (ImGui::ListBox("##src", &m_selectedSourceIdx, get_name, m_mgr, m_mgr->source_count(), height / ImGui::GetTextLineHeightWithSpacing())) {
					m_source = m_mgr->source(m_selectedSourceIdx);
					m_audio->stop();
					m_audio->buffer(m_source->audio());
					m_audio->play();
					for (u8 b = 0;b < bi_bone_count;b++) {
						m_source->bones[b].dragging = false;
						m_source->bones[b].set(0.0f, vec2f(0.0f, 0.1f + ((f32(b) / f32(bi_bone_count)) * 0.8f)));
						m_source->bones[b].set(0.0f, true);
					}
					m_source->save_bones();
				}


				ImGui::PopItemWidth();
				ImGui::NextColumn();
				width = ImGui::GetColumnWidth() - 10.0f;
				ImGui::PushItemWidth(width);


				ImVec2 imgSize = ImVec2(width, width * 0.55384615384f);
				if (m_audio->isPlaying()) m_playPos = m_audio->playPosition();
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
								m_source->bones[b].set(frameTime, !m_source->bones[b].hidden(frameTime));
								m_source->save_bones();
								foundActive = true;
							}
						}

						if (m_source->bones[b].dragging) {
							ImVec2 del = ImGui::GetMouseDragDelta(0);
							if (del.x != 0.0f || del.y != 0.0f) {
								m_source->bones[b].set(frameTime, vec2f(
									(mp.x - windowTL.x) / imgSize.x,
									(mp.y - windowTL.y) / imgSize.y
								));
							}
							foundActive = true;
						}
					}
				}

				ImGui::EndChild();

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

				ImGui::PopItemWidth();
			}
			ImGui::EndChild();

			ImGui::BeginChild("##sk_bot", ImVec2(ImGui::GetWindowContentRegionWidth(), 200.0f), true);
			{
				auto dl = ImGui::GetWindowDrawList();
				ImVec2 windowTL = ImGui::GetWindowPos();
				f32 width = ImGui::GetWindowContentRegionWidth();
				f32 dur = m_audio->duration();
				
				ImVec2 cp = ImGui::GetCursorPos();
				f32 spos = cp.x;
				cp.x += windowTL.x;
				cp.y += windowTL.y;

				auto ttopx = [width, spos, dur](f32 t) { return (width - spos) * (t / dur); };
				dl->AddLine(
					ImVec2(cp.x + ttopx(m_playPos), cp.y),
					ImVec2(cp.x + ttopx(m_playPos), cp.y + ImGui::GetWindowHeight()),
					ImColor(0.5f, 0.5f, 0.5f, 0.5f)
				);

				for (u8 b = 0;b < bi_bone_count;b++) {
					f32 voff = 2.0f + (2.0f * b) + (5.0f * b);
					
					for (auto it = m_source->bones[b].frames.begin();it != m_source->bones[b].frames.end();it++) {
						dl->AddCircle(ImVec2(cp.x + ttopx(it->time) + 2.5f, cp.y + voff + 2.5f), 2.5f, ImColor(0.5f, 0.5f, 0.5f), 3);
					}
				}
			}
			ImGui::EndChild();
		}

		if (prevValue && !(*isOpen)) {
			if (m_audio->isPlaying()) m_audio->stop();
		}
		ImGui::End();
	}
};