#include <ui/speech_planner.h>
#include <managers/source_man.h>

#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

using namespace r2;

namespace rosen {
	inline GLuint textureId(texture_buffer* tex) {
		return ((gl_render_driver*)r2engine::renderer()->driver())->get_texture_id(tex);
	}

	speech_planner::speech_planner(source_man* smgr, scene* s) {
		m_scene = s;
		m_mgr = smgr;
		m_plan = new speech_plan();
		m_visualizePlan = nullptr;
		m_execution = nullptr;
		memset(m_wordSearch, 0, 1024);
		memset(m_premixText, 0, 64);

		m_audio = new audio_source(NO_AUDIO_BUFFER);
		m_texture = s->create_texture();
		m_texture->create(1, 1, 3, tt_unsigned_byte);
	}

	speech_planner::~speech_planner() {
		delete m_audio;
		m_scene->destroy(m_texture);
		if (m_plan) delete m_plan;
		if (m_execution) delete m_execution;
	}

	void speech_planner::premixes_modified() {
		m_mgr->save_premixes();
	}

	void speech_planner::update(f32 frameDt, f32 updateDt) {
		if (m_audio->isPlaying()) {
			if (m_execution) {
				m_execution->update(m_audio, m_texture);
				if (m_execution->completed) {
					delete m_execution; m_execution = nullptr;
					if (m_visualizePlan) delete m_visualizePlan; m_visualizePlan = nullptr;
					m_audio->stop();
				}
			}
		}
	}

	void speech_planner::render(bool* isOpen) {
		if (!isOpen || !(*isOpen)) return;

		bool prevValue = *isOpen;
		if (ImGui::Begin("Speech Planner", isOpen)) {
			ImGui::Columns(2);
			f32 width = ImGui::GetColumnWidth();
			ImGui::PushItemWidth(width - 10.0f);
			ImGui::InputText("##sp_search", m_wordSearch, 1024);

			ImGui::BeginChild("##sp_results");
			mstring search = m_wordSearch;
			for (u16 i = 0;i < search.length();i++) search[i] = tolower(search[i]);

			struct list_result {
				mstring source;
				mstring text;
				speech_plan* premixed;
				source_content* snip_source;
				u32 snip_idx;
			};
			mvector<list_result> results;
			for (u32 s = 0;s < m_mgr->source_count();s++) {
				source_content* src = m_mgr->source(s);
				for (u32 sn = 0;sn < src->snippets.size();sn++) {
					source_content::snippet& snip = src->snippets[sn];
					if (snip.text.find(search) == mstring::npos) continue;

					results.push_back({ src->name(), snip.text, nullptr, src, sn });
				}
			}

			for (u32 p = 0;p < m_mgr->mixedWords.size();p++) {
				if (m_mgr->mixedWords[p].text.find(search) == mstring::npos) continue;
				results.push_back({ "Pre-mixed", m_mgr->mixedWords[p].text, m_mgr->mixedWords[p].plan, nullptr, 0 });
			}


			ImGui::Columns(3);
			for (u32 r = 0;r < results.size();r++) {
				list_result& result = results[r];
				ImGui::Text(result.source.c_str());
				ImGui::NextColumn();
				ImGui::Text(result.text.c_str());
				ImGui::NextColumn();

				char bid[32] = { 0 };
				snprintf(bid, 32, "Play##sp_r_p%d", r);
				ImGui::PushItemWidth(80.0f);
				if (ImGui::Button(bid)) {
					if (m_visualizePlan) delete m_visualizePlan; m_visualizePlan = nullptr;
					if (m_execution) delete m_execution; m_execution = nullptr;

					m_visualizePlan = new speech_plan();
					if (result.snip_source) m_visualizePlan->add(result.snip_source, result.snip_idx);
					else m_visualizePlan->append(result.premixed);

					m_execution = new speech_execution_context(m_visualizePlan);
					m_audio->stop();
					m_audio->buffer(m_visualizePlan->snippets[0]->source->audio());
					m_audio->setPlayPosition(m_visualizePlan->snippets[0]->source->snippets[m_visualizePlan->snippets[0]->snippetIdx].start);
					m_audio->play();
				}

				memset(bid, 0, 32);
				snprintf(bid, 32, "Add##sp_r_a%d", r);
				ImGui::SameLine(0, 10.0f);
				if (ImGui::Button(bid)) {
					if (result.premixed) m_plan->append(result.premixed);
					else m_plan->add(result.snip_source, result.snip_idx);
				}
				ImGui::PopItemWidth();

				ImGui::NextColumn();
			}
			ImGui::EndChild();

			ImGui::PopItemWidth();

			ImGui::NextColumn();
			ImGui::SetColumnWidth(1, 500.0f);

			ImGui::Image((void*)textureId(m_texture), ImVec2(480, 270));

			if (ImGui::Button("Execute Plan##sp_ep")) {
				if (m_execution) delete m_execution;
				m_execution = new speech_execution_context(m_plan);
				m_audio->play();
			}

			ImGui::InputText("##sp_pni", m_premixText, 64);

			if (ImGui::Button("Save As Premix##sp_sap") && m_premixText[0]) {
				speech_plan* p = new speech_plan();
				p->append(m_plan);

				premixed_word w = { p, mstring(m_premixText) };
				m_mgr->mixedWords.push_back(w);
				memset(m_premixText, 0, 64);

				premixes_modified();
			}

			width = ImGui::GetColumnWidth();
			ImGui::PushItemWidth(width - 20.0f);
			ImGui::BeginChild("##sp_pl");
				ImGui::Columns(3);

				for (u32 s = 0;s < m_plan->snippets.size();s++) {
					source_content* src = m_plan->snippets[s]->source;
					source_content::snippet& snip = src->snippets[m_plan->snippets[s]->snippetIdx];
					ImGui::Text(src->name().c_str());
					ImGui::NextColumn();
					ImGui::Text(snip.text.c_str());
					ImGui::NextColumn();

					char bid[32] = { 0 };
					snprintf(bid, 32, "Play##sp_pl_%d", s);
					if (ImGui::Button(bid)) {
						if (m_visualizePlan) delete m_visualizePlan; m_visualizePlan = nullptr;
						if (m_execution) delete m_execution; m_execution = nullptr;
						m_visualizePlan = new speech_plan();
						m_visualizePlan->add(src, m_plan->snippets[s]->snippetIdx);
						m_execution = new speech_execution_context(m_visualizePlan);
						m_audio->stop();
						m_audio->buffer(src->audio());
						m_audio->setPlayPosition(snip.start);
						m_audio->play();
					}

					memset(bid, 0, 32);
					snprintf(bid, 32, "x##sp_pl_%d", s);
					ImGui::SameLine(0, 10.0f);
					if (ImGui::Button(bid)) {
						m_plan->snippets.remove(s);
					}

					ImGui::NextColumn();
				}
			ImGui::EndChild();
			ImGui::PopItemWidth();
		}


		if (prevValue && !(*isOpen)) {
			if (m_audio->isPlaying()) m_audio->stop();
			if (m_execution) delete m_execution; m_execution = nullptr;
			if (m_plan) delete m_plan; m_plan = nullptr;
		}

		ImGui::End();
	}
};