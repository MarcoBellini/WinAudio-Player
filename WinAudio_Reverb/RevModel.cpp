
#include "pch.h"
#include "RevModel.h"


RevModel::RevModel() 
{
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);
	mute();
}

void RevModel::mute() 
{
	if (getmode() >= freezemode)
		return;

	bufcombL1.mute();
	bufcombL2.mute();
	bufcombL3.mute();
	bufcombL4.mute();
	bufcombL5.mute();
	bufcombL6.mute();
	bufcombL7.mute();
	bufcombL8.mute();

	bufallpassL1.mute();
	bufallpassL2.mute();
	bufallpassL3.mute();
	bufallpassL4.mute();
}

float RevModel::processsample(float in)
{
	float samp{ in };
	float mono_out{ 0.0f };
	float mono_in{ samp };
	float input{ mono_in * gain };

	
	mono_out = bufcombL1.process(input);
	mono_out += bufcombL2.process(input);
	mono_out += bufcombL3.process(input);
	mono_out += bufcombL4.process(input);
	mono_out += bufcombL5.process(input);
	mono_out += bufcombL6.process(input);
	mono_out += bufcombL7.process(input);
	mono_out += bufcombL8.process(input);
	
	mono_out = bufallpassL1.process(mono_out);
	mono_out = bufallpassL2.process(mono_out);
	mono_out = bufallpassL3.process(mono_out);
	mono_out = bufallpassL4.process(mono_out);

	samp = mono_in * dry + mono_out * wet1;

	return samp;
}

void RevModel::update() 
{
	
	wet1 = wet * (width / 2.0f + 0.5f);

	if (mode >= freezemode) {
		roomsize1 = 1.0f;
		damp1 = 0.0f;
		gain = muted;
	}
	else {
		roomsize1 = roomsize;
		damp1 = damp;
		gain = fixedgain;
	}

	bufcombL1.setfeedback(roomsize1);
	bufcombL2.setfeedback(roomsize1);
	bufcombL3.setfeedback(roomsize1);
	bufcombL4.setfeedback(roomsize1);
	bufcombL5.setfeedback(roomsize1);
	bufcombL6.setfeedback(roomsize1);
	bufcombL7.setfeedback(roomsize1);
	bufcombL8.setfeedback(roomsize1);

	bufcombL1.setdamp(damp1);
	bufcombL2.setdamp(damp1);
	bufcombL3.setdamp(damp1);
	bufcombL4.setdamp(damp1);
	bufcombL5.setdamp(damp1);
	bufcombL6.setdamp(damp1);
	bufcombL7.setdamp(damp1);
	bufcombL8.setdamp(damp1);
}

void RevModel::setroomsize(float value) 
{
	roomsize = (value * scaleroom) + offsetroom;
	update();
}

float RevModel::getroomsize() 
{
	return (roomsize - offsetroom) / scaleroom;
}

void RevModel::setdamp(float value) 
{
	damp = value * scaledamp;
	update();
}

float RevModel::getdamp() 
{
	return damp / scaledamp;
}

void RevModel::setwet(float value) 
{
	wet = value * scalewet;
	update();
}

float RevModel::getwet() 
{
	return wet / scalewet;
}

void RevModel::setdry(float value) 
{
	dry = value * scaledry;
}

float RevModel::getdry() {
	return dry / scaledry;
}

void RevModel::setwidth(float value) 
{
	width = value;
	update();
}

float RevModel::getwidth()
{
	return width;
}

void RevModel::setmode(float value) 
{
	mode = value;
	update();
}

float RevModel::getmode() 
{
	if (mode >= freezemode)
		return 1.0f;
	else
		return 0.0f;
}