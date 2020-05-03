#include <r2/config.h>
#include <ui/keyframe_editor.h>

namespace r2 {
	class audio_source;
	class texture_buffer;
	class scene;
};

namespace rosen {
	class source_content;
	class source_man;

	class source_skeletizer {
		public:

			source_skeletizer(source_man* smgr, r2::scene* s);
			~source_skeletizer();

			void skeletons_modified();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

		protected:
			source_content* m_source;
			source_man* m_mgr;

			r2::audio_source* m_audio;
			r2::texture_buffer* m_texture;
			r2::scene* m_scene;

			// ui params
			r2::i32 m_selectedSourceIdx;
			r2::f32 m_playPos;
			kf::KeyframeEditorInterface* m_keyframes;
	};
};