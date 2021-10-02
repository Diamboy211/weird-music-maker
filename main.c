#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "helper.h"

typedef struct
{
	float attack_time;
	float decay_time;
	float release_time;

	float sustain_amp;
	float attack_amp;

	float press_time;
	float unpress_time;
} env_adsr;

void create_default_adsr(env_adsr *o, float press_time, float unpress_time)
{
	if (unpress_time < 0.02f)
	{
	o->attack_time = 0.001f;
	o->decay_time = 0.001f;
	o->release_time = 0.001f;

	o->attack_amp = 0.0f;
	o->sustain_amp = 0.0f;

	o->press_time = press_time;
	o->unpress_time = unpress_time;
	
	}
	o->attack_time = 0.01f;
	o->decay_time = 0.01f;
	o->release_time = 0.02f;

	o->attack_amp = 1.0f;
	o->sustain_amp = 0.8f;

	o->press_time = press_time;
	o->unpress_time = unpress_time;
}

float lerp(float t0, float t1, float t)
{
	return (1-t) * t0 + t * t1;
}

float adsr_amp(env_adsr *env, float t)
{
	if (!env) return 1.f;
	float amp = 0.f;
	if (t > env->unpress_time)
	{
		float nt = (t - env->unpress_time) / env->release_time;
		if (nt > 1) return 0.f;
		return env->sustain_amp * (1 - nt);
	}
	if (t < env->press_time) return 0.f;

	if (t < env->attack_time)
	{
		float nt = t / env->attack_time;
		return nt * env->attack_amp;
	}
	if (t < env->attack_time+env->decay_time)
	{
		float nt = (t - env->attack_time) / env->decay_time;
		return lerp(env->attack_amp, env->sustain_amp, nt);
	}
	amp = env->sustain_amp;
	return amp;
}

float cubic_sin(float t)
{
	t *= 0.159155f;
	t -= (int)t;
	return 10.3923f*t*(2.f*t*t - 3.f*t + 1.f);
}

float sqr_osc(float t)
{
	t *= 0.159155f;
	t -= (int)t;
	return (t > 0.5) ? -1 : 1;
}

short sin_osc(short amp, int freq, int t, int srate, env_adsr *env)
{
	float sec = float_time(t, srate);
	if (freq == 0) return 0;
	return amp*adsr_amp(env, float_time(t, srate))
		*cubic_sin(w(sec) * (float)freq
		+ 0.001f * (float)freq * sinf(w(5*sec)));
}

int main(int argc, char **argv)
{
	// argument handling
	char *out_name = "file.wav";
	char *in_name = "file.dbs";
	int srate = 8192; // sample rate
	if (argc >= 2)
	{
		in_name = argv[1];
		if (argc >= 3)
		{
			out_name = argv[2];
			if (argc >= 4) srate = atoi(argv[3]);
		}
	}
	// load dbs file and handling
	FILE *in_f = fopen(in_name, "rb");
	char *dbs_header = malloc(8);
	char *dbs_body = NULL;
	fread(dbs_header, sizeof(char), 8, in_f);
	
	// get core variables
	uint32_t dbs_size = *(uint32_t *)dbs_header;
	uint16_t dbs_bpm = *(uint16_t *)(dbs_header + 4);
	uint8_t dbs_instr = *(uint8_t *)(dbs_header + 6);
	uint32_t dbs_ticks = dbs_size / dbs_instr;	
	float dbs_time = (float)dbs_ticks * 60.f / dbs_bpm;
	float dbs_notedur = 60.f / dbs_bpm;
	// get notes
	dbs_body = malloc(dbs_size);
	fread(dbs_body, sizeof(char), dbs_size, in_f);
	// this shitty code just to work out how long is the longest note
	uint32_t *note_len = malloc(sizeof(uint32_t) * dbs_size);
	{
		uint32_t *tmp;
		int l = 256;
		for (int i = 0; i < dbs_instr; i++)
		{
			for (int j = 0; j < dbs_ticks; j++)
			{
				if (l != (int)dbs_body[j*dbs_instr+i])
				{
					tmp = note_len + j*dbs_instr+i;
					*tmp = 1;
					l = dbs_body[j*dbs_instr+i];
				}
				else
				{
					(*tmp)++;
					note_len[j*dbs_instr+i] = 0;
				}
			}
		}
	}
	uint32_t max_len = 0;
	{
		uint32_t *_max_len = malloc(dbs_instr);
		uint16_t *_last = malloc(dbs_instr);
		for (int i = 0; i < dbs_instr; i++)
		{
			int temp = 0;
			_max_len[i] = 0;
			for (int j = 0; j < dbs_ticks; j++)
			{
				if (dbs_body[j*dbs_instr+i] == _last[i]) temp++;
				else
				{
					_last[i] = dbs_body[j*dbs_instr+i];
					temp = 1;
				}
				if (_max_len[i] < temp) _max_len[i] = temp;
			}
		}
		for (int i = 0; i < dbs_instr; i++)
			if (max_len < _max_len[i]) max_len = _max_len[i];

		free(_max_len);
		free(_last);
	}

	// ahhhh the good old wav file header
	const uint32_t ssize = int_time(dbs_time, srate) * 2;
	unsigned char *buf = malloc(44+ssize*2);
	char *marker = "RIFF";
	uint32_t size = 44 + ssize*2 - 8; // file size - 8
	char *header = "WAVE";
	char *fmt = "fmt "; // format chunk header
	uint32_t fdatasize = 16; // size of above
	uint16_t ftype = 1, nchannels = 1; // format type, num of channels
	uint16_t bps = 16; // bits per sample
	uint32_t sbc8 = srate * bps * nchannels / 8;
	uint16_t bc8 = bps * nchannels / 8;
	char *dch = "data"; // data chunk header
	uint32_t dsize = ssize*2; // size of data

	memcpy(buf, marker, 4);
	memcpy(buf+4, &size, 4);
	memcpy(buf+8, header, 4);
	memcpy(buf+12, fmt, 4);
	memcpy(buf+16, &fdatasize, 4);
	memcpy(buf+20, &ftype, 2);
	memcpy(buf+22, &nchannels, 2);
	memcpy(buf+24, &srate, 4);
	memcpy(buf+28, &sbc8, 4);
	memcpy(buf+32, &bc8, 2);
	memcpy(buf+34, &bps, 2);
	memcpy(buf+36, dch, 4);
	memcpy(buf+40, &dsize, 4);

	short *sbuf = (short *)(buf+44);
	
	env_adsr *envs = malloc(sizeof(env_adsr) * max_len);
	for (int i = 0; i <= max_len; i++)
	{
		create_default_adsr(envs+i, 0.f, dbs_notedur * i);
	}
	for (int i = 0; i < dbs_instr; i++)
	for (int j = 0; j < dbs_ticks; j++)
	{
		for (int k = 0;
		k < int_time(dbs_notedur * note_len[j*dbs_instr+i], srate);
		k++)
		{
			sbuf[k+int_time(dbs_notedur * j, srate)]
				+= sin_osc(5000, get_freq(dbs_body[j*dbs_instr+i]),
					k, srate, NULL) +
					sin_osc(3000, 2*get_freq(dbs_body[j*dbs_instr+i]),
					k, srate, NULL);
		}
	}
	FILE *f = fopen(out_name, "wb");
	fwrite(buf, 44+ssize, 1, f);
	fclose(f);
	printf("free(buf)\n");
	free(buf);
	printf("free(dbs_header)\n");
	free(dbs_header);
	printf("free(note_len)\n");
	free(note_len);
	printf("free(dbs_body)\n");
	free(dbs_body);
	return 0;
}
