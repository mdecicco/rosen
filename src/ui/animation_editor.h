#pragma once
#include <r2/config.h>
#include <r2/utilities/imgui/imgui.h>
#include <vector>

namespace r2 {
	class scene_entity;
	class keyframe_base;
};

namespace kf {
	class KeyframeEditorInterface;
};

namespace rosen {
	class ui_man;
	class space_man;

	class animation_editor {
		public:
			animation_editor(space_man* smgr, ui_man* mgr);
			~animation_editor();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

			void save_anims();
			void init_entity_data();
			void destroy_entity_data();

	protected:
		ui_man* m_mgr;
		space_man* m_smgr;
		r2::scene_entity* m_last_entity;

		char m_animNameBuf[64];
		r2::u8 m_selectedAnimation;
		r2::keyframe_base* m_keyframeOptions;
		r2::keyframe_base* m_lastKeyframeOptions;
		std::vector<kf::KeyframeEditorInterface*> m_entityAnims;
	};
};