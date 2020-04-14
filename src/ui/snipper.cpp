#include <ui/snipper.h>
#include <managers/source_man.h>

#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

using namespace r2;

namespace rosen {
	inline GLuint textureId(texture_buffer* tex) {
		return ((gl_render_driver*)r2engine::renderer()->driver())->get_texture_id(tex);
	}

	source_snipper::source_snipper(source_man* smgr, scene* s) {
		m_scene = s;
		m_mgr = smgr;
		m_selectedSourceIdx = 0;
		m_source = m_mgr->source(m_selectedSourceIdx);
		m_plan = nullptr;
		m_execution = nullptr;
		memset(m_visualize, 0, 1024);
		memset(m_visualizeAll, 0, 1024);
		memset(m_snipName, 0, 256);

		m_audio = new audio_source(m_source->audio());
		m_texture = s->create_texture();
		m_texture->create(1, 1, 3, tt_unsigned_byte);
		m_times[0] = m_times[1] = 0.0f;
		m_offsets[0] = m_offsets[1] = 0.0f;
	}

	source_snipper::~source_snipper() {
		delete m_audio;
		m_scene->destroy(m_texture);
		if (m_plan) delete m_plan;
		if (m_execution) delete m_execution;
	}

	void source_snipper::snips_modified() {
		m_source->save_snippets();
	}

	void source_snipper::update(f32 frameDt, f32 updateDt) {
		if (m_audio->isPlaying()) {
			if (m_execution) {
				m_execution->update(m_audio, m_texture);
				if (m_execution->completed) {
					delete m_execution; m_execution = nullptr;
					delete m_plan; m_plan = nullptr;
					m_audio->stop();
					m_audio->buffer(m_source->audio());
				}
			} else m_source->frame(m_audio->playPosition(), m_texture);
		}
	}

