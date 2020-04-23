#include <managers/space_man.h>
#include <utils/lode_png.h>
#include <entities/rosen_cam.h>
#include <entities/space_element.h>
#include <entities/space_collision_element.h>

#include <r2/engine.h>
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

	mat4f get_transform(aiNode* node) {
		aiMatrix4x4 transform = node->mTransformation;
		aiNode* parent = node->mParent;
		while (parent) {
			transform = parent->mTransformation * transform;
			parent = parent->mParent;
		}

		return assimp_to_glm(transform);
	}



	rosen_space::rosen_space(const mstring& space_name, space_man* mgr) {
		m_mgr = mgr;
		m_camera = new rosen_camera_entity(space_name + "_camera");

		Assimp::Importer imp;
		const aiScene* s = imp.ReadFile("./resources/space/" + space_name + "/space.fbx", aiProcess_Triangulate);

		for (u32 n = 0;n < s->mRootNode->mNumChildren;n++) {
			aiNode* a_node = s->mRootNode->mChildren[n];

			mvector<aiNode*> meshNodes;
			get_meshes(s, a_node, meshNodes);

			for (u32 mn = 0;mn < meshNodes.size();mn++) {
				aiNode* cnode = meshNodes[mn];
				for (u32 i = 0;i < cnode->mNumMeshes;i++) {
					aiMesh* mesh = s->mMeshes[cnode->mMeshes[i]];
					mstring name(mesh->mName.data, mesh->mName.length);

					if (name.find("collision") != mstring::npos) {
						r2Log("Collision: %s", name.c_str());
						btTriangleMesh* tmesh = new btTriangleMesh(false, false);

						for (u16 f = 0;f < mesh->mNumFaces;f++) {
							aiFace& face = mesh->mFaces[f];
							aiVector3D& v0 = mesh->mVertices[face.mIndices[0]];
							aiVector3D& v1 = mesh->mVertices[face.mIndices[1]];
							aiVector3D& v2 = mesh->mVertices[face.mIndices[2]];
							tmesh->addTriangle(
								btVector3(v0.x, v0.y, v0.z),
								btVector3(v1.x, v1.y, v1.z),
								btVector3(v2.x, v2.y, v2.z)
							);
						}

						m_colliders.push(new space_collision_element_entity(name, get_transform(cnode), tmesh));
						continue;
					}

					r2Log("Mesh: %s", name.c_str());

					if (!mesh->HasNormals()) {
						r2Warn("Mesh has no normals. skipping");
						continue;
					}

					if (!mesh->HasTextureCoords(0)) {
						r2Warn("Mesh has no texcoords. skipping");
						continue;
					}

					mesh_construction_data mcd(mgr->get_vfmt(), it_unsigned_short, mgr->get_ifmt());
					mcd.set_max_vertex_count(mesh->mNumVertices);
					mcd.set_max_index_count(mesh->mNumFaces * 3);

					bool validFaces = true;
					for (u32 f = 0;f < mesh->mNumFaces && validFaces;f++) {
						aiFace& face = mesh->mFaces[f];
						validFaces = face.mNumIndices == 3;
						if (validFaces) {
							mcd.append_index<u16>(face.mIndices[0]);
							mcd.append_index<u16>(face.mIndices[1]);
							mcd.append_index<u16>(face.mIndices[2]);
						}
					}
					if (!validFaces) {
						r2Warn("Mesh has invalid face index count. skipping");
						continue;
					}

					struct vertex {
						vec3f pos;
						vec3f norm;
						vec2f tex;
					};
					for (u32 v = 0;v < mesh->mNumVertices;v++) {
						mcd.append_vertex<vertex>({
							{ mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z },
							{ mesh->mNormals[v].x, mesh->mNormals[v].z, mesh->mNormals[v].y },
							{ mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y }
						});
					}

					render_node* node = m_mgr->get_scene()->add_mesh(&mcd);

					node_material_instance* mat_instance = m_mgr->get_element_material()->instantiate(m_mgr->get_scene());

					aiMaterial* material = s->mMaterials[mesh->mMaterialIndex];
					aiColor4D color;
					if (material->Get("$clr.diffuse", 0, 0, color) == aiReturn_SUCCESS) mat_instance->uniforms()->uniform_vec4f("diffuse", vec4f(color.r, color.g, color.b, color.a));
					else mat_instance->uniforms()->uniform_vec4f("diffuse", vec4f(0.5f, 0.5f, 0.5f, 1.0f));

					aiString path;
					if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == aiReturn_SUCCESS) {
						mstring tex(path.data, path.length);

						mvector<u8> pixels;
						u32 w, h;
						u32 err = lodepng::decode(pixels, w, h, "./resources/space/" + space_name + "/" + tex);
						if (err) {
							r2Warn("Failed to load texture for %s: %s", name.c_str(), lodepng_error_text(err));
						}
						else {
							texture_buffer* tbuf = m_mgr->get_scene()->create_texture();
							tbuf->create(pixels.data(), w, h, 4, tt_unsigned_byte);
							tbuf->set_min_filter(tmnf_linear);
							tbuf->set_mag_filter(tmgf_linear);
							mat_instance->set_texture("diffuse_tex", tbuf);
						}
					}

					node->set_material_instance(mat_instance);

					m_elements.push(new space_element_entity(name, node, get_transform(cnode)));
				}
			}
		}

		for (u32 i = 0;i < s->mNumCameras;i++) {
			aiCamera* cam = s->mCameras[i];
			aiNode* node = s->mRootNode->FindNode(cam->mName);
			aiNode* target = s->mRootNode->FindNode((mstring(cam->mName.data, cam->mName.length) + ".Target").c_str());
			
			if (node && target) {
				m_cameraAngles.push({
					vec3f(get_transform(node) * vec4f(0.0f, 0.0f, 0.0f, 1.0f)),
					vec3f(get_transform(target) * vec4f(0.0f, 0.0f, 0.0f, 1.0f)),
					glm::degrees(cam->mHorizontalFOV),
					0.0f,
					0.0f,
					false
				});
			}
		}

		for (u32 i = 0;i < s->mNumLights;i++) {
			aiLight* light = s->mLights[i];
			aiNode* lnode = s->mRootNode->FindNode(light->mName);

			if (lnode) {
				r2Log("Light: %s\n", light->mName.C_Str());
				mat4f transform = get_transform(lnode);
				vec3f pos = transform * vec4f(0,  0, 0, 1);
				vec3f dir = transform * vec4f(0, -1, 0, 1);
				dir = glm::normalize(dir - pos);

				i32 type = 0;
				switch (light->mType) {
					case aiLightSource_DIRECTIONAL: { type = 1; break; }
					case aiLightSource_POINT: { type = 2; break; }
					case aiLightSource_SPOT: { type = 3; break; }
					default: {
						r2Warn("Light: %s has no unsupported type. ignoring\n", light->mName.C_Str());
					}
				};


				if (type) {
					m_lights.push({
						type,
						pos,
						dir,
						vec3f(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b),
						cosf(light->mAngleInnerCone * 0.5f),
						cosf(light->mAngleOuterCone * 0.5f),
						light->mAttenuationConstant,
						light->mAttenuationLinear,
						light->mAttenuationQuadratic,
					});
				}
			} else {
				r2Warn("Light: %s has no corresponding node. ignoring\n", light->mName.C_Str());
			}
		}

		data_container* script = r2engine::files()->open("./resources/space/" + space_name + "/space.js", DM_TEXT, space_name + "_script");
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

								obj->Set(v8str("get_collider"), v8pp::wrap_function(isolate, "get_collider", [this](const mstring& name) {
									for (u32 i = 0;i < this->m_colliders.size();i++) {
										if ((*m_colliders[i])->name() == name) {
											return (scene_entity*)*m_colliders[i];
										}
									}
								}));

								m_scriptObj.Reset(r2engine::isolate(), obj);
								
								Local<Value> val = obj->Get(v8str("initialize"));
								if (!val.IsEmpty() && val->IsFunction()) {
									Local<Function> ifunc = Local<Function>::Cast(val);
									m_init.Reset(isolate, ifunc);
									ifunc->Call(isolate->GetCurrentContext(), instance, 0, nullptr);
								}

								val = obj->Get(v8str("deinitialize"));
								if (!val.IsEmpty() && val->IsFunction()) m_deinit.Reset(isolate, Local<Function>::Cast(val));

								val = obj->Get(v8str("update"));
								if (!val.IsEmpty() && val->IsFunction()) m_update.Reset(isolate, Local<Function>::Cast(val));
							}
						}
					}
				}
			} else {
				r2Warn("Script for space '%s' doesn't contain a class. Ignoring", space_name.c_str());
			}
		}
	}

	rosen_space::~rosen_space() {
		if (!m_deinit.IsEmpty()) {
			auto isolate = r2engine::isolate();
			m_deinit.Get(isolate)->Call(isolate->GetCurrentContext(), m_scriptObj.Get(isolate), 0, nullptr);
		}

		m_elements.for_each([this](space_element_entity** element) {
			render_node* node = (*element)->node();
			delete node->material_instance();
			this->m_mgr->get_scene()->remove_node(node);
			(*element)->destroy();
			return true;
		});

		m_colliders.for_each([this](space_collision_element_entity** element) {
			(*element)->destroy();
			return true;
		});

		m_scriptObj.Reset();
		m_init.Reset();
		m_deinit.Reset();
		m_update.Reset();
	}

	void rosen_space::update(f32 dt) {
		if (!m_update.IsEmpty()) {
			auto isolate = r2engine::isolate();
			Local<Value> args[] = {
				v8pp::convert<f32>::to_v8(isolate, dt)
			};
			m_update.Get(isolate)->Call(isolate->GetCurrentContext(), m_scriptObj.Get(isolate), 1, args);
		}
	}

	void rosen_space::update_uniforms() {
		shader_program* shader = m_mgr->get_element_material()->shader();
		shader->activate();
		shader->uniform1i(shader->get_uniform_location("light_count"), m_lights.size());

		char idxStr[8] = { 0 };
		for (u32 i = 0;i < m_lights.size();i++) {
			memset(idxStr, 0, 8);
			snprintf(idxStr, 8, "[%d]", i);
			light_def* light = m_lights[i];
			shader->uniform1i(shader->get_uniform_location(mstring("u_lights") + idxStr + ".type"), light->type);
			shader->uniform3f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".position"), light->position.x, light->position.y, light->position.z);
			shader->uniform3f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".direction"), light->direction.x, light->direction.y, light->direction.z);
			shader->uniform3f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".color"), light->color.x, light->color.y, light->color.z);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".cosConeInnerAngle"), light->cosConeInnerAngle);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".cosConeOuterAngle"), light->cosConeOuterAngle);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".constantAtt"), light->constantAtt);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".linearAtt"), light->linearAtt);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".quadraticAtt"), light->quadraticAtt);
		}
		shader->deactivate();

		shader = m_mgr->get_rosen_shader();
		shader->activate();
		shader->uniform1i(shader->get_uniform_location("light_count"), m_lights.size());

		for (u32 i = 0;i < m_lights.size();i++) {
			memset(idxStr, 0, 8);
			snprintf(idxStr, 8, "[%d]", i);
			light_def* light = m_lights[i];
			shader->uniform1i(shader->get_uniform_location(mstring("u_lights") + idxStr + ".type"), light->type);
			shader->uniform3f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".position"), light->position.x, light->position.y, light->position.z);
			shader->uniform3f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".direction"), light->direction.x, light->direction.y, light->direction.z);
			shader->uniform3f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".color"), light->color.x, light->color.y, light->color.z);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".cosConeInnerAngle"), light->cosConeInnerAngle);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".cosConeOuterAngle"), light->cosConeOuterAngle);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".constantAtt"), light->constantAtt);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".linearAtt"), light->linearAtt);
			shader->uniform1f(shader->get_uniform_location(mstring("u_lights") + idxStr + ".quadraticAtt"), light->quadraticAtt);
		}
		shader->deactivate();
	}

	void rosen_space::set_current_camera(u8 idx, bool noTransition) {
		if (m_currentCamera == idx) {
			if (!m_camera->camera->is_active()) camera_sys::get()->activate_camera(m_camera);
			return;
		}
		m_currentCamera = idx;

		camera_def* cam = m_cameraAngles[idx];

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

	

	mat4f rosen_space::camera_def::projection() const {
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

		m_iformat = nullptr;

		m_mformat = new uniform_format();
		m_mformat->add_attr("diffuse", uat_vec4f);

		m_element_shader = s->load_shader("./resources/shader/space_element.glsl", "space_element_shader");
		m_element_material = new node_material("material", m_mformat);
		m_element_material->set_shader(m_element_shader);


		m_rosen_shader = s->load_shader("./resources/shader/rosen.glsl", "rosen_shader");

		m_current = new rosen_space("test", this);
		m_current->update_uniforms();

		m_spaces.push(m_current);
	}

	space_man::~space_man() {
		m_spaces.for_each([](rosen_space** space) {
			delete *space;
			return true;
		});
	}
};