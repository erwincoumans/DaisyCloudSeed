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
static ReverbSc verb;
static DcBlock blk[2];
::daisy::Parameter lpParam;
static float drylevel, send;

CloudSeed::ReverbController* reverb = 0;


DSY_SDRAM_BSS double delayBuffer[44100];
DSY_SDRAM_BSS double outputBuffer[44100];


static void VerbCallback(float **in, float **out, size_t size)
{
    float dryL, dryR, wetL, wetR, sendL, sendR;
    patch.UpdateAnalogControls();
    for (size_t i = 0; i < size; i++)
    {
        // read some controls
        drylevel = patch.GetCtrlValue(patch.CTRL_1);
        send     = patch.GetCtrlValue(patch.CTRL_2);
        verb.SetFeedback(patch.GetCtrlValue(patch.CTRL_3) * .94f);
        verb.SetLpFreq(lpParam.Process());

        // Read Inputs (only stereo in are used)
        dryL = in[0][i];
        dryR = in[1][i];

        // Send Signal to Reverb
        sendL = dryL * send;
        sendR = dryR * send;
        verb.Process(sendL, sendR, &wetL, &wetR);

        // Dc Block
        wetL = blk[0].Process(wetL);
        wetR = blk[1].Process(wetR);

        // Out 1 and 2 are Mixed 
        out[0][i] = (dryL * drylevel) + wetL;
        out[1][i] = (dryR * drylevel) + wetR;

        // Out 3 and 4 are just wet
        out[2][i] = wetL;
        out[3][i] = wetR;
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
    //reverb = new CloudSeed::ReverbController(samplerate);
    CloudSeed::ReverbChannel*  channelL = new CloudSeed::ReverbChannel(256, samplerate, CloudSeed::ChannelLR::Left, delayBuffer, outputBuffer);

    verb.Init(samplerate);
    verb.SetFeedback(0.85f);
    verb.SetLpFreq(18000.0f);

    blk[0].Init(samplerate);
    blk[1].Init(samplerate);

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
