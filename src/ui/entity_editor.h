#pragma once
#include <r2/config.h>

namespace rosen {
	class entity_editor {
		public:
			entity_editor();
			~entity_editor();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

		protected:
	};
};