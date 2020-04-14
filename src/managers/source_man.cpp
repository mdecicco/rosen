#include <managers/source_man.h>
#include <utils/audio.h>
#include <utils/video.h>
#include <r2/engine.h>
using namespace r2;

namespace rosen {
	// trim from start (in place)
	static inline void ltrim(mstring &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	static inline void rtrim(mstring &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void trim(mstring &s) {
		ltrim(s);
		rtrim(s);
	}

	static inline mvector<mstring> split(const mstring& str, const mstring& delim) {
		mvector<mstring> tokens;
		size_t prev = 0, pos = 0;
		do {
			pos = str.find(delim, prev);
			if (pos == string::npos) pos = str.length();
			string token = str.substr(prev, pos - prev);
			if (!token.empty()) tokens.push_back(token);
			prev = pos + delim.length();
		}
		while (pos < str.length() && prev < str.length());
		return tokens;
	}



	source_content::source_content(const mstring& name) {
		m_video = new video_container(name);
		m_audio = new audio_container(name);
		m_name = name;

		if (r2engine::files()->exists("./resources/snip/" + name + ".csv")) {
			data_container* snips = r2engine::files()->open("./resources/snip/" + name + ".csv", DM_TEXT);
			if (!snips) return;

			mstring line;
			while (!snips->at_end(1) && snips->read_line(line)) {
				if (line.find_first_of(',') == string::npos) continue;
				mstring cols[3];
				u8 ccol = 0;
				for (u8 x = 0;x < line.length();x++) {
					if (line[x] == ',') {
						ccol++;
					} else if (line[x] == '\n' || line[x] == '\r') break;
					else {
						cols[ccol] += ccol == 0 ? tolower(line[x]) : line[x];
					}
				}

				trim(cols[0]);

				f32 start = atof(cols[1].c_str());
				f32 end = atof(cols[2].c_str());

				if (end >= m_audio->buffer()->duration()) {
					end = m_audio->buffer()->duration() - 0.1f;
				}

				snippets.push_back({
					start,
					end,
					cols[0]
				});
				line = "";
			}

			r2engine::files()->destroy(snips);
		}
	}

	source_content::~source_content() {
		delete m_video;
		delete m_audio;
	}

	audio_buffer* source_content::audio() const {
		return m_audio->buffer();
	}

	texture_buffer* source_content::frame(u32 frameId, texture_buffer* tex) const {
		return m_video->frame(frameId, tex);
	}

	texture_buffer* source_content::frame(f32 playbackTime, texture_buffer* tex) const {
		return m_video->frame(u32(m_video->info.framerate) * playbackTime, tex);
	}

	u32 source_content::frameId(f32 playbackTime) const {
		return u32(m_video->info.framerate) * playbackTime;
	}

	f32 source_content::duration() const {
		return m_audio->buffer()->duration();
	}



	void speech_plan::add(source_content* source, r2::u32 snippetIdx) {
		snippets.push({ source, snippetIdx });
		duration += (source->snippets[snippetIdx].end - source->snippets[snippetIdx].start);
	}

	u32 speech_plan::update(audio_source* audio, texture_buffer* texture, u32 lastSnippetIndex) {
		f32 tm = 0;

		u32 idx = lastSnippetIndex == UINT32_MAX ? 0 : lastSnippetIndex;
		snippet* s = snippets[idx];
		source_content::snippet& info = s->source->snippets[s->snippetIdx];

		if (lastSnippetIndex == UINT32_MAX) {
			// speech just started
			if (audio->buffer() != s->source->audio()) {
				// a different source was previously enabled
				audio->stop();
				audio->buffer(s->source->audio());
				audio->setPlayPosition(info.start);
				audio->play();
				printf("[%s]\n", info.text.c_str());
			} else audio->setPlayPosition(info.start);

			if (texture) s->source->frame(info.start, texture);
		} else {
			f32 ppos = audio->playPosition();
			if (ppos >= info.end) {
				// last snippet finished
				idx++;
				if (idx == snippets.size()) {
					printf("finished speech\n");
					return UINT32_MAX;
				}
				s = snippets[idx];
				source_content::snippet& info = s->source->snippets[s->snippetIdx];

				if (audio->buffer() != s->source->audio()) {
					// a different source was previously enabled
					audio->stop();
					audio->buffer(s->source->audio());
					audio->setPlayPosition(info.start);
					audio->play();
					printf("[%s]\n", info.text.c_str());
				} else {
					printf("[%s]\n", info.text.c_str());
					audio->setPlayPosition(info.start);
				}

				if (texture) s->source->frame(info.start, texture);
			} else if (texture) s->source->frame(ppos, texture);
		}

		return idx;
	}



	speech_execution_context::speech_execution_context(speech_plan* p) : plan(p), completed(false), lastSnippetIndex(UINT32_MAX) { }

	speech_execution_context::~speech_execution_context() { }

	void speech_execution_context::reset() {
		lastSnippetIndex = UINT32_MAX;
		completed = false;
	}

	void speech_execution_context::update(r2::audio_source* audio, r2::texture_buffer* texture) {
		if (completed) return;
		lastSnippetIndex = plan->update(audio, texture, lastSnippetIndex);
		if (lastSnippetIndex == UINT32_MAX) completed = true;
	}



	source_man::source_man() {
		directory_info* info = r2engine::files()->parse_directory("./resources/video");
		
		for (u32 i = 0;i < info->entry_count();i++) {
			directory_entry* entry = info->entry(i);
			if (entry->Type == DET_FOLDER && entry->Name != "." && entry->Name != "..") {
				m_sources.push_back(new source_content(entry->Name));
			}
		}

		r2engine::files()->destroy_directory(info);
	}
	
	source_man::~source_man() {
	}

	source_content* source_man::source(const mstring& name) {
		for (auto it = m_sources.begin();it != m_sources.end();it++) {
			if ((*it)->name() == name) return *it;
		}
		return nullptr;
	}

	speech_plan* source_man::plan_speech(const mstring& text, i32 using_source_idx) {
		speech_plan* plan = new speech_plan;

		mvector<mstring> words;
		mstring cword = "";
		for (u32 x = 0;x < text.length();x++) {
			char c = text[x];
			if (isspace(c)) {
				if (cword.length() > 0) words.push_back(cword);
				cword = "";
				continue;
			}

			if (c == ',') {
				// todo: short pause
				if (cword.length() > 0) words.push_back(cword);
				cword = "";
				continue;
			}

			if (c == '.' || c == '!' || c == '?') {
				// todo: long pause
				if (cword.length() > 0) words.push_back(cword);
				cword = "";
				continue;
			}

			cword += tolower(c);
		}
		if (cword.length() > 0) words.push_back(cword);

		struct possible_selection {
			u32 sourceIdx;
			u32 snippetIdx;
			u32 consumeCount;
		};

		for (u32 w = 0;w < words.size();w++) {
			dynamic_pod_array<possible_selection> applicable;
			bool found_phrase = false;
			if (using_source_idx < 0) {
				for (u32 sc = 0;sc < m_sources.size() && !found_phrase;sc++) {
					source_content* source = m_sources[sc];
					for (u32 s = 0;s < source->snippets.size();s++) {
						source_content::snippet& snip = source->snippets[s];
						if (snip.text == words[w]) applicable.push({ sc, s, 1 });
						else if (snip.text.find(words[w]) != string::npos) {
							mvector<mstring> swords = split(snip.text, " ");
							bool same_phrase = true;
							for (u32 sw = 0;sw < swords.size() && same_phrase;sw++) {
								same_phrase = !(w + sw == words.size() || swords[sw] != words[w + sw]);
							}

							if (same_phrase) {
								plan->add(source, s);
								found_phrase = true;
								w += swords.size() - 1;
								break;
							}
						}
					}
				}
			} else {
				source_content* source = m_sources[using_source_idx];
				for (u32 s = 0;s < source->snippets.size();s++) {
					source_content::snippet& snip = source->snippets[s];
					if (snip.text == words[w]) applicable.push({ (u32)using_source_idx, s, 1 });
					else if (snip.text.find(words[w]) != string::npos) {
						mvector<mstring> swords = split(snip.text, " ");
						bool same_phrase = true;
						for (u32 sw = 0;sw < swords.size() && same_phrase;sw++) {
							same_phrase = !(w + sw == words.size() || swords[sw] != words[w + sw]);
						}

						if (same_phrase) {
							plan->add(source, s);
							found_phrase = true;
							w += swords.size() - 1;
							break;
						}
					}
				}
			}

			if (!found_phrase && applicable.size() > 0) {
				possible_selection* snip = applicable[rand() % applicable.size()];
				plan->add(m_sources[snip->sourceIdx], snip->snippetIdx);
			}
		}

		return plan;
	}
};