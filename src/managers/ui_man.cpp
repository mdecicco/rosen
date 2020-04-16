#include <managers/ui_man.h>
#include <ui/snipper.h>
#include <ui/speech_planner.h>
#include <ui/skeletizer.h>

#include <r2/engine.h>

using namespace r2;

namespace rosen {
	ui_man::ui_man(source_man* smgr, scene* s) {
		m_sourceMgr = smgr;
		m_scene = s;

		m_snipper = new source_snipper(m_sourceMgr, m_scene);
		m_snipperOpen = false;

		m_skeletizer = new source_skeletizer(m_sourceMgr, m_scene);
		m_skeletizerOpen = false;

		m_planner = new speech_planner(m_sourceMgr, m_scene);
		m_plannerOpen = false;
	}

	ui_man::~ui_man() {
	}

	void ui_man::update(f32 frameDt, f32 updateDt) {
		m_snipper->update(frameDt, updateDt);
		m_skeletizer->update(frameDt, updateDt);
		m_planner->update(frameDt, updateDt);
	}

	void ui_man::render() {
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(r2engine::get()->window()->get_size().x, 30));
		ImGuiWindowFlags window_flags = 0
			| ImGuiWindowFlags_MenuBar
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus
		;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##main_window", NULL, window_flags);

		ImGui::BeginMenuBar();
			if (ImGui::BeginMenu("Windows")) {
				ImGui::MenuItem("Snipper", NULL, &m_snipperOpen);
				ImGui::MenuItem("Skeletizer", NULL, &m_skeletizerOpen);
				ImGui::MenuItem("Speech Planner", NULL, &m_plannerOpen);
				ImGui::EndMenu();
			}
		ImGui::EndMenuBar();

		ImGui::End();
		ImGui::PopStyleVar(3);

		m_snipper->render(&m_snipperOpen);
		m_skeletizer->render(&m_skeletizerOpen);
		m_planner->render(&m_plannerOpen);
	}
};