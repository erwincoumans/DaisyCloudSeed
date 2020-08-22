
#include "Default.h"
#include "ReverbController.h"
#include "FastSin.h"
#include "AudioLib/ValueTables.h"

using namespace CloudSeed;
bool isInitialized = false;

extern "C"
{
	ReverbController* Create(int samplerate)
	{
		if (!isInitialized)
		{
			AudioLib::ValueTables::Init();
			FastSin::Init();
			isInitialized = true;
		}

		return new ReverbController(samplerate);
	}

	void Delete(ReverbController* item)
	{
		delete item;
	}

	int GetSamplerate(ReverbController* item)
	{
		return item->GetSamplerate();
	}

	void SetSamplerate(ReverbController* item, int samplerate)
	{
		return item->SetSamplerate(samplerate);
	}

	int GetParameterCount(ReverbController* item)
	{
		return item->GetParameterCount();
	}

	double* GetAllParameters(ReverbController* item)
	{
		return item->GetAllParameters();
	}

	double GetScaledParameter(ReverbController* item, Parameter param)
	{
		return item->GetScaledParameter(param);
	}

	void SetParameter(ReverbController* item, Parameter param, double value)
	{
		item->SetParameter(param, value);
	}

	void ClearBuffers(ReverbController* item)
	{
		item->ClearBuffers();
	}

	void Process(ReverbController* item, double* input, double* output, int bufferSize)
	{
		item->Process(input, output, bufferSize);
	}
}
