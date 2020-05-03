#include <managers/source_man.h>
#include <utils/audio.h>
#include <utils/video.h>
#include <r2/engine.h>
using namespace r2;

#include <marl/event.h>
#include <marl/waitgroup.h>

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



	void source_content::bone::set(r2::f32 time, r2::vec2f pos) {
		if (pos.x < 0.0f) pos.x = 0.0f;
		else if (pos.x > 1.0f) pos.x = 1.0f;
		if (pos.y < 0.0f) pos.y = 0.0f;
		else if (pos.y > 1.0f) pos.y = 1.0f;
		f32 epsilon = 0.0001f;
		u32 idx = 0;
		for (auto& it = frames.begin();it != frames.end();it++) {
			if (it->time >= time - epsilon && it->time <= time + epsilon) {
				it->time = time;
				it->position = pos;
				it->hidden = hidden(time);
				return;
			}
			if (it->time > time + epsilon) {
				frames.insert(it--, { time, pos, hidden(time) });
				return;
			}
			idx++;
		}
		frames.push_back({ time, pos, hidden(time) });
	}

	void source_content::bone::set(r2::f32 time, bool hidden) {
		f32 epsilon = 0.0001f;
		u32 idx = 0;
		for (auto& it = frames.begin();it != frames.end();it++) {
			if (it->time >= time - epsilon && it->time <= time + epsilon) {
				it->time = time;
				it->position = position(time);
				it->hidden = hidden;
				return;
			}
			if (it->time > time + epsilon) {
				frames.insert(it--, { time, position(time), hidden });
				return;
			}
			idx++;
		}
		frames.push_back({ time, position(time), hidden });
	}

	r2::vec2f source_content::bone::position(r2::f32 time) const {
		for (auto& it = frames.begin();it != frames.end();it++) {
			auto next = std::next(it);
			if (next == frames.end()) return it->position;
			if (it->time <= time && next->time > time) {
				f32 fac = (time - it->time) / (next->time - it->time);
				return it->position + ((next->position - it->position) * fac);
			}
		}

		return vec2f(0, 0);
	}

	bool source_content::bone::hidden(r2::f32 time) const {
		for (auto& it = frames.begin();it != frames.end();it++) {
			auto next = std::next(it);
			if (next == frames.end() || (it->time <= time && next->time > time)) return it->hidden;
		}

		return true;
	}



	source_content::source_content(const mstring& name) {
		m_name = name;

		marl::WaitGroup g(2);

		marl::schedule([this, g]{
			m_video = new video_container(m_name);
			g.done();
		});
		marl::schedule([this, g]{
			m_audio = new audio_container(m_name);
			g.done();
		});

		g.wait();

		if (r2engine::files()->exists("./resources/snip/" + name + ".csv")) {
			data_container* snips = r2engine::files()->open("./resources/snip/" + name + ".csv", DM_TEXT);
			if (snips) {
				mstring line;
				while (!snips->at_end(1) && snips->read_line(line)) {
					if (line.find_first_of(',') == string::npos) {
						line = "";
						continue;
					}
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
					line = "";

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
				}

				r2engine::files()->destroy(snips);
			}
		}

		if (r2engine::files()->exists("./resources/skel/" + name + ".csv")) {
			data_container* skel = r2engine::files()->open("./resources/skel/" + name + ".csv", DM_TEXT);
			if (skel) {
				mstring line;
				while (!skel->at_end(1) && skel->read_line(line)) {
					if (line.find_first_of(',') == string::npos) {
						line = "";
						continue;
					}
					mstring cols[5];
					u8 ccol = 0;
					for (u8 x = 0;x < line.length();x++) {
						if (line[x] == ',') {
							ccol++;
						} else if (line[x] == '\n' || line[x] == '\r') break;
						else cols[ccol] += line[x];
					}
					line = "";

					for (u8 c = 0;c < 5;c++) trim(cols[c]);

					u32 bone = atoi(cols[0].c_str());
					f32 time = atof(cols[1].c_str());
					f32 px = atof(cols[2].c_str());
					f32 py = atof(cols[3].c_str());
					bool hidden = cols[4] == "1";

					bones[bone].frames.push_back({ time, vec2f(px, py), hidden });
				}
				r2engine::files()->destroy(skel);
			}
		}
	}

	source_content::~source_content() {
		delete m_video;
		delete m_audio;
	}

	audio_buffer* source_content::audio() const {
		return m_audio->buffer();
	}

	video_container* source_content::video() const { return m_video; }

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

	void source_content::save_snippets() {
		data_container* csv = r2engine::files()->create(DM_TEXT);

		char linebuf[512] = { 0 };
		for (u32 i = 0;i < snippets.size();i++) {
			source_content::snippet& snip = snippets[i];
			snprintf(linebuf, 512, "%s,%f,%f\n", snip.text.c_str(), snip.start, snip.end);
			csv->write_string(linebuf);
			memset(linebuf, 0, 512);
		}

		r2engine::files()->save(csv, "./resources/snip/" + m_name + ".csv");
		r2engine::files()->destroy(csv);
	}

	void source_content::save_bones() {
		data_container* csv = r2engine::files()->create(DM_TEXT);

		char linebuf[512] = { 0 };
		for (u32 i = 0;i < bi_bone_count;i++) {
			for (auto k = bones[i].frames.begin();k != bones[i].frames.end();k++) {
				snprintf(linebuf, 512, "%d,%f,%f,%f,%d\n", i, k->time, k->position.x, k->position.y, k->hidden ? 1 : 0);
				csv->write_string(linebuf);
				memset(linebuf, 0, 512);
			}
		}

		r2engine::files()->save(csv, "./resources/skel/" + m_name + ".csv");
		r2engine::files()->destroy(csv);
	}



	void speech_plan::add(source_content* source, r2::u32 snippetIdx) {
		snippets.push({ source, snippetIdx });
		duration += (source->snippets[snippetIdx].end - source->snippets[snippetIdx].start);
	}

	void speech_plan::append(speech_plan* plan) {
		for (u32 s = 0;s < plan->snippets.size();s++) {
			add(plan->snippets[s]->source, plan->snippets[s]->snippetIdx);
		}
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
				//printf("[%s]\n", info.text.c_str());
			} else audio->setPlayPosition(info.start);

			if (texture) s->source->frame(info.start, texture);
		} else {
			f32 ppos = audio->playPosition();
			if (ppos >= info.end) {
				// last snippet finished
				idx++;
				if (idx == snippets.size()) {
					//printf("finished speech\n");
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
					//printf("[%s]\n", info.text.c_str());
				} else {
					//printf("[%s]\n", info.text.c_str());
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
		return;

		directory_info* info = r2engine::files()->parse_directory("./resources/video");
		if (info) {
			mvector<mstring> source_names;
			for (u32 i = 0;i < info->entry_count();i++) {
				directory_entry* entry = info->entry(i);
				if (entry->Type == DET_FOLDER && entry->Name != "." && entry->Name != "..") {
					source_names.push_back(entry->Name);
				}
			}
			r2engine::files()->destroy_directory(info);

			marl::Event sourcesLoaded(marl::Event::Mode::Manual);

			size_t total = source_names.size();
			for (u16 i = 0;i < source_names.size();i++) {
				mstring name = source_names[i];
				marl::schedule([this, total, name, sourcesLoaded]{
					source_content* content = new source_content(name);

					m_lock.lock();
					m_sources.push_back(content);
					size_t current = m_sources.size();
					if (current == total) m_loadingProgress = 0.90f;
					else m_loadingProgress = f32(current) / f32(total);
					m_lock.unlock();
					if (current == total) sourcesLoaded.signal();
				});
			}

			marl::schedule([this, sourcesLoaded]{
				sourcesLoaded.wait();

				m_lock.lock();
				if (m_sources.size() == 0) {
					m_lock.unlock();
					return;
				}
				m_lock.unlock();

				if (r2engine::files()->exists("./resources/snip/premix.csv")) {
					data_container* premixes = r2engine::files()->open("./resources/snip/premix.csv", DM_TEXT);
					if (premixes) {
						mstring line;

						mstring premixName = "";
						speech_plan* premixPlan = nullptr;
						while (!premixes->at_end(1) && premixes->read_line(line)) {
							u32 cur = premixes->position();
							u32 sz = premixes->size();
							m_lock.lock();
							m_loadingProgress = 0.95f + ((f32(cur) / f32(sz)) * 0.05f);
							m_lock.unlock();

							if (line.length() == 0) continue;
							if (line.find_first_of(',') == string::npos) {
								if (premixPlan) {
									if (premixPlan->snippets.size() > 0) {
										m_lock.lock();
										mixedWords.push_back({
											premixPlan,
											premixName
										});
										m_lock.unlock();
									} else {
										r2Error("Premix \"%s\" specified with no snippet table", premixName.c_str());
									}
								}
								premixPlan = new speech_plan();
								premixName = line;
								line = "";
								continue;
							} else {
								mstring cols[2];
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
								source_content* src = source(cols[0]);
								if (!src) {
									r2Error("Premix \"%s\" references nonexistent source \"%s\"", premixName.c_str(), cols[0].c_str());
									continue;
								}

								u32 idx = atoi(cols[1].c_str());
								if (idx > src->snippets.size()) {
									r2Error("Premix \"%s\" snippet index %d out of range for source \"%s\"", premixName.c_str(), idx, cols[0]);
									continue;
								}

								if (!premixPlan) {
									r2Error("No premix name specified for snippet table");
									continue;
								}

								premixPlan->add(source(cols[0]), idx);
							}

							line = "";
						}

						if (premixPlan && premixPlan->snippets.size() > 0 && premixName.length() > 0) {
							m_lock.lock();
							mixedWords.push_back({
								premixPlan,
								premixName
							});
							m_lock.unlock();
						} else if (premixPlan) {
							r2Error("Premix \"%s\" specified with no snippet table", premixName.c_str());
							delete premixPlan;
						}

						r2engine::files()->destroy(premixes);
					}
				}

				m_lock.lock();
				m_loadingProgress = 1.0f;
				m_lock.unlock();
			});
		}
	}
	
	source_man::~source_man() {
	}

	source_content* source_man::source(const mstring& name) {
		mstring search = name;
		transform(search.begin(), search.end(), search.begin(), ::tolower);

		for (auto it = m_sources.begin();it != m_sources.end();it++) {
			mstring sname = (*it)->name();
			transform(sname.begin(), sname.end(), sname.begin(), ::tolower);
			if (search == sname) return *it;
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
			u32 premixIdx;
		};

		for (u32 w = 0;w < words.size();w++) {
			dynamic_pod_array<possible_selection> applicable;
			bool found_phrase = false;
			if (using_source_idx < 0) {
				for (u32 sc = 0;sc < m_sources.size() && !found_phrase;sc++) {
					source_content* source = m_sources[sc];
					for (u32 s = 0;s < source->snippets.size();s++) {
						source_content::snippet& snip = source->snippets[s];
						if (snip.text == words[w]) applicable.push({ sc, s, 1, 0 });
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

				if (!found_phrase) {
					for (u32 p = 0;p < mixedWords.size();p++) {
						premixed_word& word = mixedWords[p];
						if (word.text == words[w]) applicable.push({ 0, 0, 1, p + 1 });
						else if (word.text.find(words[w]) != string::npos) {
							mvector<mstring> swords = split(word.text, " ");
							bool same_phrase = true;
							for (u32 sw = 0;sw < swords.size() && same_phrase;sw++) {
								same_phrase = !(w + sw == words.size() || swords[sw] != words[w + sw]);
							}

							if (same_phrase) {
								plan->append(word.plan);
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
					if (snip.text == words[w]) applicable.push({ (u32)using_source_idx, s, 1, 0 });
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
				if (snip->premixIdx) plan->append(mixedWords[snip->premixIdx - 1].plan);
				else plan->add(m_sources[snip->sourceIdx], snip->snippetIdx);
			}
		}

		return plan;
	}

	void source_man::save_premixes() {
		data_container* csv = r2engine::files()->create(DM_TEXT);

		char linebuf[512] = { 0 };
		for (u32 i = 0;i < mixedWords.size();i++) {
			premixed_word& premix = mixedWords[i];
			csv->write_string(premix.text + "\n");

			for (u32 s = 0;s < premix.plan->snippets.size();s++) {
				snprintf(linebuf, 512, "%s,%d\n", premix.plan->snippets[s]->source->name().c_str(), premix.plan->snippets[s]->snippetIdx);
				csv->write_string(linebuf);
				memset(linebuf, 0, 512);
			}

			csv->write_string("\n");
		}

		r2engine::files()->save(csv, "./resources/snip/premix.csv");
		r2engine::files()->destroy(csv);
	}

	f32 source_man::loadingProgress() {
		m_lock.lock();
		f32 r = m_loadingProgress;
		m_lock.unlock();
		return r;
	}
};