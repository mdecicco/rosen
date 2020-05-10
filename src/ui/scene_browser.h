#pragma once
#include <r2/config.h>

namespace rosen {
	class ui_man;
	class scene_browser {
		public:
			scene_browser(ui_man* ui);
			~scene_browser();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

		protected:
			ui_man* m_ui;
	};
};