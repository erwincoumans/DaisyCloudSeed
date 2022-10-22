///main entry point of DaisyBouquet
#include "daisysp.h"
#include "daisy_patch.h"
#include "cloudyreverb_app.h"

#include <string>


namespace test {
#include "../SamplePlayer/printf.h"
#include "../SamplePlayer/printf.c"
};

char buf[64];

using namespace daisy;
using namespace daisysp;

static DaisyPatch patch;
::daisy::Parameter lpParam;



DaisySynthApp* gApp = 0;
CloudyReverbApp reverb_app(patch);


bool gUpdateOled = true;
bool gRisingEdge = false;
bool gAudioStarted = false;


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


float ctrlVal[4]={1,1,1,1};

void updateControls()
{
   patch.ProcessAnalogControls();
   patch.ProcessDigitalControls();

   //the encoders are not precise and stop above 0 or below 1, 
   // so add some deadzone around 0 and 1
   for (int i = 0; i < 4; i++)
   {
        //Get the four control values
        ctrlVal[i] = patch.controls[i].Process();
        if (ctrlVal[i]<0.01)
           ctrlVal[i] = 0;
        else
          {
            ctrlVal[i]-= 0.01;
          }
        ctrlVal[i]/=0.95;

        if (ctrlVal[i]>1.)
           ctrlVal[i]=1.;
    }
   
}
static void BouquetCallback(const float *const *in, float **out, size_t size)
{
  
     // read some controls
    updateControls();
    
    if (gApp)
    {
      gApp->AudioTickCallback(ctrlVal, in,out,size); 
    } else
    {
      for (size_t i = 0; i < size; i++)
      {
          out[0][i] = 0.;
          out[1][i] = 0.;
          out[2][i] = 0.;
          out[3][i] = 0.;
      }
    }
    
}


void UpdateOled()
{
  if (gApp)
  {
    gApp->UpdateOled();
  }
}



int main(void)
{
    patch.Init();
    lpParam.Init(patch.controls[3], 20, 20000, ::daisy::Parameter::LOGARITHMIC);

    //briefly display the module name
    std::string str = "Cloudy Reverb";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    patch.display.Update();
    patch.DelayMs(1000);

    patch.StartAdc();
    
    
    gAudioStarted = true;
    reverb_app.Init();
    gApp = &reverb_app;
    patch.StartAudio(BouquetCallback);
          
    while(1) 
    {
      
      patch.DelayMs(1);

      if (!gAudioStarted)
      {
        updateControls();
      }
      
      if (gApp)
      {
        gApp->MainLoopCallback(); 
      }
      
      
      //turn off display after 3 seconds hold
      if (gRisingEdge)
      {
        
        gRisingEdge = false;
      }

      if (gUpdateOled)
      {
        static int skip = 0;
        if (skip++>20)
        {
          skip=0;
          UpdateOled();
        }
        
      }
    }
}

