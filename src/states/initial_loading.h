#include <r2/managers/stateman.h>
#include <r2/utilities/interpolator.hpp>

namespace r2 {
	class shader_program;
};

namespace rosen {
	class source_man;

	class initial_loading_state : public r2::state {
		public:
			initial_loading_state(source_man* sourceMgr);
			~initial_loading_state();

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
			r2::timer m_startDelayTimer;
			r2::interpolator<r2::f32> m_progress;
	};
};