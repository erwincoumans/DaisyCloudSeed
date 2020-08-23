//see https://github.com/ValdemarOrn/CloudSeed
#ifndef REVERBCONTROLLER
#define REVERBCONTROLLER

#include <vector>
#include "Default.h"
#include "Parameter.h"
#include "ReverbChannel.h"

#include "ReverbController.h"
#include "AudioLib/ValueTables.h"
#include "AllpassDiffuser.h"
#include "MultitapDiffuser.h"
#include "Utils.h"

namespace CloudSeed
{
	class ReverbController
	{
	private:
		static const int bufferSize = 16; // just make it huge by default...
		int samplerate;

		ReverbChannel channelL;
		ReverbChannel channelR;
		double leftChannelIn[bufferSize];
		double rightChannelIn[bufferSize];
		double leftLineBuffer[bufferSize];
		double rightLineBuffer[bufferSize];
		double parameters[(int)Parameter::Count];

	public:
		ReverbController(int samplerate)
			: channelL(bufferSize, samplerate, ChannelLR::Left)
			, channelR(bufferSize, samplerate, ChannelLR::Right)
		{
			this->samplerate = samplerate;
			initFactoryChorus();
			
		}

		void initFactoryChorus()
		{
			//parameters from Chorus Delay in
			//https://github.com/ValdemarOrn/CloudSeed/tree/master/Factory%20Programs
			parameters[(int)Parameter::InputMix] = 0.0;
			parameters[(int)Parameter::PreDelay] = 0.070000000298023224;
			parameters[(int)Parameter::HighPass] = 0.0;
			parameters[(int)Parameter::LowPass] = 0.29000008106231689;
			parameters[(int)Parameter::TapCount]= 0.36499997973442078;
			parameters[(int)Parameter::TapLength]= 1.0;
			parameters[(int)Parameter::TapGain] = 1.0;
			parameters[(int)Parameter::TapDecay] = 0.86500012874603271;
			parameters[(int)Parameter::DiffusionEnabled] = 1.0;
			parameters[(int)Parameter::DiffusionStages] = 0.4285714328289032;
			parameters[(int)Parameter::DiffusionDelay] = 0.43500006198883057;
			parameters[(int)Parameter::DiffusionFeedback] = 0.725000262260437;
			parameters[(int)Parameter::LineCount] = 1.0;
			parameters[(int)Parameter::LineDelay] = 0.68499988317489624;
			parameters[(int)Parameter::LineDecay] = 0.68000012636184692;
			parameters[(int)Parameter::LateDiffusionEnabled] = 1.0;
			parameters[(int)Parameter::LateDiffusionStages] = 0.28571429848670959;
			parameters[(int)Parameter::LateDiffusionDelay] = 0.54499995708465576;
			parameters[(int)Parameter::LateDiffusionFeedback] = 0.65999996662139893;
			parameters[(int)Parameter::PostLowShelfGain] = 0.5199999213218689;
			parameters[(int)Parameter::PostLowShelfFrequency] = 0.31499990820884705;
			parameters[(int)Parameter::PostHighShelfGain] = 0.83500003814697266;
			parameters[(int)Parameter::PostHighShelfFrequency] = 0.73000013828277588;
			parameters[(int)Parameter::PostCutoffFrequency] = 0.73499983549118042;
			parameters[(int)Parameter::EarlyDiffusionModAmount] = 0.50000005960464478;
			parameters[(int)Parameter::EarlyDiffusionModRate] = 0.42500010132789612;
			parameters[(int)Parameter::LineModAmount] = 0.59000003337860107;
			parameters[(int)Parameter::LineModRate] = 0.46999993920326233;
			parameters[(int)Parameter::LateDiffusionModAmount] = 0.619999885559082;
			parameters[(int)Parameter::LateDiffusionModRate] = 0.42500019073486328;
			parameters[(int)Parameter::TapSeed] = 0.0011500000255182385;
			parameters[(int)Parameter::DiffusionSeed] = 0.00018899999849963933;
			parameters[(int)Parameter::DelaySeed] = 0.00033700000494718552;
			parameters[(int)Parameter::PostDiffusionSeed] = 0.00050099997315555811;
			parameters[(int)Parameter::CrossSeed] = 0.0;
			parameters[(int)Parameter::DryOut] = 0.94499987363815308;
			parameters[(int)Parameter::PredelayOut] = 0.0;
			parameters[(int)Parameter::EarlyOut] = 0.77999997138977051;
			parameters[(int)Parameter::MainOut] = 0.74500006437301636;
			parameters[(int)Parameter::HiPassEnabled] = 0.0;
			parameters[(int)Parameter::LowPassEnabled] = 0.0;
			parameters[(int)Parameter::LowShelfEnabled] = 0.0;
			parameters[(int)Parameter::HighShelfEnabled] = 0.0;
			parameters[(int)Parameter::CutoffEnabled] = 1.0;
			parameters[(int)Parameter::LateStageTap] = 1.0;
			parameters[(int)Parameter::Interpolation] = 0.0;

			for (auto value = 0; value < (int)Parameter::Count; value++)
			{
				SetParameter((Parameter)value, parameters[value]);
			}

		}



