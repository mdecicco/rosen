#pragma once
#include <r2/config.h>

namespace rosen {
	class scene_browser {
		public:
			scene_browser();
			~scene_browser();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

		protected:
			// ui params
			r2::i32 m_selectedEntityIdx;
	};
};