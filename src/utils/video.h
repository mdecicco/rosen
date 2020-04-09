#include <r2/config.h>
#include <r2/managers/memman.h>

namespace r2 {
	class texture_buffer;
	class data_container;
	class scene;
};

namespace rosen {
	#pragma pack(push, 1)
	class video_container {
		public:
			struct header {
				r2::u16 width;
				r2::u16 height;
				r2::u16 framerate;
				r2::u32 framecount;
			};

			struct frameinfo {
				r2::u32 size;
				r2::u32 offset;
			};

			video_container(const r2::mstring& name, r2::scene* scene);
			~video_container();

			r2::texture_buffer* frame(r2::u32 frameIdx, r2::texture_buffer* tex = nullptr);

			header info;
			frameinfo* frames;

		protected:
			r2::data_container* m_video;
			r2::scene* m_scene;
	};
	#pragma pack(pop)
};