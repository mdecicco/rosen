#pragma once
#include <r2/managers/memman.h>
#include <r2/utilities/interpolator.hpp>
#include <r2/bindings/v8helpers.h>

class btCollisionShape;

namespace r2 {
	class scene;
	class render_node;
	class texture_buffer;
	class vertex_format;
	class instance_format;
	class uniform_format;
	class shader_program;
	class node_material;
	class debug_drawer;
	enum light_type;
};

namespace rosen {
	class space_man;
	class rosen_camera_entity;
	class space_element_entity;
	class space_collision_element_entity;
	class space_light_element_entity;
	class rosen_entity;

	class rosen_space {
		public:
			#pragma pack(push, 1)
			struct mesh_data {
				struct vertex {
					r2::vec3f position;
					r2::vec3f normal;
					r2::vec2f texcoord;
				};

				struct {
					r2::vec4f diffuse;
					r2::u8 texture_len;
					char* texture;
				} material;

				r2::u32 vertex_count;
				vertex* vertices;
				r2::u32 index_count;
				r2::u16* indices;
				r2::u16 instance_count;
			};

			struct collision_mesh_data {
				r2::u32 vertex_count;
				r2::vec3f* vertices;
				r2::u32 index_count;
				r2::u16* indices;
			};

			struct camera_node {
				r2::vec3f position;
				r2::vec3f target;
				r2::f32 fieldOfView;
				r2::f32 width;
				r2::f32 height;
				bool isOrthographic;

				r2::mat4f projection() const;
			};

			struct mesh_node {
				r2::u16 mesh_idx;
			};

			struct light_node {
				r2::light_type type;
				r2::vec3f color;
				r2::f32 coneInnerAngle;
				r2::f32 coneOuterAngle;
				r2::f32 constantAttenuation;
				r2::f32 linearAttenuation;
				r2::f32 quadraticAttenuation;
			};

			struct collision_node {
				r2::u16 collision_mesh_idx;
			};

			enum node_type {
				nt_mesh,
				nt_light,
				nt_camera,
				nt_collision,
				nt_pointOfInterest
			};

			struct scene_node {
				node_type type;
				r2::u8 name_len;
				char* name;
				r2::mat4f transform;
				union {
					camera_node cam;
					mesh_node mesh;
					light_node light;
					collision_node collision;
				};
			};

			struct scene_graph {
				~scene_graph();

				r2::u16 mesh_count;
				mesh_data* meshes;

				r2::u16 collider_count;
				collision_mesh_data* collision_meshes;

				r2::u16 node_count;
				scene_node* nodes;
			};
			#pragma pack(pop)

			rosen_space(const r2::mstring& name, space_man* mgr);
			~rosen_space();

			bool load();
			void unload();
			void initialize();

			void update(r2::f32 dt);
			void set_current_camera(r2::u8 idx, bool noTransition = false);

			inline const r2::mstring& name() const { return m_name; }
			inline size_t light_count() const { return m_lights.size(); }
			inline space_light_element_entity* light(r2::u8 idx) { return *m_lights[idx]; }
			inline size_t element_count() const { return m_elements.size(); }
			inline space_element_entity* element(r2::u8 idx) { return *m_elements[idx]; }
			inline size_t camera_count() const { return m_cameraAngles.size(); }
			inline camera_node* camera(r2::u8 idx) { return m_cameraAngles[idx]; }
			inline const r2::mstring& camera_name(r2::u8 idx) const { return m_cameraNames[idx]; }
			inline size_t collider_count() const { return m_colliders.size(); }
			inline space_collision_element_entity* collider(r2::u8 idx) { return *m_colliders[idx]; }
			inline size_t point_of_interest_count() const { return m_pointsOfInterest.size(); }
			inline r2::mvector<r2::mstring> point_of_interest_names() const { return m_pointsOfInterest.keys(); }
			inline r2::mat4f* point_of_interest(r2::mstring name) { return m_pointsOfInterest[name]; }
			inline rosen_camera_entity* camera() const { return m_camera; }

			void debug_draw(r2::debug_drawer* draw);

			rosen_entity* spawn_rosen(const r2::mstring& name, const r2::mat4f& transform);

		protected:
			scene_graph* load_fbx(const r2::mstring& file);
			scene_graph* load_space(const r2::mstring& file);
			bool save_space(const r2::mstring& file, scene_graph* space);

			void init_script_obj(v8::Local<v8::Object>& obj);

			PersistentObjectHandle m_scriptObj;
			PersistentFunctionHandle m_init;
			PersistentFunctionHandle m_deinit;
			PersistentFunctionHandle m_update;

			r2::mvector<r2::mstring> m_cameraNames;
			r2::dynamic_pod_array<camera_node> m_cameraAngles;
			r2::dynamic_pod_array<space_light_element_entity*> m_lights;
			r2::dynamic_pod_array<space_element_entity*> m_elements;
			r2::dynamic_pod_array<btCollisionShape*> m_shapes;
			r2::dynamic_pod_array<r2::render_node*> m_rNodes;
			r2::dynamic_pod_array<space_collision_element_entity*> m_colliders;
			r2::dynamic_pod_array<rosen_entity*> m_rosens;
			r2::associative_pod_array<r2::mstring, r2::mat4f> m_pointsOfInterest;
			r2::associative_pod_array<r2::mstring, r2::texture_buffer*> m_textures;
			r2::u8 m_currentCamera;
			r2::mstring m_name;
			rosen_camera_entity* m_camera;
			bool m_initialized;

			space_man* m_mgr;
	};

	class space_man {
		public:
			space_man(r2::scene* s);
			~space_man();

			inline r2::vertex_format* get_vfmt() const { return m_vformat; }
			inline r2::instance_format* get_ifmt() const { return m_iformat; }
			inline r2::node_material* get_element_material() const { return m_element_material; }
			inline r2::vertex_format* get_rosen_vfmt() const { return m_rosen_vfmt; }
			inline r2::instance_format* get_rosen_ifmt() const { return m_rosen_ifmt; }
			inline r2::node_material* get_rosen_material() const { return m_rosen_material; }
			inline r2::scene* get_scene() const { return m_scene; }

			inline rosen_space* get_current() const { return m_current; }
			inline size_t space_count() const { return m_spaces.size(); }
			inline rosen_space* space(size_t idx) { return *m_spaces[idx]; }
			void load_space(rosen_space* space);

		protected:
			r2::vertex_format* m_vformat;
			r2::instance_format* m_iformat;
			r2::uniform_format* m_mformat;
			r2::shader_program* m_element_shader;
			r2::node_material* m_element_material;

			r2::vertex_format* m_rosen_vfmt;
			r2::instance_format* m_rosen_ifmt;
			r2::uniform_format* m_rosen_mformat;
			r2::shader_program* m_rosen_shader;
			r2::node_material* m_rosen_material;

			r2::scene* m_scene;
			rosen_space* m_current;
			r2::dynamic_pod_array<rosen_space*> m_spaces;
	};
};
