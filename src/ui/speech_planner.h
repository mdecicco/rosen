#include <r2/config.h>

namespace r2 {
	class audio_source;
	class texture_buffer;
	class scene;
};

namespace rosen {
	class source_man;
	struct speech_plan;
	struct speech_execution_context;

	class speech_planner {
		public:
			speech_planner(source_man* smgr, r2::scene* s);
			~speech_planner();

			void premixes_modified();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

		protected:
			source_man* m_mgr;
			speech_plan* m_visualizePlan;
			speech_plan* m_plan;
			speech_execution_context* m_execution;

			r2::audio_source* m_audio;
			r2::texture_buffer* m_texture;
			r2::scene* m_scene;

			// ui params
			char m_wordSearch[1024];
			char m_premixText[64];
	};
};