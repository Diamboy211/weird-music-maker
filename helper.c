float min(float a, float b)
{
	return (a > b) ? b : a;
}

float w(float i)
{
	return 6.283185f * i;
}

float float_time(int t, int srate)
{
	return (float)t / srate;
}

int int_time(float t, int srate)
{
	return t * srate;
}

float get_freq(int dist)
{
	static const double l = 1.0594630943592953107,
			    rl = 0.9438743126816935151;
	if (dist == 0) return 440.0f;
	if (dist == -128) return 0.0f;
	if (dist > 0)
	{
		double f = 440;
		for (int i = 0; i < dist; i++) f *= l;
		return f;
	}
	else
	{
		double f = 440;
		for (int i = 0; i < -dist; i++) f *= rl;
		return f;
	}
}
