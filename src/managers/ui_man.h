#pragma once
#include <r2/config.h>
#include <r2/utilities/imgui/imgui.h>
#include <ui/imguizmo.h>

namespace r2 {
	class scene;
	class scene_entity;
	class debug_drawer;
};

namespace rosen {
	class source_snipper;
	class source_skeletizer;
	class speech_planner;
	class space_browser;
	class scene_browser;
	class entity_editor;
	class animation_editor;
	class source_man;
	class space_man;

	class ui_man {
		public:
			ui_man(source_man* smgr, space_man* spmgr, r2::scene* s);
			~ui_man();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render();

			void draw_camera(r2::scene_entity* cam, r2::debug_drawer* draw);

			r2::scene_entity* selectedEntity;
			r2::scene_entity* rightClickedEntity;
			r2::vec3f cursorWorldPosition;
			bool snipperOpen;
			bool skeletizerOpen;
			bool plannerOpen;
			bool spaceBrowserOpen;
			bool sceneBrowserOpen;
			bool entityEditorOpen;
			bool animationEditorOpen;
		protected:
			source_man* m_sourceMgr;
			space_man* m_spaceMgr;
			r2::scene* m_scene;
			
			ImGuizmo::OPERATION m_transformationOperation;
			ImGuizmo::MODE m_transformationSpace;

			source_snipper* m_snipper;
			source_skeletizer* m_skeletizer;
			speech_planner* m_planner;
			space_browser* m_spaceBrowser;
			scene_browser* m_sceneBrowser;
			entity_editor* m_entityEditor;
			animation_editor* m_animationEditor;
	};
};

