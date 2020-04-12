#include <r2/config.h>

namespace r2 {
	class audio_source;
	class texture_buffer;
	class scene;
};

namespace rosen {
	class source_content;
	class source_man;
	struct speech_plan;
	struct speech_execution_context;

	class source_snipper {
		public:
			source_snipper(source_man* smgr, r2::scene* s);
			~source_snipper();

			void snips_modified();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render();

		protected:
			source_content* m_source;
			source_man* m_mgr;
			speech_plan* m_plan;
			speech_execution_context* m_execution;


			r2::audio_source* m_audio;
			r2::texture_buffer* m_texture;
			r2::scene* m_scene;

			// ui params
			r2::i32 m_selectedSourceIdx;
			char m_visualize[1024];
			char m_visualizeAll[1024];
			char m_snipName[256];
			r2::f32 m_times[2];
			r2::f32 m_offsets[2];
	};
};