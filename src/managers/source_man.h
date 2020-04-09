#include <r2/managers/memman.h>
namespace r2 {
	class audio_buffer;
	class audio_source;
	class texture_buffer;
	class timer;
};

namespace rosen {
	class video_container;
	class audio_container;

	class source_content {
		public:
			source_content(const r2::mstring& name);
			~source_content();

			struct snippet {
				r2::f32 start;
				r2::f32 end;
				r2::mstring text;
			};

			inline const r2::mstring name() const { return m_name; }
			inline const char* cname() const { return m_name.c_str(); }

			r2::audio_buffer* audio() const;
			r2::texture_buffer* frame(r2::u32 frameId, r2::texture_buffer* tex) const;
			r2::texture_buffer* frame(r2::f32 playbackTime, r2::texture_buffer* tex) const;
			r2::u32 frameId(r2::f32 playbackTime) const;
			r2::f32 duration() const;

			r2::mvector<snippet> snippets;

		protected:
			r2::mstring m_name;
			video_container* m_video;
			audio_container* m_audio;
	};

	struct speech_plan {
		struct snippet {
			source_content* source;
			r2::u32 snippetIdx;
		};

		void add(source_content* source, r2::u32 snippetIdx);

		r2::u32 update(r2::audio_source* audio, r2::texture_buffer* texture, r2::u32 lastSnippetIndex);
		
		r2::dynamic_pod_array<snippet> snippets;
		r2::f32 duration = 0;
	};

	struct speech_execution_context {
		speech_execution_context(speech_plan* p);
		~speech_execution_context();

		speech_plan* plan;
		r2::u32 lastSnippetIndex;
		bool completed;

		void reset();

		void update(r2::audio_source* audio, r2::texture_buffer* texture);
	};

	class source_man {
		public:
			source_man();
			~source_man();

			source_content* source(const r2::mstring& name);
			inline source_content* source(size_t index) const { return m_sources[index]; }
			inline size_t source_count() const { return m_sources.size(); }

			speech_plan* plan_speech(const r2::mstring& text);

		protected:
			r2::mvector<source_content*> m_sources;
	};
};