		int GetSamplerate()
		{
			return samplerate;
		}

		void SetSamplerate(int samplerate)
		{
			this->samplerate = samplerate;

			channelL.SetSamplerate(samplerate);
			channelR.SetSamplerate(samplerate);
		}

		int GetParameterCount()
		{
			return (int)Parameter::Count;
		}

		double* GetAllParameters()
		{
			return parameters;
		}

		double GetScaledParameter(Parameter param)
		{
			switch (param)
			{
				// Input
			case Parameter::InputMix:                  return P(Parameter::InputMix);
			case Parameter::PreDelay:                  return (int)(P(Parameter::PreDelay) * 1000);

			case Parameter::HighPass:                  return 20 + ValueTables::Get(P(Parameter::HighPass), ValueTables::Response4Oct) * 980;
			case Parameter::LowPass:                   return 400 + ValueTables::Get(P(Parameter::LowPass), ValueTables::Response4Oct) * 19600;

				// Early
			case Parameter::TapCount:                  return 1 + (int)(P(Parameter::TapCount) * (MultitapDiffuser::MaxTaps - 1));
			case Parameter::TapLength:                 return (int)(P(Parameter::TapLength) * 500);
			case Parameter::TapGain:                   return ValueTables::Get(P(Parameter::TapGain), ValueTables::Response2Dec);
			case Parameter::TapDecay:                  return P(Parameter::TapDecay);

			case Parameter::DiffusionEnabled:          return P(Parameter::DiffusionEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::DiffusionStages:           return 1 + (int)(P(Parameter::DiffusionStages) * (AllpassDiffuser::MaxStageCount - 0.001));
			case Parameter::DiffusionDelay:            return (int)(10 + P(Parameter::DiffusionDelay) * 90);
			case Parameter::DiffusionFeedback:         return P(Parameter::DiffusionFeedback);

				// Late
			case Parameter::LineCount:                 return 1 + (int)(P(Parameter::LineCount) * 11.999);
			case Parameter::LineDelay:                 return (int)(20.0 + ValueTables::Get(P(Parameter::LineDelay), ValueTables::Response2Dec) * 980);
			case Parameter::LineDecay:                 return 0.05 + ValueTables::Get(P(Parameter::LineDecay), ValueTables::Response3Dec) * 59.95;

			case Parameter::LateDiffusionEnabled:      return P(Parameter::LateDiffusionEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::LateDiffusionStages:       return 1 + (int)(P(Parameter::LateDiffusionStages) * (AllpassDiffuser::MaxStageCount - 0.001));
			case Parameter::LateDiffusionDelay:        return (int)(10 + P(Parameter::LateDiffusionDelay) * 90);
			case Parameter::LateDiffusionFeedback:     return P(Parameter::LateDiffusionFeedback);

				// Frequency Response
			case Parameter::PostLowShelfGain:          return ValueTables::Get(P(Parameter::PostLowShelfGain), ValueTables::Response2Dec);
			case Parameter::PostLowShelfFrequency:     return 20 + ValueTables::Get(P(Parameter::PostLowShelfFrequency), ValueTables::Response4Oct) * 980;
			case Parameter::PostHighShelfGain:         return ValueTables::Get(P(Parameter::PostHighShelfGain), ValueTables::Response2Dec);
			case Parameter::PostHighShelfFrequency:    return 400 + ValueTables::Get(P(Parameter::PostHighShelfFrequency), ValueTables::Response4Oct) * 19600;
			case Parameter::PostCutoffFrequency:       return 400 + ValueTables::Get(P(Parameter::PostCutoffFrequency), ValueTables::Response4Oct) * 19600;

				// Modulation
			case Parameter::EarlyDiffusionModAmount:   return P(Parameter::EarlyDiffusionModAmount) * 2.5;
			case Parameter::EarlyDiffusionModRate:     return ValueTables::Get(P(Parameter::EarlyDiffusionModRate), ValueTables::Response2Dec) * 5;
			case Parameter::LineModAmount:             return P(Parameter::LineModAmount) * 2.5;
			case Parameter::LineModRate:               return ValueTables::Get(P(Parameter::LineModRate), ValueTables::Response2Dec) * 5;
			case Parameter::LateDiffusionModAmount:    return P(Parameter::LateDiffusionModAmount) * 2.5;
			case Parameter::LateDiffusionModRate:      return ValueTables::Get(P(Parameter::LateDiffusionModRate), ValueTables::Response2Dec) * 5;

				// Seeds
			case Parameter::TapSeed:                   return (int)std::floor(P(Parameter::TapSeed) * 1000000 + 0.001);
			case Parameter::DiffusionSeed:             return (int)std::floor(P(Parameter::DiffusionSeed) * 1000000 + 0.001);
			case Parameter::DelaySeed:                 return (int)std::floor(P(Parameter::DelaySeed) * 1000000 + 0.001);
			case Parameter::PostDiffusionSeed:         return (int)std::floor(P(Parameter::PostDiffusionSeed) * 1000000 + 0.001);

				// Output
			case Parameter::CrossSeed:                 return P(Parameter::CrossSeed);

			case Parameter::DryOut:                    return ValueTables::Get(P(Parameter::DryOut), ValueTables::Response2Dec);
			case Parameter::PredelayOut:               return ValueTables::Get(P(Parameter::PredelayOut), ValueTables::Response2Dec);
			case Parameter::EarlyOut:                  return ValueTables::Get(P(Parameter::EarlyOut), ValueTables::Response2Dec);
			case Parameter::MainOut:                   return ValueTables::Get(P(Parameter::MainOut), ValueTables::Response2Dec);

				// Switches
			case Parameter::HiPassEnabled:             return P(Parameter::HiPassEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::LowPassEnabled:            return P(Parameter::LowPassEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::LowShelfEnabled:           return P(Parameter::LowShelfEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::HighShelfEnabled:          return P(Parameter::HighShelfEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::CutoffEnabled:             return P(Parameter::CutoffEnabled) < 0.5 ? 0.0 : 1.0;
			case Parameter::LateStageTap:			   return P(Parameter::LateStageTap) < 0.5 ? 0.0 : 1.0;

				// Effects
			case Parameter::Interpolation:			   return P(Parameter::Interpolation) < 0.5 ? 0.0 : 1.0;

			default: return 0.0;
			}

			return 0.0;
		}

		void SetParameter(Parameter param, double value)
		{
			parameters[(int)param] = value;
			auto scaled = GetScaledParameter(param);
			
			channelL.SetParameter(param, scaled);
			channelR.SetParameter(param, scaled);
		}

		

		void ClearBuffers()
		{
			channelL.ClearBuffers();
			channelR.ClearBuffers();
		}

		void Process(double* input, double* output, int bufferSize)
		{
			auto len = bufferSize;
			auto cm = GetScaledParameter(Parameter::InputMix) * 0.5;
			auto cmi = (1 - cm);

			for (int i = 0; i < len; i++)
			{
				leftChannelIn[i] = input[i*2] * cmi + input[i*2+1] * cm;
				rightChannelIn[i] = input[i*2+1] * cmi + input[i*2] * cm;
			}

			channelL.Process(leftChannelIn, len);
			channelR.Process(rightChannelIn, len);
			auto leftOut = channelL.GetOutput();
			auto rightOut = channelR.GetOutput();

			for (int i = 0; i < len; i++)
			{
				output[i*2] = leftOut[i];
				output[i*2+1] = rightOut[i];
			}
		}
		
	private:
		double P(Parameter para)
		{
			auto idx = (int)para;
			return idx >= 0 && idx < (int)Parameter::Count ? parameters[idx] : 0.0;
		}
	};
}
#endif
