#pragma once

#include "audioeffectx.h"
#include "tone_filter.hpp"
#include "aligned_alloc.hpp"
#include <atomic>

class ToneFilterPlugin : public AudioEffectX, public Util::AlignedAllocation<ToneFilterPlugin>
{
public:
	explicit ToneFilterPlugin(audioMasterCallback audio_master);
	~ToneFilterPlugin() override;

	void setParameter(VstInt32 index, float value) override;
	float getParameter(VstInt32 index) override;
	void processReplacing(float **inputs, float **outputs, VstInt32 sample_frames) override;
	void setProgramName(char *name) override;
	void getProgramName(char *name) override;
	bool getParameterProperties(VstInt32 index, VstParameterProperties *p) override;
	bool getEffectName(char *name) override;

	VstInt32 getProgram() override;
	void setProgram(VstInt32 program) override;
	void getParameterLabel(VstInt32 index, char* label) override;
	void getParameterDisplay(VstInt32 index, char* text) override;
	void getParameterName(VstInt32 index, char* text) override;

	void setParameterAutomated(VstInt32 index, float value) override;
	VstInt32 getNumMidiInputChannels() override { return 1; }
	void resume() override;
	void suspend() override;
	VstInt32 canDo(char *text) override;

	void set_automated_global_parameter(VstInt32 index, float value)
	{
		setParameterAutomated(index, value);
	}

private:
	enum { NumParameters = 0, MonoBufferSize = 1024 };
	char program_name[kVstMaxProgNameLen];

	Granite::Audio::DSP::ToneFilter *tone = nullptr;
	alignas(16) float mono_buffer_input[1024];
	alignas(16) float mono_buffer_output[1024];
	void post_update();
	void flush_parameters();
};
