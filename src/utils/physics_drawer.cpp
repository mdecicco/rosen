#include <utils/physics_drawer.h>

#include <r2/engine.h>
#include <r2/utilities/debug_drawer.h>
using namespace r2;

namespace rosen {
	physics_drawer::physics_drawer(r2::debug_drawer* _drawer, bool drawWireframe) : drawer(_drawer) {
		if (drawWireframe) debugMode = btIDebugDraw::DBG_DrawWireframe;
		else debugMode = btIDebugDraw::DBG_DrawAabb;
	}

	physics_drawer::~physics_drawer() { }

	void physics_drawer::reportErrorWarning(const char* warningString) {
		r2Warn(warningString);
	}

	void physics_drawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
		drawer->line(vec3f(from.x(), from.y(), from.z()), vec3f(to.x(), to.y(), to.z()), vec4f(color.x(), color.y(), color.z(), 1.0f));
	}

	void physics_drawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
	}

	void physics_drawer::draw3dText(const btVector3& location, const char* textString) {
	}

	void physics_drawer::setDebugMode(int mode) {
		debugMode = mode;
	}

	int physics_drawer::getDebugMode() const {
		return debugMode;
	}
};