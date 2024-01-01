#pragma once


template <typename T, std::size_t N>
class CombFilter
{	
private:
	std::array<T, N> Buffer{ 0 };
	int index;

	T feedback;
	T filterstore;
	T damp1;
	T damp2;

	T xabs(T n)
	{
		return n < 0 ? -n : n;
	}

public:
	CombFilter()
	{
		filterstore = 0;
		index = 0;
		feedback = 0;
		damp1 = 0;
		damp2 = 0;
	}

	T process(T input)
	{
		auto output{ Buffer[index] };

		//output = Buffer[index];

		if (xabs(output) < 1e-37)
			output = 0;

		filterstore = (output * damp2) + (filterstore * damp1);

		if (xabs(filterstore) < 1e-37)
			filterstore = 0;

		Buffer[index] = input + (filterstore * feedback);

		index++;
		index %= Buffer.size();

		return output;
	}

	void mute()
	{
		for (auto& b : Buffer)
			b = 0;
	}

	void setdamp(T val)
	{
		damp1 = val;
		damp2 = 1 - val;
	}

	T getdamp() 
	{
		return damp1;
	}

	void setfeedback(T val)
	{
		feedback = val;
	}


	T getfeedback()
	{
		return feedback;
	}

};



