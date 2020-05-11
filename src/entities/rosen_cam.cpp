#include <entities/rosen_cam.h>
using namespace r2;

namespace rosen {
	rosen_camera_entity::rosen_camera_entity(const mstring& name) :
		scene_entity(name),
		cameraPosition(vec3f(0.0f), 1.0f, interpolate::easeInOutCubic),
		cameraTarget(vec3f(0.0f), 1.0f, interpolate::easeInOutCubic),
		cameraOrthoFactor(0.0f, 1.0f, interpolate::easeInOutCubic),
		cameraFov(60.0f, 1.0f, interpolate::easeInOutCubic),
		cameraWidth(16.0f, 1.0f, interpolate::easeInOutCubic),
		cameraHeight(9.0f, 1.0f, interpolate::easeInOutCubic),
		cameraNear(0.01f, 1.0f, interpolate::easeInOutCubic),
		cameraFar(100.0f, 1.0f, interpolate::easeInOutCubic)
	{
	}

	rosen_camera_entity::~rosen_camera_entity() {
	}

	void rosen_camera_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		camera_sys::get()->addComponentTo(this);
		setUpdateFrequency(60.0f);
	}

	void rosen_camera_entity::onUpdate(f32 frameDt, f32 updateDt) {
		bool update_frustum = false;
		if (!cameraPosition.stopped() || !cameraTarget.stopped()) {
			transform->transform = glm::lookAt((vec3f)cameraPosition, (vec3f)cameraTarget, vec3f(0.0f, 1.0f, 0.0f));
			update_frustum = true;
		}

		if (
			!cameraOrthoFactor.stopped() || !cameraFov.stopped() || !cameraWidth.stopped() ||
			!cameraHeight.stopped() || !cameraNear.stopped() || !cameraFar.stopped()
		) {
			camera->orthographic_factor = cameraOrthoFactor;
			camera->field_of_view = cameraFov;
			camera->width = cameraWidth;
			camera->height = cameraHeight;
			camera->near_plane = cameraNear;
			camera->far_plane = cameraFar;
			camera->update_projection();

			// Frustum updated by update_projection
			update_frustum = false;
		}

		if (update_frustum) camera->update_frustum();
	}

	void rosen_camera_entity::onEvent(event* evt) {
	}

	void rosen_camera_entity::willBeDestroyed() {
	}

	void rosen_camera_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}

	void rosen_camera_entity::force_camera_update() {
		camera->orthographic_factor = cameraOrthoFactor;
		camera->field_of_view = cameraFov;
		camera->width = cameraWidth;
		camera->height = cameraHeight;
		camera->near_plane = cameraNear;
		camera->far_plane = cameraFar;
		camera->update_projection();
	}
};