#include <managers/space_man.h>
#include <utils/lode_png.h>
#include <entities/rosen.h>
#include <entities/rosen_cam.h>
#include <entities/space_element.h>
#include <entities/space_collision_element.h>
#include <entities/space_light_element.h>

#include <r2/engine.h>
#include <r2/utilities/debug_drawer.h>
#include <glm/gtc/matrix_inverse.hpp>
using namespace r2;

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

namespace rosen {
	mat4f assimp_to_glm(const aiMatrix4x4& m) {
		glm::mat4 to;

		to[0][0] = m.a1; to[0][1] = m.b1;  to[0][2] = m.c1; to[0][3] = m.d1;
		to[1][0] = m.a2; to[1][1] = m.b2;  to[1][2] = m.c2; to[1][3] = m.d2;
		to[2][0] = m.a3; to[2][1] = m.b3;  to[2][2] = m.c3; to[2][3] = m.d3;
		to[3][0] = m.a4; to[3][1] = m.b4;  to[3][2] = m.c4; to[3][3] = m.d4;

		return to;
	}

	void get_meshes(const aiScene* s, aiNode* node, mvector<aiNode*>& meshNodes) {
		for (u32 i = 0;i < node->mNumChildren;i++) {
			get_meshes(s, node->mChildren[i], meshNodes);
		}

		if (node->mNumMeshes > 0) meshNodes.push_back(node);
	}

	mat4f flipYZ(const mat4f& m) {
		return mat4f(
			m[0],
			m[2],
			-m[1],
			m[3]
		);
	}

	mat4f get_baked_transform(aiNode* node) {
		aiMatrix4x4 transform;
		aiNode* parent = node->mParent;
		while (parent) {
			if (mstring(parent->mName.data, parent->mName.length).find("$AssimpFbx$_Geometric") != mstring::npos) {
				transform = parent->mTransformation * transform;
			}
			parent = parent->mParent;
		}

		return assimp_to_glm(transform);
	}

	mat4f get_transform(aiNode* node, bool _flipYZ = false) {
		aiMatrix4x4 transform = node->mTransformation;
		aiNode* parent = node->mParent;
		while (parent) {
			if (mstring(parent->mName.data, parent->mName.length).find("$AssimpFbx$_Geometric") == mstring::npos) {
				transform = parent->mTransformation * transform;
			}
			parent = parent->mParent;
		}
		
		if (_flipYZ) {
			return flipYZ(assimp_to_glm(transform));
		}

		return assimp_to_glm(transform);
	}



	rosen_space::rosen_space(const mstring& space_name, space_man* mgr) {
		m_mgr = mgr;
		m_name = space_name;
		m_camera = nullptr;
		m_initialized = false;
	}

	rosen_space::~rosen_space() {
		unload();
	}

