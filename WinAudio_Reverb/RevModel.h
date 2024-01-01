#pragma once

#include "CombFilter.h"
#include "AllPass.h"

constexpr float	muted = 0;
constexpr float	fixedgain = 0.015f;
constexpr float	scalewet = 3;
constexpr float	scaledry = 2;
constexpr float	scaledamp = 0.4f;
constexpr float	scaleroom = 0.28f;
constexpr float	offsetroom = 0.7f;
constexpr float	initialroom = 0.5f;
constexpr float	initialdamp = 0.5f;
constexpr float	initialwet = 1 / scalewet;
constexpr float	initialdry = 0;
constexpr float	initialwidth = 1;
constexpr float	initialmode = 0;
constexpr float	freezemode = 0.5f;

constexpr int combtuningL1 = 1116;
constexpr int combtuningL2 = 1188;
constexpr int combtuningL3 = 1277;
constexpr int combtuningL4 = 1356;
constexpr int combtuningL5 = 1422;
constexpr int combtuningL6 = 1491;
constexpr int combtuningL7 = 1557;
constexpr int combtuningL8 = 1617;
constexpr int allpasstuningL1 = 556;
constexpr int allpasstuningL2 = 441;
constexpr int allpasstuningL3 = 341;
constexpr int allpasstuningL4 = 225;


class RevModel {
public:
	RevModel();
	void mute();
	float processsample(float in);
	void setroomsize(float value);
	float getroomsize();
	void setdamp(float value);
	float getdamp();
	void setwet(float value);
	float getwet();
	void setdry(float value);
	float getdry();
	void setwidth(float value);
	float getwidth();
	void setmode(float value);
	float getmode();
private:
	void update();

	float gain;
	float roomsize, roomsize1;
	float damp, damp1;
	float wet, wet1, wet2;
	float dry;
	float width;
	float mode;


	CombFilter<float, combtuningL1> bufcombL1;
	CombFilter<float, combtuningL2> bufcombL2;
	CombFilter<float, combtuningL3> bufcombL3;
	CombFilter<float, combtuningL4> bufcombL4;
	CombFilter<float, combtuningL5> bufcombL5;
	CombFilter<float, combtuningL6> bufcombL6;
	CombFilter<float, combtuningL7> bufcombL7;
	CombFilter<float, combtuningL8> bufcombL8;
	
	// Set feedback at 0.5
	AllPass<float, allpasstuningL1> bufallpassL1 { 0.5f };
	AllPass<float, allpasstuningL2> bufallpassL2 { 0.5f };
	AllPass<float, allpasstuningL3> bufallpassL3 { 0.5f };
	AllPass<float, allpasstuningL4> bufallpassL4 { 0.5f };

};
