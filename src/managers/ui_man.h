#pragma once
#include <r2/config.h>

namespace r2 {
	class scene;
};

namespace rosen {
	class source_snipper;
	class source_skeletizer;
	class speech_planner;
	class space_browser;
	class scene_browser;
	class entity_editor;
	class source_man;
	class space_man;

	class ui_man {
		public:
			ui_man(source_man* smgr, space_man* spmgr, r2::scene* s);
			~ui_man();
			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render();

		protected:
			source_man* m_sourceMgr;
			space_man* m_spaceMgr;
			r2::scene* m_scene;

			source_snipper* m_snipper;
			bool m_snipperOpen;

			source_skeletizer* m_skeletizer;
			bool m_skeletizerOpen;

			speech_planner* m_planner;
			bool m_plannerOpen;

			space_browser* m_spaceBrowser;
			bool m_spaceBrowserOpen;

			scene_browser* m_sceneBrowser;
			bool m_sceneBrowserOpen;

			entity_editor* m_entityEditor;
			bool m_entityEditorOpen;
	};
};

