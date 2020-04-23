#pragma once
#include <r2/config.h>
#include <LinearMath/btIDebugDraw.h>

namespace r2 {
	class debug_drawer;
};

namespace rosen {
	class physics_drawer : public btIDebugDraw {
		public:
			physics_drawer(r2::debug_drawer* _drawer, bool drawWireframe = true);
			~physics_drawer();

			virtual void reportErrorWarning(const char* warningString);

			virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);

			virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);

			virtual void draw3dText(const btVector3& location, const char* textString);

			virtual void setDebugMode(int mode);

			virtual int getDebugMode() const;

			r2::i32 debugMode;
			r2::debug_drawer* drawer;
	};
};
