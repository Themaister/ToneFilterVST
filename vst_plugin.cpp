#include "vst_plugin.hpp"
#include "dsp.hpp"
#include <algorithm>

ToneFilterPlugin::ToneFilterPlugin(audioMasterCallback audio_master)
	: AudioEffectX(audio_master, 1, NumParameters)
{
	isSynth(false);
	setNumInputs(2);
	setNumOutputs(2);
	setUniqueID(0x7fb92346u);
	canProcessReplacing(true);

	strncpy(program_name, "Default", sizeof(program_name));

	tone = new Granite::Audio::DSP::ToneFilter();
	tone->init(sampleRate);

	//setEditor(new FMEditor(this));
	suspend();
}

ToneFilterPlugin::~ToneFilterPlugin()
{
	delete tone;
}

#if 0
void ToneFilterPlugin::Parameters::reset(fmsynth_t *fm, bool normalized)
{
	dirty_global.store(0);
	dirty_global_operator.store(0);
	for (auto &o : dirty_operator)
		o.store(0);

	if (normalized)
	{
		for (uint32_t i = 0; i < FMSYNTH_GLOBAL_PARAM_END; i++)
			global[i] = fmsynth_convert_to_normalized_global_parameter(fm, i, fmsynth_get_global_parameter(fm, i));

		for (uint32_t o = 0; o < FMSYNTH_OPERATORS; o++)
			for (uint32_t i = 0; i < FMSYNTH_PARAM_END; i++)
				operators[o][i] = fmsynth_convert_to_normalized_parameter(fm, i, fmsynth_get_parameter(fm, i, o));
	}
	else
	{
		for (uint32_t i = 0; i < FMSYNTH_GLOBAL_PARAM_END; i++)
			global[i] = fmsynth_get_global_parameter(fm, i);

		for (uint32_t o = 0; o < FMSYNTH_OPERATORS; o++)
			for (uint32_t i = 0; i < FMSYNTH_PARAM_END; i++)
				operators[o][i] = fmsynth_get_parameter(fm, i, o);
	}
}
#endif

void ToneFilterPlugin::post_update()
{
	//static_cast<FMEditor *>(editor)->post_update();
}

void ToneFilterPlugin::setProgram(VstInt32 program)
{
	post_update();
}

VstInt32 ToneFilterPlugin::getProgram()
{
	return 0;
}

void ToneFilterPlugin::getParameterLabel(VstInt32, char *label)
{
	*label = '\0';
}

void ToneFilterPlugin::getParameterDisplay(VstInt32, char *text)
{
	*text = '\0';
}

void ToneFilterPlugin::getParameterName(VstInt32, char *text)
{
	*text = '\0';
}

bool ToneFilterPlugin::getEffectName(char *name)
{
	strncpy(name, "ToneFilterPlugin", kVstMaxEffectNameLen);
	return true;
}

bool ToneFilterPlugin::getParameterProperties(VstInt32, VstParameterProperties *p)
{
	*p = {};
	p->flags = kVstParameterUsesFloatStep;
	p->stepFloat = 0.1f;
	p->smallStepFloat = 0.01f;
	p->largeStepFloat = 0.2f;
	strncpy(p->label, "", kVstMaxLabelLen);
	strncpy(p->shortLabel, "", kVstMaxShortLabelLen);
	return true;
}

void ToneFilterPlugin::resume()
{
	delete tone;
	tone = new Granite::Audio::DSP::ToneFilter();
	tone->init(sampleRate);

#if 0
	parameters_real.dirty_global_operator.store((1u << FMSYNTH_OPERATORS) - 1u);
	parameters_real.dirty_global.store((1u << FMSYNTH_GLOBAL_PARAM_END) - 1u);
	for (auto &p : parameters_real.dirty_operator)
		p.store((1u << FMSYNTH_PARAM_END) - 1u);
#endif
}

void ToneFilterPlugin::suspend()
{
	//fmsynth_release_all(fm);
}

VstInt32 ToneFilterPlugin::canDo(char *)
{
	return 0;
}

