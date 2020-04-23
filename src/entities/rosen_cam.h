#include <r2/engine.h>
#include <r2/utilities/interpolator.hpp>

namespace rosen {
	class rosen_camera_entity : public r2::scene_entity {
		public:
			rosen_camera_entity(const r2::mstring& name);

			~rosen_camera_entity();

			virtual void onInitialize();

			virtual void onUpdate(r2::f32 frameDt, r2::f32 updateDt);

			virtual void onEvent(r2::event* evt);

			virtual void willBeDestroyed();

			virtual void belowFrequencyWarning(r2::f32 percentLessThanDesired, r2::f32 desiredFreq, r2::f32 timeSpentLowerThanDesired);

			void force_camera_update();

			r2::interpolator<r2::vec3f> cameraPosition;
			r2::interpolator<r2::vec3f> cameraTarget;
			r2::interpolator<r2::mat4f> cameraProjection;
	};
};