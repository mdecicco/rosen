#include <r2/engine.h>
#include <r2/utilities/interpolator.hpp>

namespace r2 {
	class texture_buffer;
	class render_node;
};

namespace rosen {
	class speech_component;

	class rosen_entity : public r2::scene_entity {
		public:
			rosen_entity(const r2::mstring& name, r2::render_node* node);

			~rosen_entity();

			virtual void onInitialize();

			virtual void onUpdate(r2::f32 frameDt, r2::f32 updateDt);

			virtual void onEvent(r2::event* evt);

			virtual void willBeDestroyed();

			virtual void belowFrequencyWarning(r2::f32 percentLessThanDesired, r2::f32 desiredFreq, r2::f32 timeSpentLowerThanDesired);

			speech_component* speech();

			void speak(const r2::mstring& text);

			void speak_nonsense(u32 word_count);


			r2::texture_buffer* texture;
			r2::render_node* node;
			r2::interpolator<r2::vec3f> pos;
			r2::vec3f shirt_color_hsv;
	};
};