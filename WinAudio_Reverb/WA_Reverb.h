#pragma once


class WA_Reverb
{
private:
	WA_AudioFormat Format{};

	std::vector<std::unique_ptr<RevModel>> models{};

	float drytime;
	float wettime;
	float dampness;
	float roomwidth;
	float roomsize;

	void BytesToFloat(const int8_t* pByte, uint32_t uByteLen, std::vector<float>& Out);
	void FloatToBytes(const std::vector<float>& In, int8_t* pByte, uint32_t uByteLen);
	

public:
	WA_Reverb();
	void UpdateFormat(WA_AudioFormat Format);
	void Process(int8_t* pByte, uint32_t uByteLen);

	void SetDry(float val);
	float GetDry();
	void SetWet(float val);
	float GetWet();
	void SetDampness(float val);
	float GetDampness();
	void SetRoomWidth(float val);
	float GetRoomWidth();
	void SetRoomSize(float val);
	float GetRoomSize();
	
};
