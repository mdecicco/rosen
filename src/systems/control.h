#include <r2/systems/entity.h>

namespace r2 {
};

namespace rosen {
	class control_component : public r2::scene_entity_component {
		public:
			control_component();
			~control_component();

			bool control_enabled;
			bool on_ground;
			r2::f32 movement_speed;
			r2::f32 jump_impulse;
	};

	class control_system : public r2::entity_system, r2::periodic_update {
		public:
			~control_system();

			static control_system* create();

			static control_system* get();

			virtual const size_t component_size() const { return sizeof(control_component); }

			virtual void initialize();

			virtual void deinitialize();

			virtual void initialize_entity(r2::scene_entity* entity);

			virtual void deinitialize_entity(r2::scene_entity* entity);

			virtual r2::scene_entity_component* create_component(r2::entityId id);

			virtual void bind(r2::scene_entity_component* component, r2::scene_entity* entity);

			virtual void unbind(r2::scene_entity* entity);

			virtual void tick(r2::f32 dt);

			virtual void handle(r2::event* evt);

			virtual void doUpdate(r2::f32 frameDelta, r2::f32 updateDelta);

			r2::f32 analog_deadzone;
			r2::timer last_jump_press;

		protected:
			static control_system* instance;

			control_system();
	};
};