#include <utils/video.h>
#include <r2/engine.h>
#include <zlib.h>

using namespace r2;

namespace rosen {
	video_container::video_container(const mstring& name, scene* _scene) {
		m_scene = _scene;

		m_video = r2engine::files()->open("./resources/video/" + name + "/video.vid", DM_BINARY);
		frames = nullptr;
		memset(&info, 0, sizeof(header));

		if (m_video) {
			if (m_video->read(info)) {
				frames = new frameinfo[info.framecount];
				for (u32 i = 0;i < info.framecount;i++) {
					if (!m_video->read(frames[i].size)) {
						delete frames;
						r2engine::files()->destroy(m_video);
						m_video = nullptr;
						frames = nullptr;
						return;
					}
					frames[i].offset = m_video->position();
					m_video->seek(frames[i].size);
				}
			}
		}
	}

	video_container::~video_container() {
		if (m_video) r2engine::files()->destroy(m_video);
		if (frames) delete [] frames;
	}

	texture_buffer* video_container::frame(u32 frameIdx, texture_buffer* tex) {
		if (frameIdx >= info.framecount) {
			r2Error("video_tester::frame(): Specified index out of range");
			return nullptr;
		}

		m_video->set_position(frames[frameIdx].offset);
		size_t decompressed_size = info.width * info.height * 3;
		u8* compressed = new u8[frames[frameIdx].size];
		u8* decompressed = new u8[decompressed_size];
		if (!m_video->read_data(compressed, frames[frameIdx].size)) {
			delete [] compressed;
			delete [] decompressed;
			return nullptr;
		}

		uLongf out_size = decompressed_size;
		if (uncompress(decompressed, &out_size, compressed, frames[frameIdx].size) != Z_OK) {
			r2Error("Failed to decompress frame %d", frameIdx);
			delete [] compressed;
			delete [] decompressed;
			return nullptr;
		}
		delete [] compressed;

		if (!tex) tex = m_scene->create_texture();
		tex->create(decompressed, info.width, info.height, 3, tt_unsigned_byte);
		delete [] decompressed;

		return tex;
	}
};