#include <r2/engine.h>

namespace rosen {
	class space_collision_element_entity : public r2::scene_entity {
		public:
			space_collision_element_entity(const r2::mstring& name, const r2::mat4f& transform, btTriangleMesh* mesh);

			~space_collision_element_entity();

			virtual void onInitialize();

			virtual void onUpdate(r2::f32 frameDt, r2::f32 updateDt);

			virtual void onEvent(r2::event* evt);

			virtual void willBeDestroyed();

			virtual void belowFrequencyWarning(r2::f32 percentLessThanDesired, r2::f32 desiredFreq, r2::f32 timeSpentLowerThanDesired);

		protected:
			r2::mat4f m_initialTransform;
			btTriangleMesh* m_mesh;
	};
};