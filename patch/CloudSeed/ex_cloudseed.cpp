#include "daisysp.h"
#include "daisy_patch.h"
#include <string>
#include "../../CloudSeed/Default.h"
#include "../../CloudSeed/ReverbController.h"
#include "../../CloudSeed/FastSin.h"
#include "../../CloudSeed/AudioLib/ValueTables.h"
#include "../../CloudSeed/AudioLib/MathDefs.h"

using namespace daisy;
using namespace daisysp;
static DaisyPatch patch;
::daisy::Parameter lpParam;
static float drylevel, send;

CloudSeed::ReverbController* reverb = 0;



#define CUSTOM_POOL_SIZE (52*1024*1024)

DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];

size_t pool_index = 0;
int allocation_count = 0;

void* custom_pool_allocate(size_t size)
{
        if (pool_index + size >= CUSTOM_POOL_SIZE)
        {
                return 0;
        }
        void* ptr = &custom_pool[pool_index];
        pool_index += size;
        return ptr;
}


static void VerbCallback(float **in, float **out, size_t size)
{
    float dryL, dryR, wetL, wetR, sendL, sendR;
    patch.UpdateAnalogControls();
    for (size_t i = 0; i < size; i++)
    {
        // read some controls
        drylevel = patch.GetCtrlValue(patch.CTRL_1);
        send     = patch.GetCtrlValue(patch.CTRL_2);
        //reverb->SetParameter(::Parameter::LineDelay, drylevel);
        // Read Inputs (only stereo in are used)
        dryL = in[0][i];
        dryR = in[1][i];

        // Send Signal to Reverb
        sendL = dryL * send;
        sendR = dryR * send;
        //verb.Process(sendL, sendR, &wetL, &wetR);
        float ins[2]={sendL,sendR};
	float outs[2]={sendL,sendR};
        reverb->Process( ins, outs, 1);
        wetL=outs[0];
        wetR=outs[1];	


        // Out 1 and 2 are Mixed 
        out[0][i] = outs[0];//(dryL * drylevel) + wetL;
        out[1][i] = outs[1];//(dryR * drylevel) + wetR;

        // Out 3 and 4 are just wet
        out[2][i] = outs[0];//wetL;
        out[3][i] = outs[1];//wetR;
    }
}

void UpdateOled();

int main(void)
{
    float samplerate;
    patch.Init();
    samplerate = patch.AudioSampleRate();

    AudioLib::ValueTables::Init();
    CloudSeed::FastSin::Init();
    reverb = new CloudSeed::ReverbController(samplerate);
    reverb->ClearBuffers();

    lpParam.Init(patch.controls[3], 20, 20000, ::daisy::Parameter::LOGARITHMIC);

    //briefly display the module name
    std::string str = "CloudSeed Reverb";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    patch.display.Update();
    patch.DelayMs(1000);

    patch.StartAdc();
    patch.StartAudio(VerbCallback);

    while(1) 
    {
        UpdateOled();
    }

}

void UpdateOled()
{
    patch.DisplayControls(false);
}
