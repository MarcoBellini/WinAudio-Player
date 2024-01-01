#pragma once

template <typename T, std::size_t N>
class AllPass 
{
private:
	std::array<T, N> Buffer{ 0 };
	int index;
	T feedback;

	T xabs(T n)
	{
		return n < 0 ? -n : n;
	}
public:
	AllPass(T fbk)
	{	
		feedback = fbk;
		index = 0;
	}
	
	void mute()
	{
		for (auto& e : Buffer)
			e = 0;
	}

	void setfeedback(T fbk) { feedback = fbk; }
	T getfeedback() { return feedback; }
	T process(T input)
	{
		T out, bufout;

		bufout = Buffer[index];

		if (xabs(bufout) < 1e-37)
			bufout = 0;

		out = -input + bufout;
		Buffer[index] = input + (bufout * feedback);

		index++;
		index %= Buffer.size();

		return out;
	}
	
};