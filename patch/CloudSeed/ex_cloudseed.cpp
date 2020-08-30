#include "daisysp.h"
#include "daisy_patch.h"
#include <string>


namespace test {
#include "printf.h"
#include "printf.c"
};
//64 bytes should be enough
char buf[64];


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
bool gUpdateOled = true;
bool gRisingEdge = false;

#define CUSTOM_POOL_SIZE (48*1024*1024)

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


float ctrlVal[4];
float prevCtrlVal[4];



static void VerbCallback(float **in, float **out, size_t size)
{
    send = 1.0;
    float dryL, dryR, wetL, wetR, sendL, sendR;
     // read some controls
    
    patch.UpdateAnalogControls();
    patch.DebounceControls();

    if (patch.encoder.RisingEdge())
    {
      gRisingEdge = true;
    }
        
    for (int i = 0; i < 4; i++)
    {
        //Get the four control values
        ctrlVal[i] = patch.controls[i].Process();
        if (ctrlVal[i]<0.01)
           ctrlVal[i] = 0;
        if (ctrlVal[i]>0.97)
           ctrlVal[i]=1;
    }

    drylevel = ctrlVal[0];
    
    
    
    
    float delta = 0.01;
   
    prevCtrlVal[0] = ctrlVal[0];
    
    if ((prevCtrlVal[1] < (ctrlVal[1]-delta)) || (prevCtrlVal[1] > (ctrlVal[1]+delta)))
    {
      reverb->SetParameter(::Parameter::MainOut, ctrlVal[1]);
      prevCtrlVal[1] = ctrlVal[1];
    }

    if ((prevCtrlVal[2] < (ctrlVal[2]-delta)) || (prevCtrlVal[2] > (ctrlVal[2]+delta)))
    {
      reverb->SetParameter(::Parameter::LineDecay, ctrlVal[2]);
      prevCtrlVal[2] = ctrlVal[2];
    }
    if ((prevCtrlVal[3] < (ctrlVal[3]-delta)) || (prevCtrlVal[3] > (ctrlVal[3]+delta)))
    {
      reverb->SetParameter(::Parameter::LateDiffusionFeedback, ctrlVal[3]);
      prevCtrlVal[3] = ctrlVal[3];
    }


    for (size_t i = 0; i < size; i++)
    {
       
        // Read Inputs (only stereo in are used)
        dryL = in[0][i]*drylevel;
        dryR = in[1][i]*drylevel;

        // Send Signal to Reverb
        sendL = dryL * send;
        sendR = dryR * send;
        //verb.Process(sendL, sendR, &wetL, &wetR);
        float ins[2]={sendL,sendR};
	      float outs[2]={sendL,sendR};
        reverb->Process( ins, outs, 1);
        wetL=outs[0];
        wetR=outs[1];	


        out[0][i] = outs[0];
        out[1][i] = outs[1];

        // Out 3 and 4 are just wet
        out[2][i] = outs[0];
        out[3][i] = outs[1];
    }
}

char* param_names[4] = {"dry:","wet:", "decay:", "diffusion:"};

void UpdateOled()
{
    //patch.DisplayControls(false);
#if 1
    patch.display.Fill(false);

    test::sprintf(buf,"%s", "CloudSeed Reverb");
    patch.display.SetCursor(0,0);
    patch.display.WriteString(buf, Font_6x8, true);
    

    for (int i=0;i<4;i++)
    {
      //two circuits
      patch.display.SetCursor(0, 10+i*10);
      test::sprintf(buf, "%s:%1.5f", param_names[i], prevCtrlVal[i]);
      patch.display.WriteString(buf, Font_7x10, true);
    }

    patch.display.Update();
#endif
}



int main(void)
{
    float samplerate;
    patch.Init();
    samplerate = patch.AudioSampleRate();

    AudioLib::ValueTables::Init();
    CloudSeed::FastSin::Init();
    reverb = new CloudSeed::ReverbController(samplerate);
    reverb->ClearBuffers();
    //reverb->initFactoryRubiKaFields();
    //reverb->initFactoryDullEchos();
    //reverb->initFactoryHyperplane();
    //reverb->initFactory90sAreBack();

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
      
      patch.DelayMs(100);

      if (gRisingEdge)
      {
        patch.display.Fill(false);
        gUpdateOled = !gUpdateOled;
        patch.display.Update();
        gRisingEdge = false;
      }

      if (gUpdateOled)
      {
        UpdateOled();
      }
    }

}

