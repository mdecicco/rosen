#include <utils/audio.h>
#include <r2/engine.h>
#include <zlib.h>

using namespace r2;

namespace rosen {
	audio_container::audio_container(const mstring& name) {
		data_container* audio = r2engine::files()->open("./resources/video/" + name + "/audio.aud", DM_BINARY, name + " (audio)");

		#pragma pack(push, 1)
		struct header {
			u8 sample_size;
			u32 sample_rate;
			u32 sample_count;
			u32 decompressed_size;
			u32 compressed_size;
		} info;
		#pragma pack(pop)

		if (!audio->read(info)) {
			r2engine::files()->destroy(audio);
			return;
		}

		u8* compressed = new u8[info.compressed_size];
		u8* decompressed = new u8[info.decompressed_size];
		audio->read_data(compressed, info.compressed_size);
		uLongf out_size = info.decompressed_size;
		uncompress(decompressed, &out_size, compressed, info.compressed_size);
		r2engine::files()->destroy(audio);
		delete [] compressed;

		m_audioBuffer = new audio_buffer();
		m_audioBuffer->fill(asf_mono_16bit, decompressed, out_size, info.sample_rate);

		delete [] decompressed;
	}

	audio_container::~audio_container() {
	}

	r2::audio_buffer* audio_container::buffer() {
		return m_audioBuffer;
	}
};