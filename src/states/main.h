#include <r2/managers/stateman.h>

namespace r2 {
	class texture_buffer;
};

namespace rosen {
	class video_container;
	class audio_container;
	class main_state : public r2::state {
		public:
			main_state();
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
			video_container* m_video;
			audio_container* m_audio;
			r2::texture_buffer* m_currentTexture;
	};

};