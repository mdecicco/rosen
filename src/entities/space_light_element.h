#include <r2/engine.h>

namespace rosen {
	class space_light_element_entity : public r2::scene_entity {
		public:
			struct light_props {
				r2::light_type type;
				r2::mat4f transform;
				r2::vec3f color;
				r2::f32 coneInnerAngle;
				r2::f32 coneOuterAngle;
				r2::f32 constantAtt;
				r2::f32 linearAtt;
				r2::f32 quadraticAtt;
			};

			space_light_element_entity(const r2::mstring& name, const light_props& props);

			~space_light_element_entity();

			virtual void onInitialize();

			virtual void onUpdate(r2::f32 frameDt, r2::f32 updateDt);

			virtual void onEvent(r2::event* evt);

			virtual void willBeDestroyed();

			virtual void belowFrequencyWarning(r2::f32 percentLessThanDesired, r2::f32 desiredFreq, r2::f32 timeSpentLowerThanDesired);

		protected:
			light_props m_initial_props;
	};
};