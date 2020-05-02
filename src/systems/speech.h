#include <r2/systems/entity.h>

namespace r2 {
	class audio_source;
	class texture_buffer;
};

namespace rosen {
	class source_man;
	struct speech_plan;
	struct speech_execution_context;

	class speech_component : public r2::scene_entity_component {
		public:
			speech_component();
			~speech_component();

			speech_plan* plan;
			speech_execution_context* execution;
			r2::audio_source* audio;
			r2::texture_buffer* texture;
			r2::f32 lod_skip_interval;
			r2::f32 lod_skip_time;
			r2::f32 lod_falloff_start_dist;
			r2::f32 volume;
			r2::f32 pitch;
	};

	class speech_system : public r2::entity_system, r2::periodic_update {
		public:
			~speech_system();

			static speech_system* create();

			static speech_system* get();

			virtual const size_t component_size() const { return sizeof(speech_component); }

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

			source_man* sources;
			r2::f32 dist_lod_skip_mult;

		protected:
			static speech_system* instance;

			speech_system();
	};
};