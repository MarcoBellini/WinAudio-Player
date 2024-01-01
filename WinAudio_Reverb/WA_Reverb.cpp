
#include "pch.h"
#include "RevModel.h"
#include "WA_Reverb.h"

static inline float norm(float fValue)
{

	if (fValue > 1.0f)
		return 1.0f;
	else if (fValue < -1.0f)
		return -1.0f;
	else
		return fValue;
}

WA_Reverb::WA_Reverb()
{	
	drytime = 0.43f;
	wettime = 0.57f;
	dampness = 0.45f;
	roomwidth = 0.56f;
	roomsize = 0.56f;

	models.clear();
}

void WA_Reverb::UpdateFormat(WA_AudioFormat Format)
{
	this->Format = Format;

	models.clear();

	for (int32_t i = 0; i < Format.uChannels; i++)	
		models.push_back(std::make_unique<RevModel>());


	for (auto& model : models)
	{
		model->setdry(drytime);
		model->setwet(wettime);
		model->setdamp(dampness);
		model->setwidth(roomwidth);
		model->setroomsize(roomsize);
	}
		
	
}

void WA_Reverb::BytesToFloat(const int8_t* pByte, uint32_t uByteLen, std::vector<float>& Out)
{

	uint32_t uSampleSize, uTotalSamples;

	uSampleSize = Format.uBitsPerSample / 8U;
	uTotalSamples = uByteLen / uSampleSize;


	for (uint32_t uSample = 0U; uSample < uByteLen; uSample += uSampleSize)
	{

		switch (Format.uBitsPerSample)
		{
		case 8:
			Out.push_back(pByte[uSample] / 127.0f);			
			break;
		case 16:
			Out.push_back(((pByte[uSample] & 0xFF) | (pByte[uSample + 1] << 8)) / 32767.0f);
			break;
		case 24:
			Out.push_back(((pByte[uSample] & 0xFF) | ((pByte[uSample + 1] << 8) & 0xFF00) |
				(pByte[uSample + 2] << 16)) / 8388607.0f);
			break;
		case 32:
			Out.push_back(((pByte[uSample] & 0xFF) | ((pByte[uSample + 1] << 8) & 0xFF00) |
				((pByte[uSample + 2] << 16) & 0xFF0000) | (pByte[uSample + 3] << 24)) / 2147483647.0f);
		}		
	}
}


void WA_Reverb::FloatToBytes(const std::vector<float>& In, int8_t* pByte, uint32_t uByteLen)
{

	uint32_t uSampleSize, uSample;
	
	uSampleSize = Format.uBitsPerSample / 8U;
	uSample = 0U;

	for (auto s : In)
	{
		s = norm(s);

		switch (Format.uBitsPerSample)
		{
		case 8:
			pByte[uSample] = (int8_t)(s * 127.0f
				);
			break;
		case 16:
			pByte[uSample] = (int8_t)((int16_t)(s * 32767.0f) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int16_t)(s * 32767.0f) & 0xFF00) >> 8);
			break;
		case 24:
			pByte[uSample] = (int8_t)((int32_t)(s * 8388607.0f) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int32_t)(s * 8388607.0f) & 0xFF00) >> 8);
			pByte[uSample + 2] = (int8_t)(((int32_t)(s * 8388607.0f) & 0xFF0000) >> 16);
			break;
		case 32:
			pByte[uSample] = (int8_t)((int32_t)(s * 2147483647.0f) & 0xFF);
			pByte[uSample + 1] = (int8_t)(((int32_t)(s * 2147483647.0f) & 0xFF00) >> 8);
			pByte[uSample + 2] = (int8_t)(((int32_t)(s * 2147483647.0f) & 0xFF0000) >> 16);
			pByte[uSample + 3] = (int8_t)(((int32_t)(s * 2147483647.0f) & 0xFF000000) >> 24);
			break;
		}

		uSample += uSampleSize;

	}
}

void WA_Reverb::Process(int8_t * pByte, uint32_t uByteLen)
{
	std::vector<float> Samples;

	BytesToFloat(pByte, uByteLen, Samples);


	for (size_t i = 0; i < models.size(); i++)
	{

		for (size_t j = i; j < Samples.size(); j += models.size())
		{

			Samples[j] = models[i]->processsample(Samples[j]);
		}

	}


	FloatToBytes(Samples, pByte, uByteLen);

}

void WA_Reverb::SetDry(float val)
{
	drytime = val;

	for (auto& model : models)
		model->setdry(val);
}

float WA_Reverb::GetDry()
{
	return drytime;
}

void WA_Reverb::SetWet(float val)
{

	wettime = val;

	for (auto& model : models)
		model->setwet(val);

}

float WA_Reverb::GetWet()
{
	return wettime;
}

void WA_Reverb::SetDampness(float val)
{
	dampness = val;

	for (auto& model : models)
		model->setdamp(val);
}

float WA_Reverb::GetDampness()
{
	return dampness;
}

void WA_Reverb::SetRoomWidth(float val)
{
	roomwidth = val;

	for (auto& model : models)
		model->setwidth(val);
}

float WA_Reverb::GetRoomWidth()
{
	return roomwidth;
}

void WA_Reverb::SetRoomSize(float val)
{
	roomsize = val;

	for (auto& model : models)
		model->setroomsize(val);
}

float WA_Reverb::GetRoomSize()
{
	return roomsize;
}

