#include <entities/rosen_cam.h>
using namespace r2;

namespace rosen {
	rosen_camera_entity::rosen_camera_entity(const mstring& name) :
		scene_entity(name),
		cameraPosition(vec3f(0.0f), 1.0f, interpolate::easeInOutCubic),
		cameraTarget(vec3f(0.0f), 1.0f, interpolate::easeInOutCubic),
		cameraProjection(mat4f(1.0f), 1.0f, interpolate::easeInOutCubic)
	{
	}

	rosen_camera_entity::~rosen_camera_entity() {
	}

	void rosen_camera_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		camera_sys::get()->addComponentTo(this);
	}

	void rosen_camera_entity::onUpdate(f32 frameDt, f32 updateDt) {
		if (!cameraPosition.stopped() || !cameraTarget.stopped() || !cameraProjection.stopped()) {
			transform->transform = glm::lookAt((vec3f)cameraPosition, (vec3f)cameraTarget, vec3f(0.0f, 1.0f, 0.0f));
			camera->projection = cameraProjection;
			camera->update_frustum();
		}
	}

	void rosen_camera_entity::onEvent(event* evt) {
	}

	void rosen_camera_entity::willBeDestroyed() {
	}

	void rosen_camera_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}

	void rosen_camera_entity::force_camera_update() {
		transform->transform = glm::lookAt((vec3f)cameraPosition, (vec3f)cameraTarget, vec3f(0.0f, 1.0f, 0.0f));
		camera->projection = cameraProjection;
		camera->update_frustum();
	}
};