	bool rosen_space::load() {
		m_initialized = false;
		m_camera = new rosen_camera_entity(m_name + "_camera");

		scene_graph* sg = nullptr;
		if (r2engine::files()->exists("./resources/space/" + m_name + "/space.spce")) {
			sg = load_space("./resources/space/" + m_name + "/space.spce");
		} else {
			sg = load_fbx("./resources/space/" + m_name + "/space.fbx");
			if (sg) save_space("./resources/space/" + m_name + "/space.spce", sg);
		}
		if (!sg) return false;

		struct id {
			mat4f transform;
			mat3f normal_transform;
			i32 entity_id;
		} initial = { mat4f(1.0f), mat3f(1.0f), 0 };
		for (u32 i = 0;i < sg->mesh_count;i++) {
			mesh_construction_data mcd(m_mgr->get_vfmt(), it_unsigned_short, m_mgr->get_ifmt());
			mcd.set_max_vertex_count(sg->meshes[i].vertex_count);
			mcd.set_max_index_count(sg->meshes[i].index_count);
			mcd.set_max_instance_count(sg->meshes[i].instance_count);
			mcd.append_index_data(sg->meshes[i].indices, sg->meshes[i].index_count);
			mcd.append_vertex_data(sg->meshes[i].vertices, sg->meshes[i].vertex_count);
			for (size_t in = 0;in < sg->meshes[i].instance_count;in++) mcd.append_instance<id>(initial);

			render_node* node = m_mgr->get_scene()->add_mesh(&mcd);
			node_material_instance* material = m_mgr->get_element_material()->instantiate(m_mgr->get_scene());
			material->uniforms()->uniform_vec4f("diffuse", sg->meshes[i].material.diffuse);

			if (sg->meshes[i].material.texture_len > 0) {
				mstring tex(sg->meshes[i].material.texture, sg->meshes[i].material.texture_len);

				if (m_textures.has(tex)) material->set_texture("diffuse_tex", *m_textures.get(tex));
				else {
					mvector<u8> pixels;
					u32 w, h;
					u32 err = lodepng::decode(pixels, w, h, "./resources/space/" + m_name + "/" + tex);
					if (err) {
						r2Warn("Failed to load texture %s: %s", tex.c_str(), lodepng_error_text(err));
					}
					else {
						texture_buffer* tbuf = m_mgr->get_scene()->create_texture();
						tbuf->create(pixels.data(), w, h, 4, tt_unsigned_byte);
						tbuf->set_min_filter(tmnf_linear);
						tbuf->set_mag_filter(tmgf_linear);
						tbuf->set_x_wrap_mode(tw_repeat);
						tbuf->set_y_wrap_mode(tw_repeat);
						m_textures.set(tex, tbuf);
						material->set_texture("diffuse_tex", tbuf);
					}
				}
			}

			node->set_material_instance(material);
			m_rNodes.push(node);
		}

		for (u32 i = 0;i < sg->collider_count;i++) {
			btTriangleMesh* tmesh = new btTriangleMesh(false, false);
			for (u32 idx = 0;idx < sg->collision_meshes[i].index_count;idx += 3) {
				vec3f v0 = sg->collision_meshes[i].vertices[sg->collision_meshes[i].indices[idx + 0]];
				vec3f v1 = sg->collision_meshes[i].vertices[sg->collision_meshes[i].indices[idx + 1]];
				vec3f v2 = sg->collision_meshes[i].vertices[sg->collision_meshes[i].indices[idx + 2]];
				tmesh->addTriangle(
					btVector3(v0.x, v0.y, v0.z),
					btVector3(v1.x, v1.y, v1.z),
					btVector3(v2.x, v2.y, v2.z)
				);
			}
			btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(tmesh, true);
			m_shapes.push(shape);
		}

		for (u32 i = 0;i < sg->node_count;i++) {
			mstring name(sg->nodes[i].name, sg->nodes[i].name_len);
			switch (sg->nodes[i].type) {
				case nt_mesh: {
					m_elements.push(new space_element_entity(name, *m_rNodes[sg->nodes[i].mesh.mesh_idx], sg->nodes[i].transform));
					break;
				}
				case nt_light: {
					m_lights.push(new space_light_element_entity(name, {
						sg->nodes[i].light.type,
						sg->nodes[i].transform,
						sg->nodes[i].light.color,
						sg->nodes[i].light.coneInnerAngle,
						sg->nodes[i].light.coneOuterAngle,
						sg->nodes[i].light.constantAttenuation,
						sg->nodes[i].light.linearAttenuation,
						sg->nodes[i].light.quadraticAttenuation
					}));
					break;
				}
				case nt_camera: {
					m_cameraNames.push_back(mstring(sg->nodes[i].name, sg->nodes[i].name_len));
					m_cameraAngles.push(sg->nodes[i].cam);
					break;
				}
				case nt_collision: {
					u32 shape_idx = sg->nodes[i].collision.collision_mesh_idx;
					btCollisionShape** shape = m_shapes[shape_idx];
					m_colliders.push(new space_collision_element_entity(name, sg->nodes[i].transform, *shape));
					break;
				}
				case nt_pointOfInterest: {
					m_pointsOfInterest.set(name, sg->nodes[i].transform);
					break;
				}
				default: {
					r2Warn("Invalid node type encountered in %s", ("./resources/space/" + m_name + "/space.spce").c_str());
				}
			}
		}

		if (r2engine::files()->exists("./resources/space/" + m_name + "/space.js")) {
			data_container* script = r2engine::files()->open("./resources/space/" + m_name + "/space.js", DM_TEXT, m_name + "_script");
			if (script) {
				mstring code;
				script->read_string(code, script->size());
				r2engine::files()->destroy(script);

				if (code.find("class") != mstring::npos) {
					code = "() => { return " + code + "; };";
					Local<Value> get_class_func;
					if (r2engine::scripts()->execute(code, &get_class_func)) {
						if (get_class_func->IsFunction()) {
							auto isolate = r2engine::isolate();
							MaybeLocal<Value> class_value = Local<Function>::Cast(get_class_func)->Call(isolate->GetCurrentContext(), Null(isolate), 0, nullptr);
							Local<Value> constructor;
							class_value.ToLocal(&constructor);
							if (constructor->IsFunction()) {
								MaybeLocal<Value> minstance = Local<Function>::Cast(constructor)->CallAsConstructor(isolate->GetCurrentContext(), 0, nullptr);
								Local<Value> instance;
								minstance.ToLocal(&instance);

								if (!instance.IsEmpty() && instance->IsObject()) {
									Local<Object> obj = Local<Object>::Cast(instance);
									init_script_obj(obj);
								}
							}
						}
					}
				} else {
					r2Warn("Script for space '%s' doesn't contain a class. Ignoring", m_name.c_str());
				}
			}
		}

		return true;
	}

	void rosen_space::unload() {
		m_initialized = false;
		if (m_camera) m_camera->destroy();
		m_camera = nullptr;

		if (!m_deinit.IsEmpty()) {
			auto isolate = r2engine::isolate();
			m_deinit.Get(isolate)->Call(isolate->GetCurrentContext(), m_scriptObj.Get(isolate), 0, nullptr);
		}

		m_cameraNames.clear();
		m_cameraAngles.clear();

		m_elements.for_each([this](space_element_entity** element) {
			(*element)->stop_periodic_updates();
			(*element)->mesh->release_node();
			(*element)->destroy();
			return true;
		});
		m_elements.clear();

		m_lights.for_each([this](space_light_element_entity** light) {
			(*light)->stop_periodic_updates();
			(*light)->destroy();
			return true;
		});
		m_lights.clear();

		m_colliders.for_each([this](space_collision_element_entity** collider) {
			(*collider)->stop_periodic_updates();
			(*collider)->physics->destroy();
			(*collider)->destroy();
			return true;
		});
		m_colliders.clear();

		m_rosens.for_each([this](rosen_entity** rosen) {
			(*rosen)->stop_periodic_updates();
			(*rosen)->mesh->release_node();
			(*rosen)->destroy();
			return true;
		});
		m_rosens.clear();

		m_rNodes.for_each([this](render_node** node) {
			delete (*node)->material_instance();
			m_mgr->get_scene()->remove_node(*node);
			return true;
		});
		m_rNodes.clear();

		m_shapes.for_each([this](btCollisionShape** shape) {
			delete *shape;
			return true;
		});
		m_shapes.clear();

		m_textures.for_each([this](texture_buffer** tex) {
			m_mgr->get_scene()->destroy(*tex);
			return true;
		});
		m_textures.clear();

		m_pointsOfInterest.clear();
		m_currentCamera = 0;

		m_scriptObj.Reset();
		m_init.Reset();
		m_deinit.Reset();
		m_update.Reset();
	}

