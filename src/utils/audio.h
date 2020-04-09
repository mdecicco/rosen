#include <r2/config.h>
#include <r2/managers/memman.h>

namespace r2 {
	class audio_source;
	class audio_buffer;
};

namespace rosen {
	#pragma pack(push, 1)
	class audio_container {
		public:
			audio_container(const r2::mstring& name);
			~audio_container();

			r2::audio_buffer* buffer();

		protected:
			r2::audio_buffer* m_audioBuffer;
		};
	#pragma pack(pop)
};