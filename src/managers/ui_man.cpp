#include <managers/ui_man.h>
#include <ui/snipper.h>
#include <ui/speech_planner.h>
#include <ui/skeletizer.h>
#include <ui/space_browser.h>
#include <ui/scene_browser.h>
#include <ui/entity_editor.h>
#include <ui/animation_editor.h>

#include <r2/engine.h>
#include <r2/utilities/debug_drawer.h>

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
		cursorWorldPosition = vec3f(0.0f);
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
				mat4f t = selectedEntity->transform->transform;
				if (selectedEntity->camera) t = glm::inverse(t);
				ImGuizmo::Manipulate(
					&camera->transform->transform[0][0],
					&camera->camera->projection()[0][0],
					m_transformationOperation,
					m_transformationSpace,
					&t[0][0]
				);

				if (selectedEntity->camera) t = glm::inverse(t);
				selectedEntity->transform->transform = t;

				if (ImGuizmo::IsUsing()) {
					if (selectedEntity->physics) {
						selectedEntity->physics->set_transform(t);
						selectedEntity->physics->rigidBody()->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
					}
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

				rosen_space* space = m_spaceMgr->get_current();
				if (space) {
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.5f), "Spawn");
					if (ImGui::Selectable("Spawn Rosen")) {
						space->spawn_rosen("Michael Rosen", glm::translate(mat4f(1.0f), cursorWorldPosition));
					}
				}
				ImGui::EndPopup();
			}
		}

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(r2engine::get()->window()->get_size().x, ImGui::GetFont()->FontSize + (ImGui::GetStyle().FramePadding.y * 2.0f)));
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

	void ui_man::draw_camera(r2::scene_entity* cam, r2::debug_drawer* draw) {
		mat4f t(1.0f);
		mat4f p(1.0f);
		if (cam->transform) t = glm::inverse(cam->transform->transform);
		if (cam->camera) {
			cam->camera->update_projection();
			p = glm::inverse(cam->camera->projection());
		}

		vec3f cube[8] = {
			vec3f(-0.5f,  0.5f,  0.5f),
			vec3f( 0.5f,  0.5f,  0.5f),
			vec3f( 0.5f,  0.5f, -0.5f),
			vec3f(-0.5f,  0.5f, -0.5f),
			vec3f(-0.5f, -0.5f,  0.5f),
			vec3f( 0.5f, -0.5f,  0.5f),
			vec3f( 0.5f, -0.5f, -0.5f),
			vec3f(-0.5f, -0.5f, -0.5f)
		};
		
		struct line {
			u8 p0;
			u8 p1;
		};
		line lines[12] = {
			{ 0, 1 },
			{ 1, 2 },
			{ 2, 3 },
			{ 3, 0 },
			{ 0, 4 },
			{ 1, 5 },
			{ 2, 6 },
			{ 3, 7 },
			{ 4, 5 },
			{ 5, 6 },
			{ 6, 7 },
			{ 7, 4 }
		};

		for (u8 l = 0;l < 12;l++) {
			vec3f body_p0 = t * vec4f(cube[lines[l].p0], 1.0f);
			vec3f body_p1 = t * vec4f(cube[lines[l].p1], 1.0f);
			vec4f frustum_p0 = p * vec4f(cube[lines[l].p0] * 2.0f, 1.0f);
			vec4f frustum_p1 = p * vec4f(cube[lines[l].p1] * 2.0f, 1.0f);
			frustum_p0 /= frustum_p0.w;
			frustum_p1 /= frustum_p1.w;

			draw->line(body_p0, body_p1, vec4f(0.7f, 0.7f, 0.7f, 1.0f));
			draw->line(t * frustum_p0, t * frustum_p1, vec4f(0.5f, 0.5f, 0.5f, 1.0f));
		}
	}
};