void ToneFilterPlugin::flush_parameters()
{
#if 0
	uint32_t dirty_global = parameters_real.dirty_global.exchange(0);
	while (dirty_global)
	{
		uint32_t bit = ctz(dirty_global);
		fmsynth_set_global_parameter(fm, bit, parameters_real.global[bit].load(std::memory_order_relaxed));
		dirty_global &= ~(1u << bit);
	}

	uint32_t dirty_operators = parameters_real.dirty_global_operator.exchange(0);
	while (dirty_operators)
	{
		uint32_t op = ctz(dirty_operators);
		uint32_t dirty_params = parameters_real.dirty_operator[op].exchange(0);

		while (dirty_params)
		{
			uint32_t param = ctz(dirty_params);
			fmsynth_set_parameter(fm, param, op, parameters_real.operators[op][param].load(std::memory_order_relaxed));
			dirty_params &= ~(1u << param);
		}

		dirty_operators &= ~(1u << op);
	}
#endif
}

void ToneFilterPlugin::processReplacing(float **inputs, float **outputs, VstInt32 sample_frames)
{
	flush_parameters();
	float *outs[2] = { outputs[0], outputs[1] };
	float *ins[2] = { inputs[0], inputs[1] };

	//Granite::Audio::DSP::replace_channel(outs[0], ins[0], 0.0f, sample_frames);
	//Granite::Audio::DSP::replace_channel(outs[1], ins[1], 0.0f, sample_frames);

	while (sample_frames != 0)
	{
		unsigned to_process = unsigned(std::min<VstInt32>(sample_frames, MonoBufferSize));
		Granite::Audio::DSP::convert_to_mono(mono_buffer_input, ins, 2, to_process);
		tone->filter(mono_buffer_output, mono_buffer_input, to_process);
		//Granite::Audio::DSP::accumulate_channel(outs[0], mono_buffer_output, 1.0f, to_process);
		//Granite::Audio::DSP::accumulate_channel(outs[1], mono_buffer_output, 1.0f, to_process);
		Granite::Audio::DSP::replace_channel(outs[0], mono_buffer_output, 1.0f, to_process);
		Granite::Audio::DSP::replace_channel(outs[1], mono_buffer_output, 1.0f, to_process);
		sample_frames -= to_process;
		for (auto &o : outs)
			o += to_process;
		for (auto &i : ins)
			i += to_process;
	}
}

void ToneFilterPlugin::setParameter(VstInt32 index, float value)
{
#if 0
	// Atomically notify the audio thread that there are updates to the parameters.
	if (index < FMSYNTH_GLOBAL_PARAM_END)
	{
		parameters_01.global[index].store(value, std::memory_order_relaxed);
		parameters_real.global[index].store(fmsynth_convert_from_normalized_global_parameter(fm, index, value),
		                                    std::memory_order_relaxed);
		parameters_real.dirty_global.fetch_or(1u << index, std::memory_order_release);
	}
	else
	{
		index -= FMSYNTH_GLOBAL_PARAM_END;
		auto op = index / FMSYNTH_PARAM_END;
		auto param = index % FMSYNTH_PARAM_END;
		if (op < FMSYNTH_OPERATORS)
		{
			parameters_01.operators[op][param].store(value, std::memory_order_relaxed);
			parameters_real.operators[op][param].store(fmsynth_convert_from_normalized_parameter(fm, param, value),
			                                           std::memory_order_relaxed);

			parameters_real.dirty_operator[op].fetch_or(1u << param, std::memory_order_relaxed);
			parameters_real.dirty_global_operator.fetch_or(1u << op, std::memory_order_release);
		}
	}
#endif

	post_update();
}

// Called from UI.
void ToneFilterPlugin::setParameterAutomated(VstInt32 index, float value)
{
	beginEdit(index);
	setParameter(index, value);
	if (audioMaster)
		audioMaster(getAeffect(), audioMasterAutomate, index, 0, nullptr, value);
	endEdit(index);
}

float ToneFilterPlugin::getParameter(VstInt32 index)
{
#if 0
	if (index < NumParameters)
		return parameters.global[index].load(std::memory_order_relaxed);

	index -= FMSYNTH_GLOBAL_PARAM_END;
	auto op = index / FMSYNTH_PARAM_END;
	auto param = index % FMSYNTH_PARAM_END;
	if (op < FMSYNTH_OPERATORS)
		return parameters_01.operators[op][param].load(std::memory_order_relaxed);

	return 0.0f;
#else
	return 0.0f;
#endif
}

void ToneFilterPlugin::setProgramName(char *name)
{
	strcpy(program_name, name);
	post_update();
}

void ToneFilterPlugin::getProgramName(char *name)
{
	strcpy(name, program_name);
}

AudioEffect *createEffectInstance(audioMasterCallback audio_master)
{
	return new ToneFilterPlugin(audio_master);
}