	void rosen_space::initialize() {
		if (m_initialized) return;
		if (!m_scriptObj.IsEmpty() && !m_init.IsEmpty()) {
			m_init.Get(r2engine::isolate())->Call(r2engine::isolate()->GetCurrentContext(), m_scriptObj.Get(r2engine::isolate()), 0, nullptr);
		}
		m_initialized = true;
	}

	void rosen_space::update(f32 dt) {
		if (!m_update.IsEmpty() && m_initialized) {
			auto isolate = r2engine::isolate();
			Local<Value> args[] = {
				v8pp::convert<f32>::to_v8(isolate, dt)
			};
			m_update.Get(isolate)->Call(isolate->GetCurrentContext(), m_scriptObj.Get(isolate), 1, args);
		}
	}

	void rosen_space::set_current_camera(u8 idx, bool noTransition) {
		if (m_currentCamera == idx) {
			if (!m_camera->camera->is_active()) camera_sys::get()->activate_camera(m_camera);
			return;
		}
		m_currentCamera = idx;

		camera_node* cam = m_cameraAngles[idx];

		if (noTransition) {
			m_camera->cameraPosition.set_immediate(cam->position);
			m_camera->cameraTarget.set_immediate(cam->target);
			m_camera->cameraProjection.set_immediate(cam->projection());
			m_camera->force_camera_update();
		} else {
			m_camera->cameraPosition = cam->position;
			m_camera->cameraTarget = cam->target;
			m_camera->cameraProjection = cam->projection();
		}

		camera_sys::get()->activate_camera(m_camera);
	}

	void rosen_space::debug_draw(debug_drawer* draw) {
		struct vertex {
			vec3f p;
			vec3f n;
			vec2f t;
		};
		struct instance_data {
			mat4f transform;
			mat3f normal_transform;
		};

		for (u32 i = 0;i < m_elements.size();i++) {
			space_element_entity* e = *m_elements[i];
			if (e->mesh) {
				instance_data* id = (instance_data*)e->mesh->get_instance_data<instance_data>();
				vec3f c = id->transform * vec4f(0, 0, 0, 1);
				vec3f x = id->transform * vec4f(5, 0, 0, 1);
				vec3f y = id->transform * vec4f(0, 5, 0, 1);
				vec3f z = id->transform * vec4f(0, 0, 5, 1);
				draw->line(c, x, vec4f(1, 0, 0, 1));
				draw->line(c, y, vec4f(0, 1, 0, 1));
				draw->line(c, z, vec4f(0, 0, 1, 1));

				render_node* n = e->mesh->get_node();
				vertex* vertices = (vertex*)n->vertex_data();

				for (u32 v = 0;v < n->vertex_count();v++) {
					vertex& vtx = vertices[v];
					vec3f p = id->transform * vec4f(vtx.p, 1.0f);
					vec3f n = id->normal_transform * vtx.n;
					draw->line(p, p + (n * 0.1f), vec4f(n, 1.0f));
				}
			}
		}
	}

	rosen_entity* rosen_space::spawn_rosen(const mstring& name, const mat4f& transform) {
		mesh_construction_data* mesh = new mesh_construction_data(m_mgr->get_rosen_vfmt(), it_unsigned_byte, m_mgr->get_rosen_ifmt());
		mesh->set_max_vertex_count(4);
		mesh->set_max_index_count(6);
		mesh->set_max_instance_count(1);

		struct vertex { vec3f pos; vec2f tex; };
		f32 width = 1.80555556f;
		f32 height = 1.0f;
		mesh->append_vertex<vertex>({ vec3f(-width * 0.5f,  0.5f, 0.0f), vec2f(0, 0) });
		mesh->append_vertex<vertex>({ vec3f( width * 0.5f,  0.5f, 0.0f), vec2f(1, 0) });
		mesh->append_vertex<vertex>({ vec3f( width * 0.5f, -0.5f, 0.0f), vec2f(1, 1) });
		mesh->append_vertex<vertex>({ vec3f(-width * 0.5f, -0.5f, 0.0f), vec2f(0, 1) });

		mesh->append_index<u8>(0);
		mesh->append_index<u8>(1);
		mesh->append_index<u8>(3);
		mesh->append_index<u8>(1);
		mesh->append_index<u8>(2);
		mesh->append_index<u8>(3);

		struct i { mat4f t; i32 e; };
		mesh->append_instance<i>({ transform, 0 });

		render_node* node = m_mgr->get_scene()->add_mesh(mesh);
		node->set_material_instance(m_mgr->get_rosen_material()->instantiate(m_mgr->get_scene()));

		node->material_instance()->uniforms()->uniform_vec3f("shirt_tint", vec3f(0.3f, 0.5f, 2.0f));

		rosen_entity* rosen = new rosen_entity(name, node);
		rosen->initial_transform = transform;
		m_rosens.push(rosen);
		return rosen;
	}

