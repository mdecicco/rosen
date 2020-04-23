#include <r2/engine.h>

namespace rosen {
	class space_element_entity : public r2::scene_entity {
		public:
			space_element_entity(const r2::mstring& name, r2::render_node* node, const r2::mat4f& transform);

			~space_element_entity();

			virtual void onInitialize();

			virtual void onUpdate(r2::f32 frameDt, r2::f32 updateDt);

			virtual void onEvent(r2::event* evt);

			virtual void willBeDestroyed();

			virtual void belowFrequencyWarning(r2::f32 percentLessThanDesired, r2::f32 desiredFreq, r2::f32 timeSpentLowerThanDesired);

			inline r2::render_node* node() const { return m_node; }

		protected:
			r2::render_node* m_node;
			r2::mat4f m_initialTransform;
	};
};