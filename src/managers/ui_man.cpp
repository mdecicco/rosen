#include <managers/ui_man.h>
#include <ui/snipper.h>
#include <ui/speech_planner.h>
#include <ui/skeletizer.h>
#include <ui/space_browser.h>
#include <ui/scene_browser.h>
#include <ui/entity_editor.h>
#include <ui/animation_editor.h>

#include <r2/engine.h>

using namespace r2;

namespace rosen {
	ui_man::ui_man(source_man* smgr, space_man* spmgr, scene* s) {
		m_sourceMgr = smgr;
		m_spaceMgr = spmgr;
		m_scene = s;
		selectedEntity = nullptr;
		rightClickedEntity = nullptr;

		m_snipper = new source_snipper(m_sourceMgr, m_scene);
		snipperOpen = false;

		m_skeletizer = new source_skeletizer(m_sourceMgr, m_scene);
		skeletizerOpen = false;

		m_planner = new speech_planner(m_sourceMgr, m_scene);
		plannerOpen = false;

		m_spaceBrowser = new space_browser(m_spaceMgr, this);
		spaceBrowserOpen = false;

		m_sceneBrowser = new scene_browser(this);
		sceneBrowserOpen = false;

		m_entityEditor = new entity_editor(m_spaceMgr, this);
		entityEditorOpen = false;

		m_animationEditor = new animation_editor(m_spaceMgr, this);
		animationEditorOpen = false;

		ImGuizmo::Enable(true);
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		m_transformationSpace = ImGuizmo::MODE::WORLD;
		m_transformationOperation = ImGuizmo::OPERATION::TRANSLATE;
	}

	ui_man::~ui_man() {
		delete m_snipper;
		delete m_skeletizer;
		delete m_planner;
		delete m_spaceBrowser;
		delete m_sceneBrowser;
		delete m_entityEditor;
		delete m_animationEditor;
		ImGuizmo::Enable(false);
	}

	void ui_man::update(f32 frameDt, f32 updateDt) {
		m_snipper->update(frameDt, updateDt);
		m_skeletizer->update(frameDt, updateDt);
		m_planner->update(frameDt, updateDt);
		m_spaceBrowser->update(frameDt, updateDt);
		m_sceneBrowser->update(frameDt, updateDt);
		m_entityEditor->update(frameDt, updateDt);
		m_animationEditor->update(frameDt, updateDt);
	}

	void ui_man::render() {
		ImGuizmo::BeginFrame();

		if (selectedEntity && selectedEntity->transform) {
			scene_entity* camera = r2engine::current_scene()->camera;
			if (camera) {
				ImGuizmo::Manipulate(
					&camera->transform->transform[0][0],
					&camera->camera->projection()[0][0],
					m_transformationOperation,
					m_transformationSpace,
					&selectedEntity->transform->transform[0][0]
				);

				if (ImGuizmo::IsUsing()) {
				}
			}
		}

		if (rightClickedEntity) {
			if (ImGui::BeginPopupContextVoid("##_eopts")) {
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.5f), "Transform Operation");
				if (ImGui::Selectable("Translate")) {
					m_transformationOperation = ImGuizmo::TRANSLATE;
					rightClickedEntity = nullptr;
				}
				if (ImGui::Selectable("Rotate")) {
					m_transformationOperation = ImGuizmo::ROTATE;
					rightClickedEntity = nullptr;
				}
				if (ImGui::Selectable("Scale")) {
					m_transformationOperation = ImGuizmo::SCALE;
					rightClickedEntity = nullptr;
				}
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.5f), "Transform Space");
				if (ImGui::Selectable("World")) {
					m_transformationSpace = ImGuizmo::WORLD;
					rightClickedEntity = nullptr;
				}
				if (ImGui::Selectable("Object")) {
					m_transformationSpace = ImGuizmo::LOCAL;
					rightClickedEntity = nullptr;
				}
				ImGui::EndPopup();
			}
		}

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
				ImGui::MenuItem("Snipper", NULL, &snipperOpen);
				ImGui::MenuItem("Skeletizer", NULL, &skeletizerOpen);
				ImGui::MenuItem("Speech Planner", NULL, &plannerOpen);
				ImGui::MenuItem("Space Browser", NULL, &spaceBrowserOpen);
				ImGui::MenuItem("Scene Browser", NULL, &sceneBrowserOpen);
				ImGui::MenuItem("Entity Editor", NULL, &entityEditorOpen);
				ImGui::MenuItem("Animation Editor", NULL, &animationEditorOpen);
				ImGui::EndMenu();
			}
		ImGui::EndMenuBar();

		ImGui::End();
		ImGui::PopStyleVar(3);

		m_snipper->render(&snipperOpen);
		m_skeletizer->render(&skeletizerOpen);
		m_planner->render(&plannerOpen);
		m_spaceBrowser->render(&spaceBrowserOpen);
		m_sceneBrowser->render(&sceneBrowserOpen);
		m_entityEditor->render(&entityEditorOpen);
		m_animationEditor->render(&animationEditorOpen);
	}
};