	rosen_space::scene_graph* rosen_space::load_fbx(const mstring& file) {
		Assimp::Importer imp;
		const aiScene* s = imp.ReadFile(file, 0);

		struct meshref {
			mvector<mstring> names;
			mvector<mat4f> transforms;
			mesh_data mesh;
		};

		struct collision_meshref {
			mvector<mstring> names;
			mvector<mat4f> transforms;
			collision_mesh_data mesh;
		};

		struct poi {
			mstring name;
			mat4f transform;
		};

		munordered_map<u32, meshref*> rmeshes;
		munordered_map<u32, collision_meshref*> cmeshes;
		mvector<poi> pois;
		

		u32 collider_count = 0;
		u32 meshnode_count = 0;
		for (u32 n = 0;n < s->mRootNode->mNumChildren;n++) {
			aiNode* a_node = s->mRootNode->mChildren[n];

			mvector<aiNode*> meshNodes;
			get_meshes(s, a_node, meshNodes);

			mstring a_name(a_node->mName.data, a_node->mName.length);
			if (a_name.find("poi_") != mstring::npos && a_node->mNumChildren == 1) {
				aiNode* c = a_node->mChildren[0];
				while (c->mNumChildren == 1) {
					c = c->mChildren[0];
				}


				mstring c_name(c->mName.data, c->mName.length);
				pois.push_back({
					c_name.substr(4),
					get_transform(c, true)
				});
			}

			for (u32 mn = 0;mn < meshNodes.size();mn++) {
				aiNode* cnode = meshNodes[mn];
				for (u32 i = 0;i < cnode->mNumMeshes;i++) {
					aiMesh* mesh = s->mMeshes[cnode->mMeshes[i]];
					mstring name(cnode->mName.data, cnode->mName.length);

					if (name.find("collision") != mstring::npos) {
						mat4f ctransform = get_transform(cnode, true);
						if (cmeshes.count(cnode->mMeshes[i]) > 0) {
							cmeshes[cnode->mMeshes[i]]->names.push_back(name);
							cmeshes[cnode->mMeshes[i]]->transforms.push_back(ctransform);
							collider_count++;
						} else {
							r2Log("Collision: %s", name.c_str());

							collision_meshref* mr = new collision_meshref();
							memset(&mr->mesh, 0, sizeof(collision_mesh_data));

							mr->mesh.vertex_count = mesh->mNumVertices;
							mr->mesh.vertices = new vec3f[mr->mesh.vertex_count];
							mr->mesh.index_count = mesh->mNumFaces * 3;
							mr->mesh.indices = new u16[mr->mesh.index_count];

							bool validFaces = true;
							for (u32 f = 0;f < mesh->mNumFaces && validFaces;f++) {
								aiFace& face = mesh->mFaces[f];
								validFaces = face.mNumIndices == 3;
								if (validFaces) {
									mr->mesh.indices[(f * 3) + 0] = face.mIndices[0];
									mr->mesh.indices[(f * 3) + 1] = face.mIndices[1];
									mr->mesh.indices[(f * 3) + 2] = face.mIndices[2];
								}
							}

							if (!validFaces) {
								r2Warn("Mesh has invalid face index count. skipping");
								delete [] mr->mesh.vertices;
								delete [] mr->mesh.indices;
								delete mr;
								continue;
							}

							mat4f itransform = get_baked_transform(cnode);

							for (u32 v = 0;v < mesh->mNumVertices;v++) {
								vec3f pos(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
								pos = itransform * vec4f(pos, 1.0f);
								mr->mesh.vertices[v] = {
									pos.x, pos.z, -pos.y,
								};
							}

							mr->names.push_back(name);
							mr->transforms.push_back(ctransform);

							cmeshes[cnode->mMeshes[i]] = mr;
							collider_count++;
						}
						continue;
					}
					else {
						if (rmeshes.count(cnode->mMeshes[i]) > 0) {
							rmeshes[cnode->mMeshes[i]]->names.push_back(name);
							get_baked_transform(cnode);
							rmeshes[cnode->mMeshes[i]]->transforms.push_back(get_transform(cnode, true));
							rmeshes[cnode->mMeshes[i]]->mesh.instance_count++;
							meshnode_count++;
						} else {
							r2Log("Mesh: %s", name.c_str());
							if (!mesh->HasNormals()) {
								r2Warn("Mesh has no normals. skipping");
								continue;
							}

							if (!mesh->HasTextureCoords(0)) {
								r2Warn("Mesh has no texcoords. skipping");
								continue;
							}

							meshref* mr = new meshref();
							memset(&mr->mesh, 0, sizeof(mesh_data));

							mr->mesh.vertex_count = mesh->mNumVertices;
							mr->mesh.vertices = new mesh_data::vertex[mr->mesh.vertex_count];
							mr->mesh.index_count = mesh->mNumFaces * 3;
							mr->mesh.indices = new u16[mr->mesh.index_count];

							bool validFaces = true;
							for (u32 f = 0;f < mesh->mNumFaces && validFaces;f++) {
								aiFace& face = mesh->mFaces[f];
								validFaces = face.mNumIndices == 3;
								if (validFaces) {
									mr->mesh.indices[(f * 3) + 0] = face.mIndices[0];
									mr->mesh.indices[(f * 3) + 1] = face.mIndices[1];
									mr->mesh.indices[(f * 3) + 2] = face.mIndices[2];
								}
							}
							if (!validFaces) {
								r2Warn("Mesh has invalid face index count. skipping");
								delete [] mr->mesh.vertices;
								delete [] mr->mesh.indices;
								delete mr;
								continue;
							}

							mat4f itransform = get_baked_transform(cnode);
							mat4f ctransform = get_transform(cnode, true);

							for (u32 v = 0;v < mesh->mNumVertices;v++) {
								vec3f pos(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
								pos = itransform * vec4f(pos, 1.0f);
								vec3f nrm(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
								nrm = glm::inverseTranspose(mat3f(itransform)) * nrm;
								mr->mesh.vertices[v] = {
									{ pos.x, pos.z, -pos.y },
									{ nrm.x, nrm.z, -nrm.y },
									{ mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y }
								};
							}

							aiMaterial* material = s->mMaterials[mesh->mMaterialIndex];
							aiColor4D color;
							if (material->Get("$clr.diffuse", 0, 0, color) == aiReturn_SUCCESS) mr->mesh.material.diffuse = vec4f(color.r, color.g, color.b, color.a);
							else mr->mesh.material.diffuse = vec4f(0.5f, 0.5f, 0.5f, 1.0f);

							aiString path;
							if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS) {
								mstring tex(path.data, path.length);
								mr->mesh.material.texture_len = path.length;
								mr->mesh.material.texture = new char[path.length];
								memcpy(mr->mesh.material.texture, path.data, path.length);
							}

							mr->names.push_back(name);
							mr->transforms.push_back(ctransform);
							mr->mesh.instance_count = 1;

							rmeshes[cnode->mMeshes[i]] = mr;
							meshnode_count++;
						}
					}
				}
			}
		}

		scene_graph* graph = new scene_graph();
		graph->node_count = collider_count + meshnode_count + s->mNumCameras + s->mNumLights + pois.size();
		graph->nodes = new scene_node[graph->node_count];
		u32 cn = 0;

		graph->mesh_count = rmeshes.size();
		if (rmeshes.size() > 0) {
			graph->meshes = new mesh_data[rmeshes.size()];
			u32 i = 0;
			for (auto it = rmeshes.begin();it != rmeshes.end();it++) {
				meshref* ref = it->second;
				memcpy(&graph->meshes[i], &ref->mesh, sizeof(mesh_data));

				for (u32 e = 0;e < ref->names.size();e++) {
					graph->nodes[cn].type = nt_mesh;
					graph->nodes[cn].name_len = ref->names[e].length();
					graph->nodes[cn].name = new char[ref->names[e].length()];
					memcpy(graph->nodes[cn].name, ref->names[e].data(), ref->names[e].length());
					graph->nodes[cn].mesh.mesh_idx = i;
					graph->nodes[cn].transform = ref->transforms[e];
					cn++;
				}

				i++;
			}
		}
		else graph->meshes = nullptr;

		graph->collider_count = cmeshes.size();
		if (cmeshes.size() > 0) {
			graph->collision_meshes = new collision_mesh_data[cmeshes.size()];
			u32 i = 0;
			for (auto it = cmeshes.begin();it != cmeshes.end();it++) {
				collision_meshref* ref = it->second;
				memcpy(&graph->collision_meshes[i], &ref->mesh, sizeof(collision_mesh_data));

				for (u32 e = 0;e < ref->names.size();e++) {
					graph->nodes[cn].type = nt_collision;
					graph->nodes[cn].name_len = ref->names[e].length();
					graph->nodes[cn].name = new char[ref->names[e].length()];
					memcpy(graph->nodes[cn].name, ref->names[e].data(), ref->names[e].length());
					graph->nodes[cn].collision.collision_mesh_idx = i;
					graph->nodes[cn].transform = ref->transforms[e];
					cn++;
				}

				i++;
			}
		}
		else graph->collision_meshes = nullptr;

		for (u32 i = 0;i < s->mNumCameras;i++) {
			aiCamera* cam = s->mCameras[i];
			aiNode* node = s->mRootNode->FindNode(cam->mName);
			aiNode* target = s->mRootNode->FindNode((mstring(cam->mName.data, cam->mName.length) + ".Target").c_str());

			graph->nodes[cn].type = nt_camera;
			graph->nodes[cn].name_len = cam->mName.length;
			graph->nodes[cn].name = new char[cam->mName.length];
			memcpy(graph->nodes[cn].name, cam->mName.data, cam->mName.length);
			graph->nodes[cn].transform = get_transform(node);
			graph->nodes[cn].cam.position = graph->nodes[cn].transform * vec4f(0, 0, 0, 1);
			graph->nodes[cn].cam.target = get_transform(target) * vec4f(0, 0, 0, 1);
			graph->nodes[cn].cam.fieldOfView = glm::degrees(cam->mHorizontalFOV);
			graph->nodes[cn].cam.width = 0.0f;
			graph->nodes[cn].cam.height = 0.0f;
			graph->nodes[cn].cam.isOrthographic = false;
			cn++;
		}

		for (u32 i = 0;i < s->mNumLights;i++) {
			aiLight* light = s->mLights[i];
			aiNode* lnode = s->mRootNode->FindNode(light->mName);

			r2Log("Light: %s\n", light->mName.C_Str());
			light_type type = lt_none;
			switch (light->mType) {
				case aiLightSource_DIRECTIONAL: { type = lt_directional; break; }
				case aiLightSource_POINT: { type = lt_point; break; }
				case aiLightSource_SPOT: { type = lt_spot; break; }
				default: {
					r2Warn("Light: %s has unsupported type.\n", light->mName.C_Str());
				}
			};

			graph->nodes[cn].type = nt_light;
			graph->nodes[cn].name_len = light->mName.length;
			graph->nodes[cn].name = new char[light->mName.length];
			memcpy(graph->nodes[cn].name, light->mName.data, light->mName.length);
			graph->nodes[cn].transform = get_transform(lnode);
			graph->nodes[cn].light.type = type;
			graph->nodes[cn].light.color = vec3f(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b);
			graph->nodes[cn].light.coneInnerAngle = glm::degrees(light->mAngleInnerCone) * 0.5f;
			graph->nodes[cn].light.coneOuterAngle = glm::degrees(light->mAngleOuterCone) * 0.5f;
			graph->nodes[cn].light.constantAttenuation = light->mAttenuationConstant;
			graph->nodes[cn].light.linearAttenuation = light->mAttenuationLinear;
			graph->nodes[cn].light.quadraticAttenuation = light->mAttenuationQuadratic;
			cn++;
		}

		for (u32 i = 0;i < pois.size();i++) {
			graph->nodes[cn].type = nt_pointOfInterest;
			graph->nodes[cn].name_len = pois[i].name.length();
			graph->nodes[cn].name = new char[pois[i].name.length()];
			memcpy(graph->nodes[cn].name, pois[i].name.data(), pois[i].name.length());
			graph->nodes[cn].transform = pois[i].transform;
			cn++;
		}

		return graph;
	}

	rosen_space::scene_graph* rosen_space::load_space(const mstring& file) {
		data_container* fp = r2engine::files()->open(file, DM_BINARY);
		if (fp) {
			char hdr[4] = { 0 };
			if (!fp->read(hdr)) {
				r2engine::files()->destroy(fp);
				return nullptr;
			}

			if (hdr[0] != 'S' || hdr[1] != 'P' || hdr[2] != 'C' || hdr[3] != 'E') {
				r2Error("%s is not a valid SPCE file", file.c_str());
				r2engine::files()->destroy(fp);
				return nullptr;
			}

			scene_graph* sg = new scene_graph();
			memset(sg, 0, sizeof(scene_graph));

			if (!fp->read(sg->mesh_count)) {
				delete sg;
				r2engine::files()->destroy(fp);
				return nullptr;
			}

			sg->meshes = new mesh_data[sg->mesh_count];
			for (u16 i = 0;i < sg->mesh_count;i++) {
				if (!fp->read(sg->meshes[i].material.diffuse)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->meshes[i].material.texture_len)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				sg->meshes[i].material.texture = new char[sg->meshes[i].material.texture_len];
				if (!fp->read_data(sg->meshes[i].material.texture, sg->meshes[i].material.texture_len)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->meshes[i].vertex_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				sg->meshes[i].vertices = new mesh_data::vertex[sg->meshes[i].vertex_count];
				if (!fp->read_data(sg->meshes[i].vertices, sizeof(mesh_data::vertex) * sg->meshes[i].vertex_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->meshes[i].index_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				sg->meshes[i].indices = new u16[sg->meshes[i].index_count];
				if (!fp->read_data(sg->meshes[i].indices, sizeof(u16) * sg->meshes[i].index_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->meshes[i].instance_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}
			}

			if (!fp->read(sg->collider_count)) {
				delete sg;
				r2engine::files()->destroy(fp);
				return nullptr;
			}

			sg->collision_meshes = new collision_mesh_data[sg->collider_count];
			for (u16 i = 0;i < sg->collider_count;i++) {
				if (!fp->read(sg->collision_meshes[i].vertex_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				sg->collision_meshes[i].vertices = new vec3f[sg->collision_meshes[i].vertex_count];
				if (!fp->read_data(sg->collision_meshes[i].vertices, sizeof(vec3f) * sg->collision_meshes[i].vertex_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->collision_meshes[i].index_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				sg->collision_meshes[i].indices = new u16[sg->collision_meshes[i].index_count];
				if (!fp->read_data(sg->collision_meshes[i].indices, sizeof(u16) * sg->collision_meshes[i].index_count)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}
			}

			if (!fp->read(sg->node_count)) {
				delete sg;
				r2engine::files()->destroy(fp);
				return nullptr;
			}
			sg->nodes = new scene_node[sg->node_count];
			for (u16 i = 0;i < sg->node_count;i++) {
				if (!fp->read(sg->nodes[i].type)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->nodes[i].name_len)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				sg->nodes[i].name = new char[sg->nodes[i].name_len];
				if (!fp->read_data(sg->nodes[i].name, sg->nodes[i].name_len)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				if (!fp->read(sg->nodes[i].transform)) {
					delete sg;
					r2engine::files()->destroy(fp);
					return nullptr;
				}

				switch (sg->nodes[i].type) {
					case nt_mesh: {
						if (!fp->read(sg->nodes[i].mesh)) {
							delete sg;
							r2engine::files()->destroy(fp);
							return nullptr;
						}
						break;
					}
					case nt_light: {
						if (!fp->read(sg->nodes[i].light)) {
							delete sg;
							r2engine::files()->destroy(fp);
							return nullptr;
						}
						break;
					}
					case nt_camera: {
						if (!fp->read(sg->nodes[i].cam)) {
							delete sg;
							r2engine::files()->destroy(fp);
							return nullptr;
						}
						break;
					}
					case nt_collision: {
						if (!fp->read(sg->nodes[i].collision)) {
							delete sg;
							r2engine::files()->destroy(fp);
							return nullptr;
						}
						break;
					}
					case nt_pointOfInterest: {
						break;
					}
					default: {
						r2Error("Invalid node type encountered in %s", file.c_str());
						delete sg;
						r2engine::files()->destroy(fp);
						return nullptr;
					}
				}
			}

			r2engine::files()->destroy(fp);

			return sg;
		}

		return nullptr;
	}

	bool rosen_space::save_space(const mstring& file, rosen_space::scene_graph* space) {
		data_container* fp = r2engine::files()->create(DM_BINARY, file);

		char hdr[4] = { 'S', 'P', 'C', 'E' };
		if (!fp->write(hdr)) {
			r2engine::files()->destroy(fp);
			return false;
		}

		if (!fp->write(space->mesh_count)) {
			r2engine::files()->destroy(fp);
			return false;
		}

		for (u16 i = 0;i < space->mesh_count;i++) {
			if (!fp->write(space->meshes[i].material.diffuse)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->meshes[i].material.texture_len)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write_data(space->meshes[i].material.texture, space->meshes[i].material.texture_len)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->meshes[i].vertex_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write_data(space->meshes[i].vertices, sizeof(mesh_data::vertex) * space->meshes[i].vertex_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->meshes[i].index_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write_data(space->meshes[i].indices, sizeof(u16) * space->meshes[i].index_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->meshes[i].instance_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}
		}

		if (!fp->write(space->collider_count)) {
			r2engine::files()->destroy(fp);
			return false;
		}

		for (u16 i = 0;i < space->collider_count;i++) {
			if (!fp->write(space->collision_meshes[i].vertex_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write_data(space->collision_meshes[i].vertices, sizeof(vec3f) * space->collision_meshes[i].vertex_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->collision_meshes[i].index_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write_data(space->collision_meshes[i].indices, sizeof(u16) * space->collision_meshes[i].index_count)) {
				r2engine::files()->destroy(fp);
				return false;
			}
		}

		if (!fp->write(space->node_count)) {
			r2engine::files()->destroy(fp);
			return false;
		}

		for (u16 i = 0;i < space->node_count;i++) {
			if (!fp->write(space->nodes[i].type)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->nodes[i].name_len)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write_data(space->nodes[i].name, space->nodes[i].name_len)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			if (!fp->write(space->nodes[i].transform)) {
				r2engine::files()->destroy(fp);
				return false;
			}

			switch (space->nodes[i].type) {
				case nt_mesh: {
					if (!fp->write(space->nodes[i].mesh)) {
						r2engine::files()->destroy(fp);
						return false;
					}
					break;
				}
				case nt_light: {
					if (!fp->write(space->nodes[i].light)) {
						r2engine::files()->destroy(fp);
						return false;
					}
					break;
				}
				case nt_camera: {
					if (!fp->write(space->nodes[i].cam)) {
						r2engine::files()->destroy(fp);
						return false;
					}
					break;
				}
				case nt_collision: {
					if (!fp->write(space->nodes[i].collision)) {
						r2engine::files()->destroy(fp);
						return false;
					}
					break;
				}
				case nt_pointOfInterest: {
					break;
				}
			}
		}

		bool ret = r2engine::files()->save(fp, file);
		r2engine::files()->destroy(fp);

		return ret;
	}

	void rosen_space::init_script_obj(Local<Object>& obj) {
		auto isolate = r2engine::isolate();
		obj->Set(v8str("get_element"), v8pp::wrap_function(isolate, "get_element", [this](v8Args args) {
			if (args.Length() == 0 || args[0].IsEmpty() || !args[0]->IsString()) {
				args.GetReturnValue().Set(Null(args.GetIsolate()));
				return;
			}

			mstring name = v8pp::convert<mstring>::from_v8(args.GetIsolate(), args[0]);
			for (u32 i = 0;i < this->m_elements.size();i++) {
				if ((*m_elements[i])->name() == name) {
					args.GetReturnValue().Set((*m_elements[i])->script_obj());
					return;
				}
			}
		}));

		obj->Set(v8str("get_collider"), v8pp::wrap_function(isolate, "get_collider", [this](v8Args args) {
			if (args.Length() == 0 || args[0].IsEmpty() || !args[0]->IsString()) {
				args.GetReturnValue().Set(Null(args.GetIsolate()));
				return;
			}

			mstring name = v8pp::convert<mstring>::from_v8(args.GetIsolate(), args[0]);
			for (u32 i = 0;i < m_colliders.size();i++) {
				if ((*m_colliders[i])->name() == name) {
					args.GetReturnValue().Set((*m_colliders[i])->script_obj());
					return;
				}
			}
		}));

		obj->Set(v8str("get_light"), v8pp::wrap_function(isolate, "get_light", [this](v8Args args) {
			if (args.Length() == 0 || args[0].IsEmpty() || !args[0]->IsString()) {
				args.GetReturnValue().Set(Null(args.GetIsolate()));
				return;
			}

			mstring name = v8pp::convert<mstring>::from_v8(args.GetIsolate(), args[0]);
			for (u32 i = 0;i < m_lights.size();i++) {
				if ((*m_lights[i])->name() == name) {
					args.GetReturnValue().Set((*m_lights[i])->script_obj());
					return;
				}
			}
		}));

		obj->Set(v8str("get_poi"), v8pp::wrap_function(isolate, "get_light", [this](v8Args args) {
			if (args.Length() == 0 || args[0].IsEmpty() || !args[0]->IsString()) {
				args.GetReturnValue().Set(Null(args.GetIsolate()));
				return;
			}

			mstring name = v8pp::convert<mstring>::from_v8(args.GetIsolate(), args[0]);
			if (m_pointsOfInterest.has(name)) {
				args.GetReturnValue().Set(v8pp::convert<mat4f>::to_v8(args.GetIsolate(), *m_pointsOfInterest.get(name)));
			} else args.GetReturnValue().Set(Null(args.GetIsolate()));
		}));

		obj->Set(v8str("activate_camera"), v8pp::wrap_function(isolate, "activate_camera", [this](v8Args args) {
			if (args.Length() == 0 || args[0].IsEmpty() || !args[0]->IsString()) {
				args.GetReturnValue().Set(Null(args.GetIsolate()));
				return;
			}

			f32 duration = 0.0f;
			if (args.Length() >= 2 && v8pp::convert<f32>::is_valid(args.GetIsolate(), args[1])) {
				duration = v8pp::convert<f32>::from_v8(args.GetIsolate(), args[1]);
			}

			mstring name = v8pp::convert<mstring>::from_v8(args.GetIsolate(), args[0]);
			for (u32 i = 0;i < m_cameraNames.size();i++) {
				if (m_cameraNames[i] == name) {
					if (duration > 0.0f) {
						m_camera->cameraPosition.duration(duration);
						m_camera->cameraTarget.duration(duration);
						m_camera->cameraProjection.duration(duration);
					}
					set_current_camera(i, duration == 0.0f);
					return;
				}
			}

			r2Warn("Camera '%s' not found in current space", name.c_str());
		}));

		obj->SetAccessorProperty(v8str("elements"), v8pp::wrap_function(isolate, "elements", [this](v8Args args) {
			Local<Array> arr = Array::New(args.GetIsolate(), m_elements.size());
			for (u32 i = 0;i < m_elements.size();i++) {
				arr->Set(i, (*m_elements[i])->script_obj());
			}
			args.GetReturnValue().Set(arr);
		}));

		obj->SetAccessorProperty(v8str("colliders"), v8pp::wrap_function(isolate, "colliders", [this](v8Args args) {
			Local<Array> arr = Array::New(args.GetIsolate(), m_colliders.size());
			for (u32 i = 0;i < m_colliders.size();i++) {
				arr->Set(i, (*m_colliders[i])->script_obj());
			}
			args.GetReturnValue().Set(arr);
		}));

		obj->SetAccessorProperty(v8str("lights"), v8pp::wrap_function(isolate, "lights", [this](v8Args args) {
			Local<Array> arr = Array::New(args.GetIsolate(), m_lights.size());
			for (u32 i = 0;i < m_lights.size();i++) {
				arr->Set(i, (*m_lights[i])->script_obj());
			}
			args.GetReturnValue().Set(arr);
		}));

		obj->Set(v8str("spawn_rosen"), v8pp::wrap_function(isolate, "spawn_rosen", [this](v8Args args) {
			if (args.Length() < 2 || args[0].IsEmpty() || !args[0]->IsString() || args[1].IsEmpty() || !v8pp::convert<mat4f>::is_valid(args.GetIsolate(), args[1])) {
				args.GetReturnValue().Set(Null(args.GetIsolate()));
				return;
			}

			mstring name = v8pp::convert<mstring>::from_v8(args.GetIsolate(), args[0]);
			mat4f transform = v8pp::convert<mat4f>::from_v8(args.GetIsolate(), args[1]);

			rosen_entity* rosen = spawn_rosen(name, transform);
			rosen->use_physics = args.Length() >= 3 && !args[2].IsEmpty() && args[2]->IsBoolean() ? v8pp::convert<bool>::from_v8(args.GetIsolate(), args[2]) : true;
			args.GetReturnValue().Set(rosen->script_obj());
		}));

		m_scriptObj.Reset(r2engine::isolate(), obj);

		Local<Value> val = obj->Get(v8str("initialize"));
		if (!val.IsEmpty() && val->IsFunction()) m_init.Reset(isolate, Local<Function>::Cast(val));

		val = obj->Get(v8str("deinitialize"));
		if (!val.IsEmpty() && val->IsFunction()) m_deinit.Reset(isolate, Local<Function>::Cast(val));

		val = obj->Get(v8str("update"));
		if (!val.IsEmpty() && val->IsFunction()) m_update.Reset(isolate, Local<Function>::Cast(val));
	}
	


	rosen_space::scene_graph::~scene_graph() {
		if (meshes) {
			for (u16 i = 0;i < mesh_count;i++) {
				if (meshes[i].material.texture) delete [] meshes[i].material.texture;
				if (meshes[i].vertices) delete [] meshes[i].vertices;
				if (meshes[i].indices) delete [] meshes[i].indices;
			}
			delete [] meshes;
			meshes = nullptr;
			mesh_count = 0;
		}

		if (collision_meshes) {
			for (u16 i = 0;i < collider_count;i++) {
				if (collision_meshes[i].indices) delete [] collision_meshes[i].indices;
				if (collision_meshes[i].vertices) delete [] collision_meshes[i].vertices;
			}
			delete [] collision_meshes;
			collision_meshes = nullptr;
			collider_count = 0;
		}

		if (nodes) {
			for (u32 i = 0;i < node_count;i++) {
				if (nodes[i].name) delete [] nodes[i].name;
			}
			delete [] nodes;
			nodes = nullptr;
			node_count = 0;
		}
	}

	mat4f rosen_space::camera_node::projection() const {
		if (isOrthographic) return glm::ortho(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, 0.01f, 100.0f);
		else {
			vec2f screen = r2engine::get()->window()->get_size();
			return glm::perspective(glm::radians(fieldOfView), screen.x / screen.y, 0.01f, 100.0f);
		}
	}



	space_man::space_man(scene* s) {
		m_scene = s;
		m_vformat = new vertex_format();
		m_vformat->add_attr(vat_vec3f); // position
		m_vformat->add_attr(vat_vec3f); // normal
		m_vformat->add_attr(vat_vec2f); // texcoord

		m_iformat = new instance_format();
		m_iformat->add_attr(iat_mat4f, true); // transform
		m_iformat->add_attr(iat_mat3f); // normal_transform
		m_iformat->add_attr(iat_int); // entity id

		m_mformat = new uniform_format();
		m_mformat->add_attr("diffuse", uat_vec4f);

		m_element_shader = s->load_shader("./resources/shader/space_element.glsl", "space_element_shader");
		m_element_material = new node_material("material", m_mformat);
		m_element_material->set_shader(m_element_shader);



		m_rosen_vfmt = new vertex_format();
		m_rosen_vfmt->add_attr(vat_vec3f);
		m_rosen_vfmt->add_attr(vat_vec2f);

		m_rosen_ifmt = new instance_format();
		m_rosen_ifmt->add_attr(iat_mat4f, true); // transform
		m_rosen_ifmt->add_attr(iat_int); // entity id

		m_rosen_mformat = new uniform_format();
		m_rosen_mformat->add_attr("shirt_tint", uat_vec3f);
		m_rosen_shader = s->load_shader("./resources/shader/rosen.glsl", "rosen_shader");
		m_rosen_material = new node_material("material", m_rosen_mformat);
		m_rosen_material->set_shader(m_rosen_shader);

		m_current = nullptr;
		directory_info* info = r2engine::files()->parse_directory("./resources/space");
		if (info) {
			for (u32 i = 0;i < info->entry_count();i++) {
				directory_entry* entry = info->entry(i);
				if (entry->Type == DET_FOLDER && entry->Name != "." && entry->Name != "..") {
					m_spaces.push(new rosen_space(entry->Name, this));
				}
			}
			r2engine::files()->destroy_directory(info);
		}
	}

	space_man::~space_man() {
		m_spaces.for_each([](rosen_space** space) {
			delete *space;
			return true;
		});
	}

	void space_man::load_space(rosen_space* space) {
		if (m_current) m_current->unload();
		space->load();
		m_current = space;
	}
};