#include <utils/video.h>
#include <r2/engine.h>
#include <zlib.h>
#include <marl/scheduler.h>
#include <marl/waitgroup.h>
#include <marl/event.h>

using namespace r2;

namespace rosen {
	video_container::video_container(const mstring& name) {
		m_video = r2engine::files()->open("./resources/video/" + name + "/video.vid", DM_BINARY, name + " (video)");
		frames = nullptr;
		memset(&info, 0, sizeof(header));

		if (m_video) {
			if (m_video->read(info)) {
				frames = new frameinfo[info.framecount];
				for (u32 i = 0;i < info.framecount;i++) {
					frames[i].offset = m_video->position();
					if (!m_video->read(frames[i].uncompressed_block_size)) {
						r2Error("video_container: Failed to read frame %d header", i);
						delete frames;
						r2engine::files()->destroy(m_video);
						m_video = nullptr;
						frames = nullptr;
						return;
					}
					if (!m_video->read(frames[i].block_count)) {
						r2Error("video_container: Failed to read frame %d header", i);
						delete frames;
						r2engine::files()->destroy(m_video);
						m_video = nullptr;
						frames = nullptr;
						return;
					}
					for (u32 b = 0;b < frames[i].block_count;b++) {
						u32 sz = 0;
						if (!m_video->read(sz)) {
							delete frames;
							r2engine::files()->destroy(m_video);
							m_video = nullptr;
							frames = nullptr;
							return;
						}
						m_video->seek(sz);
					}
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
			r2Warn("video_container::frame(): Specified index out of range");
			return tex;
		}

		m_video->set_position(frames[frameIdx].offset);
		m_video->seek(6); // skip uncompressed_block_size, block_count

		size_t decompressed_size = info.width * info.height * 3;
		u8** block_bufs = new u8*[frames[frameIdx].block_count];
		u32* block_sizes = new u32[frames[frameIdx].block_count];

		bool did_error = false;
		bool* pdid_error = &did_error;

		for (u32 i = 0;i < frames[frameIdx].block_count;i++) {
			block_bufs[i] = new u8[frames[frameIdx].uncompressed_block_size];
			if (!m_video->read(block_sizes[i])) {
				r2Error("video_container::frame(): Failed to read frame %d block %d header", frameIdx, i);
				delete block_bufs[i];
				for (u32 b = 0;b < i;b++) delete [] block_bufs[b];
				delete [] block_bufs;
				delete [] block_sizes;
				return tex;
			}

			if (!m_video->read_data(block_bufs[i], block_sizes[i])) {
				r2Error("video_container::frame(): Failed to read frame %d block %d data", frameIdx, i);
				delete block_bufs[i];
				for (u32 b = 0;b < i;b++) delete [] block_bufs[b];
				delete [] block_bufs;
				delete [] block_sizes;
				return tex;
			}
		}
		if (!tex) tex = r2engine::current_scene()->create_texture();
		if (tex->width() != info.width || tex->height() != info.height) tex->create(info.width, info.height, 3, tt_unsigned_byte, false);
		else tex->updated(0, tex->used_size());

		u32 decomp_size = frames[frameIdx].uncompressed_block_size;
		for (u32 i = 0;i < frames[frameIdx].block_count;i++) {
			u8* dest = ((u8*)tex->data()) + (size_t(i) * size_t(frames[frameIdx].uncompressed_block_size));
			u8* src = block_bufs[i];
			u32 block_size = block_sizes[i];
			marl::schedule([pdid_error, dest, src, block_size, decomp_size]{
				uLongf out_size = decomp_size;
				if (uncompress(dest, &out_size, src, block_size) != Z_OK) *pdid_error = true;
				delete [] src;
			});
		}

		delete [] block_bufs;
		delete [] block_sizes;

		if (did_error) {
			r2Error("video_container::frame(): Failed to decompress frame %d", frameIdx);
			return tex;
		}
		return tex;
	}
};