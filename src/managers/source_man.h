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

	// left/right refer to the image's perspective, not Michael's
	enum bone_index {
		// Highest point on the head
		bi_head_top,
		bi_chin,

		// Just above Michael's ears (or approximately where they would be if they were visible)
		bi_head_left,
		bi_head_right,

		// Hand bottom refers to the side opposite of the thumb
		// Hand top refers to the side that the thumb is on
		bi_left_hand_bottom,
		bi_left_hand_top,
		bi_right_hand_bottom,
		bi_right_hand_top,

		// pupils
		bi_left_eye,
		bi_right_eye,

		// Tip of the nose
		bi_nose,

		// Center of the mouth
		bi_mouth,

		// Ear holes
		bi_left_ear,
		bi_right_ear,

		bi_bone_count
	};

	class source_content {
		public:
			source_content(const r2::mstring& name);
			~source_content();

			struct snippet {
				r2::f32 start;
				r2::f32 end;
				r2::mstring text;
			};

			struct keyframe {
				r2::f32 time;
				r2::vec2f position;
				bool hidden;
			};

			struct bone {
				bool dragging;

				void set(r2::f32 time, r2::vec2f pos);
				void set(r2::f32 time, bool hidden);
				r2::vec2f position(r2::f32 time) const;
				bool hidden(r2::f32 time) const;
				r2::mlist<keyframe> frames;
			};

			inline const r2::mstring name() const { return m_name; }
			inline const char* cname() const { return m_name.c_str(); }

			r2::audio_buffer* audio() const;
			video_container* video() const;
			r2::texture_buffer* frame(r2::u32 frameId, r2::texture_buffer* tex) const;
			r2::texture_buffer* frame(r2::f32 playbackTime, r2::texture_buffer* tex) const;
			r2::u32 frameId(r2::f32 playbackTime) const;
			r2::f32 duration() const;

			void save_snippets();
			void save_bones();

			r2::mvector<snippet> snippets;
			bone bones[bi_bone_count];

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
		void append(speech_plan* plan);

		r2::u32 update(r2::audio_source* audio, r2::texture_buffer* texture, r2::u32 lastSnippetIndex);
		
		r2::dynamic_pod_array<snippet> snippets;
		r2::f32 duration = 0;
	};

	struct premixed_word {
		speech_plan* plan;
		r2::mstring text;
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

			speech_plan* plan_speech(const r2::mstring& text, r2::i32 using_source_idx = -1);

			void save_premixes();

			r2::mvector<premixed_word> mixedWords;

		protected:
			r2::mvector<source_content*> m_sources;
	};
};