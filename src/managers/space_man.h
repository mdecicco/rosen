#pragma once
#include <r2/managers/memman.h>
#include <r2/utilities/interpolator.hpp>
#include <r2/bindings/v8helpers.h>

namespace r2 {
	class scene;
	class render_node;
	class vertex_format;
	class instance_format;
	class uniform_format;
	class shader_program;
	class node_material;
};

namespace rosen {
	class space_man;
	class rosen_camera_entity;
	class space_element_entity;
	class space_collision_element_entity;

	class rosen_space {
		public:
			struct camera_def {
				r2::vec3f position;
				r2::vec3f target;
				r2::f32 fieldOfView;
				r2::f32 width;
				r2::f32 height;
				bool isOrthographic;

				r2::mat4f projection() const;
			};

			struct light_def {
				r2::i32 type;
				r2::vec3f position;
				r2::vec3f direction;
				r2::vec3f color;
				r2::f32 cosConeInnerAngle;
				r2::f32 cosConeOuterAngle;
				r2::f32 constantAtt;
				r2::f32 linearAtt;
				r2::f32 quadraticAtt;
			};

			rosen_space(const r2::mstring& name, space_man* mgr);
			~rosen_space();

			void update(r2::f32 dt);
			void set_current_camera(r2::u8 idx, bool noTransition = false);

			inline size_t light_count() const { return m_lights.size(); }
			inline light_def* light(r2::u8 idx) { return m_lights[idx]; }

			void update_uniforms();

		protected:
			PersistentObjectHandle m_scriptObj;
			PersistentFunctionHandle m_init;
			PersistentFunctionHandle m_deinit;
			PersistentFunctionHandle m_update;

			r2::dynamic_pod_array<camera_def> m_cameraAngles;
			r2::dynamic_pod_array<light_def> m_lights;
			r2::dynamic_pod_array<space_element_entity*> m_elements;
			r2::dynamic_pod_array<space_collision_element_entity*> m_colliders;
			r2::u8 m_currentCamera;
			rosen_camera_entity* m_camera;

			space_man* m_mgr;
	};

	class space_man {
		public:
			space_man(r2::scene* s);
			~space_man();

			inline r2::vertex_format* get_vfmt() const { return m_vformat; }
			inline r2::instance_format* get_ifmt() const { return m_iformat; }
			inline r2::node_material* get_element_material() const { return m_element_material; }
			inline r2::shader_program* get_rosen_shader() const { return m_rosen_shader; }
			inline r2::scene* get_scene() const { return m_scene; }

			inline rosen_space* get_current() const { return m_current; }

		protected:
			r2::vertex_format* m_vformat;
			r2::instance_format* m_iformat;
			r2::uniform_format* m_mformat;
			r2::scene* m_scene;
			r2::shader_program* m_element_shader;
			r2::shader_program* m_rosen_shader;
			r2::node_material* m_element_material;

			rosen_space* m_current;
			r2::dynamic_pod_array<rosen_space*> m_spaces;
	};
};
