#include <managers/ui_man.h>
#include <ui/snipper.h>
#include <ui/speech_planner.h>
#include <ui/skeletizer.h>
#include <ui/space_browser.h>
#include <ui/scene_browser.h>
#include <ui/entity_editor.h>

#include <r2/engine.h>

using namespace r2;

namespace rosen {
	ui_man::ui_man(source_man* smgr, space_man* spmgr, scene* s) {
		m_sourceMgr = smgr;
		m_spaceMgr = spmgr;
		m_scene = s;

		m_snipper = new source_snipper(m_sourceMgr, m_scene);
		m_snipperOpen = false;

		m_skeletizer = new source_skeletizer(m_sourceMgr, m_scene);
		m_skeletizerOpen = false;

		m_planner = new speech_planner(m_sourceMgr, m_scene);
		m_plannerOpen = false;

		m_spaceBrowser = new space_browser(m_spaceMgr);
		m_spaceBrowserOpen = false;

		m_sceneBrowser = new scene_browser();
		m_sceneBrowserOpen = false;

		m_entityEditor = new entity_editor();
		m_entityEditorOpen = false;
	}

	ui_man::~ui_man() {
		delete m_snipper;
		delete m_skeletizer;
		delete m_planner;
		delete m_spaceBrowser;
		delete m_sceneBrowser;
		delete m_entityEditor;
	}

	void ui_man::update(f32 frameDt, f32 updateDt) {
		m_snipper->update(frameDt, updateDt);
		m_skeletizer->update(frameDt, updateDt);
		m_planner->update(frameDt, updateDt);
		m_spaceBrowser->update(frameDt, updateDt);
		m_sceneBrowser->update(frameDt, updateDt);
		m_entityEditor->update(frameDt, updateDt);
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
				ImGui::MenuItem("Space Browser", NULL, &m_spaceBrowserOpen);
				ImGui::MenuItem("Scene Browser", NULL, &m_sceneBrowserOpen);
				ImGui::MenuItem("Entity Editor", NULL, &m_entityEditorOpen);
				ImGui::EndMenu();
			}
		ImGui::EndMenuBar();

		ImGui::End();
		ImGui::PopStyleVar(3);

		m_snipper->render(&m_snipperOpen);
		m_skeletizer->render(&m_skeletizerOpen);
		m_planner->render(&m_plannerOpen);
		m_spaceBrowser->render(&m_spaceBrowserOpen);
		m_sceneBrowser->render(&m_sceneBrowserOpen);
		m_entityEditor->render(&m_entityEditorOpen);
	}
};