#include <r2/managers/stateman.h>

namespace r2 {
	class texture_buffer;
	class audio_source;
};

namespace rosen {
	class source_content;
	class source_man;
	struct speech_plan;
	struct speech_execution_context;

	class main_state : public r2::state {
		public:
			main_state(source_man* sourceMgr);
			~main_state();

			virtual void onInitialize();

			virtual void willBecomeActive();

			virtual void becameActive();

			virtual void willBecomeInactive();

			virtual void becameInactive();

			virtual void willBeDestroyed();

			virtual void onUpdate(r2::f32 frameDt, r2::f32 updateDt);

			virtual void onRender();

			virtual void onEvent(r2::event* evt);

		protected:
			source_man* m_sources;
			source_content* m_source;

			speech_plan* m_plan;
			speech_execution_context* m_speech;

			r2::audio_source* m_audio;
			r2::texture_buffer* m_currentTexture;
	};

};