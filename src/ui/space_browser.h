#pragma once
#include <r2/config.h>
#include <managers/space_man.h>

namespace rosen {
	class space_browser {
		public:
			space_browser(space_man* smgr);
			~space_browser();

			void update(r2::f32 frameDt, r2::f32 updateDt);
			void render(bool* isOpen);

			void element_ui(space_element_entity* element);
			void collider_ui(space_collision_element_entity* collider);
			void light_ui(space_light_element_entity* light);
			void camera_ui(rosen_space::camera_node* cam, r2::u32 idx);
			void poi_clicked(const r2::mstring& name);

		protected:
			space_man* m_mgr;

			// ui params
			r2::i32 m_selectedSpaceIdx;
	};
};