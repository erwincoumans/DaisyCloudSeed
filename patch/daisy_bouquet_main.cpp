///main entry point of DaisyBouquet
#include "daisysp.h"
#include "daisy_patch.h"
#include "dummy_app.h"
#include "CloudSeed/cloudseed_app.h"
#include "SamplePlayer/sample_player_app.h"
#include "Granular/granular_synth_app.h"
#include <string>
#include "fatfs.h"

namespace test {
#include "printf.h"
#include "printf.c"
};

char buf[64];

using namespace daisy;
using namespace daisysp;

static DaisyPatch patch;
::daisy::Parameter lpParam;
static SdmmcHandler sd_handler;


DaisySynthApp* gApp = 0;
//CloudSeedApp cloud(patch);
SamplePlayerApp player(patch, sd_handler);
GranularSynthApp granular(patch, sd_handler);
DummyApp dummy2(patch, "dummy2");
DummyApp dummy3(patch, "dummy3");
DaisySynthApp* gAvailableApps[4]={&player,&granular, &dummy2,&dummy3};


bool gUpdateOled = true;
bool gRisingEdge = false;
bool gAudioStarted = false;
int menu_select=0;

#define CUSTOM_POOL_SIZE (64*1024*1024)
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
   patch.UpdateAnalogControls();
   patch.DebounceControls();

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
    
    if (patch.encoder.RisingEdge())
    {
      gRisingEdge = true;
    }
    
    int inc = 0;
       // Change selected file with encoder.
    inc = patch.encoder.Increment();
    menu_select += inc;
    if (menu_select<0)
      menu_select=0;
    if (menu_select>3)
      menu_select=3;
      
    
}
static void BouquetCallback(float **in, float **out, size_t size)
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
  //64 bytes should be enough
  
  if (gApp)
  {
    gApp->UpdateOled();
  }
  else
  {
    //patch.DisplayControls(false);
    patch.display.Fill(false);

    test::sprintf(buf,"%s", "DaisyBouquet");
    patch.display.SetCursor(0,0);
    patch.display.WriteString(buf, Font_7x10, true);
    

    for (int i=0;i<4;i++)
    {
      const char* selected = (i==menu_select)? ">" : " ";
      patch.display.SetCursor(0, 10+i*10);
      test::sprintf(buf, "%s%s", selected, gAvailableApps[i]->GetName().c_str());
      patch.display.WriteString(buf, Font_7x10, true);
    }

    patch.display.SetCursor(0, 10+4*10);
    test::sprintf(buf, "sel:%d, audio:%d", menu_select, gAudioStarted);
    patch.display.WriteString(buf, Font_7x10, true);
      
      
    patch.display.Update();
  }
}



int main(void)
{
    patch.Init();
    lpParam.Init(patch.controls[3], 20, 20000, ::daisy::Parameter::LOGARITHMIC);

    //briefly display the module name
    std::string str = "DaisyBouquet";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    patch.display.Update();
    patch.DelayMs(1000);

    patch.StartAdc();
    
    sd_handler.Init();
     
    // Init Fatfs
    dsy_fatfs_init();

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
        if (!gAudioStarted)
        {
          gAudioStarted = true;
          gAvailableApps[menu_select]->Init();
          gApp = gAvailableApps[menu_select];
          patch.StartAudio(BouquetCallback);
          
        }
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

