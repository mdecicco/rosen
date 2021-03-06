#pragma once
#include <r2/config.h>
#include <r2/utilities/imgui/imgui.h>
#include <vector>

namespace r2 {
	class scene_entity;
};

namespace kf {
	class KeyframeEditorInterface;
};

namespace rosen {
	class ui_man;
	class space_man;

	class entity_editor {
		public:
			entity_editor(space_man* smgr, ui_man* mgr);
			~entity_editor();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

			void render_transform_ui(const ImVec2& size);
			void render_camera_ui(const ImVec2& size);
			void render_mesh_ui(const ImVec2& size);
			void render_physics_ui(const ImVec2& size);
			void render_lighting_ui(const ImVec2& size);
			void render_speech_ui(const ImVec2& size);

		protected:
			ui_man* m_mgr;
			space_man* m_smgr;

			char m_speechTextBuf[1024];
			r2::i32 m_speakRandomWordCount;
			r2::scene_entity* m_last_entity;
	};
};