	void source_snipper::render(bool* isOpen) {
		if (!isOpen || !(*isOpen)) return;

		bool prevValue = *isOpen;
		if (ImGui::Begin("Snipper", isOpen)) {
			ImGui::Columns(2);
			f32 width = ImGui::GetColumnWidth();
			f32 height = ImGui::GetWindowHeight();
			ImGui::PushItemWidth(width - 10.0f);
			auto get_name = [](void* data, i32 idx, const char** out) {
				source_man* sources = (source_man*)data;
				*out = sources->source(idx)->cname();
				return true;
			};
			if (ImGui::ListBox("##src", &m_selectedSourceIdx, get_name, m_mgr, m_mgr->source_count(), 24)) {
				m_source = m_mgr->source(m_selectedSourceIdx);
				m_audio->stop();
				m_audio->buffer(m_source->audio());
				m_audio->play();
				m_times[0] = 0.0f;
				m_times[1] = m_audio->duration();
				m_offsets[0] = m_offsets[1] = 0.0f;
				if (m_plan) delete m_plan; m_plan = nullptr;
				if (m_execution) delete m_execution; m_execution = nullptr;
			}

			ImGui::BeginChild("##snips", ImVec2(width - 10.0f, height * 0.5f));
			{
				f32 text_width = 200.0f;
				f32 end_padding = 20.0f;
				f32 play_btn_width = 80.0f;
				f32 del_btn_width = 50.0f;
				f32 spacing = 10.0f;
				f32 input_width = (width - (text_width + end_padding + play_btn_width + spacing + spacing + spacing + del_btn_width)) * 0.5f;

				for (u32 i = 0;i < m_source->snippets.size();i++) {
					source_content::snippet& snip = m_source->snippets[i];
					char bid[16] = { 0 };

					ImGui::Text("%s", snip.text.c_str());

					ImGui::SameLine(text_width);

					ImGui::PushItemWidth(input_width);
						snprintf(bid, 16, "##ws_1_%d%s", i, snip.text.c_str());
						if (ImGui::DragFloat(bid, &snip.start, 0.01f, 0.0f, m_audio->duration())) snips_modified();

						ImGui::SameLine(0.0f, spacing);

						snprintf(bid, 16, "##ws_2_%d%s", i, snip.text.c_str());
						if (ImGui::DragFloat(bid, &snip.end, 0.01f, 0.0f, m_audio->duration())) snips_modified();
					ImGui::PopItemWidth();

					snprintf(bid, 16, "Play##%d%s", i, snip.text.c_str());
					ImGui::SameLine(width - (play_btn_width + end_padding + del_btn_width + spacing));

					ImGui::PushItemWidth(play_btn_width);
					if (ImGui::Button(bid)) {
						if (m_plan) delete m_plan; m_plan = nullptr;
						if (m_execution) delete m_execution; m_execution = nullptr;
						m_plan = new speech_plan();
						m_plan->add(m_source, i);
						m_execution = new speech_execution_context(m_plan);
						m_audio->stop();
						m_audio->setPlayPosition(snip.start);
						m_audio->play();
					}
					ImGui::PopItemWidth();

					bool used_by_premix = false;

					for (u32 p = 0;p < m_mgr->mixedWords.size();p++) {
						for (u32 s = 0;s < m_mgr->mixedWords[p].plan->snippets.size();s++) {
							if (m_mgr->mixedWords[p].plan->snippets[s]->source == m_source && m_mgr->mixedWords[p].plan->snippets[s]->snippetIdx == i) {
								used_by_premix = true;
								break;
							}
						}
					}

					if (!used_by_premix) {
						snprintf(bid, 16, "x##%d%s", i, snip.text.c_str());
						ImGui::SameLine(width - (del_btn_width + end_padding));
						ImGui::PushItemWidth(del_btn_width);
						if (ImGui::Button(bid)) {
							m_source->snippets.erase(m_source->snippets.begin() + i);
							snips_modified();

							bool save_premixes = false;
							for (u32 p = 0;p < m_mgr->mixedWords.size();p++) {
								for (u32 s = 0;s < m_mgr->mixedWords[p].plan->snippets.size();s++) {
									if (m_mgr->mixedWords[p].plan->snippets[s]->source == m_source && m_mgr->mixedWords[p].plan->snippets[s]->snippetIdx > i) {
										m_mgr->mixedWords[p].plan->snippets[s]->snippetIdx--;
										save_premixes = true;
									}
								}
							}

							if (save_premixes) m_mgr->save_premixes();
						}
					}
					ImGui::PopItemWidth();
				}
			}
			ImGui::EndChild();


			ImGui::PopItemWidth();
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - 10.0f);

			ImGui::Image((void*)textureId(m_texture), ImVec2(480, 270));
			ImGui::PushItemWidth(ImGui::GetColumnWidth() - 110.0f);
			ImGui::InputText("##vis", m_visualize, 1024);
			ImGui::PopItemWidth();
			ImGui::SameLine(width - 110.0f);
			ImGui::PushItemWidth(100.0f);
			if (ImGui::Button("Visualize")) {
				if (m_visualize[0]) {
					if (m_plan) delete m_plan; m_plan = nullptr;
					if (m_execution) delete m_execution; m_execution = nullptr;
					m_plan = m_mgr->plan_speech(m_visualize, m_selectedSourceIdx);
					m_execution = new speech_execution_context(m_plan);
				} else {
					m_audio->stop();
					m_audio->setPlayPosition(m_times[0] + m_offsets[0]);
					m_audio->play();
				}
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(ImGui::GetColumnWidth() - 150.0f);
			ImGui::InputText("##visAll", m_visualizeAll, 1024);
			ImGui::PopItemWidth();
			ImGui::SameLine(width - 150.0f);
			ImGui::PushItemWidth(140.0f);
			if (ImGui::Button("Visualize All")) {
				if (m_visualizeAll[0]) {
					if (m_plan) delete m_plan; m_plan = nullptr;
					if (m_execution) delete m_execution; m_execution = nullptr;
					m_plan = m_mgr->plan_speech(m_visualizeAll);
					m_execution = new speech_execution_context(m_plan);
					if (!m_audio->isPlaying()) m_audio->play();
				} else {
					m_audio->stop();
					m_audio->setPlayPosition(m_times[0] + m_offsets[0]);
					m_audio->play();
				}
			}
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(ImGui::GetColumnWidth() - 110.0f);
			ImGui::InputText("##newname", m_snipName, 256);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(100.0f);
			if (ImGui::Button("Add")) {
				if (m_snipName[0]) {
					m_source->snippets.push_back({
						m_times[0] + m_offsets[0],
						m_times[1] + m_offsets[1],
						mstring(m_snipName)
					});
					snips_modified();
					memset(m_snipName, 0, 256);
				}
			}
			ImGui::PopItemWidth();

			f32 ppos = m_audio->playPosition();
			f32 pitch = m_audio->pitch();
			if (ImGui::DragFloat("pitch", &pitch, 0.01f)) m_audio->setPitch(pitch);

			bool changed = false;
			if (ImGui::SliderFloat("##times0", &m_times[0], 0.0f, m_audio->duration())) {
				if (m_times[0] > m_times[1]) m_times[1] = m_times[0];
				changed = true;
			}

			if (ImGui::SliderFloat("##times1", &m_times[1], 0.0f, m_audio->duration())) {
				if (m_times[1] < m_times[0]) m_times[0] = m_times[1];
				changed = true;
			}

			if (ImGui::SliderFloat("##offsets0", &m_offsets[0], -1.0f, 1.0f)) {
				if (m_times[0] + m_offsets[0] > m_times[1] + m_offsets[1]) m_offsets[0] = (m_times[1] + m_offsets[1]) - m_times[0];
				changed = true;
			}

			if (ImGui::SliderFloat("##offsets1", &m_offsets[1], -1.0f, 1.0f)) {
				if (m_times[1] + m_offsets[1] < m_times[0] + m_offsets[0]) m_offsets[1] = (m_times[0] + m_offsets[0]) - m_times[1];
				changed = true;
			}
			ImGui::PopItemWidth();

			if (changed) {
				if (m_plan) delete m_plan; m_plan = nullptr;
				if (m_execution) delete m_execution; m_execution = nullptr;
				m_audio->stop();
				m_audio->setPlayPosition(m_times[0] + m_offsets[0]);
				m_audio->play();
			}

			if (!m_execution && m_audio->playPosition() > m_times[1] + m_offsets[1]) {
				m_audio->stop();
			}
		}
		
		if (prevValue && !(*isOpen)) {
			if (m_audio->isPlaying()) m_audio->stop();
			if (m_execution) delete m_execution; m_execution = nullptr;
			if (m_plan) delete m_plan; m_plan = nullptr;
		}
		ImGui::End();
	}
};