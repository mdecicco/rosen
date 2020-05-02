#include <r2/managers/stateman.h>

namespace r2 {
	class fly_camera_entity;
	class shader_program;
	class debug_drawer;
};

namespace rosen {
	class source_man;
	class ui_man;
	class space_man;
	class physics_drawer;

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
			space_man* m_spaces;
			ui_man* m_ui;
			physics_drawer* m_physicsDraw;

			r2::fly_camera_entity* m_camera;
			r2::shader_program* m_debugShader;
			r2::debug_drawer* m_debugDraw;
	};
};