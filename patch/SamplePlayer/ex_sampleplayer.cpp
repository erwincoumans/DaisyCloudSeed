#include "daisysp.h"
#include "daisy_patch.h"
#include "daisy_core.h"
#include "util/wav_format.h"
#include "daisy_core.h"
#include <string.h>
#include "hid/wavplayer.h"

#include "fatfs.h"
#include "btAlignedObjectArray.h"
#include "b3ReadWavFile.h"

btAlignedObjectArray<int> bla;
#include <string>

b3ReadWavFile wavFileReader;

namespace test {

#include "printf.h"
#include "printf.c"
};

using namespace daisy;
using namespace daisysp;

static SdmmcHandler sd;


//64 bytes should be enough
char buf[64];

size_t file_cnt_=0, file_sel_=0;
#define kMaxFiles 8
WavFileInfo             file_info_[kMaxFiles];
#define WAV_FILENAME_MAX  256 /**< Maximum LFN (set to same in FatFs (ffconf.h) */
int selected_file_index=0;

std::string sd_debug_msg="no sdcard";


static DaisyPatch patch;
::daisy::Parameter lpParam;
static float drylevel, send;

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
    
    int inc = 0;
     // Change selected file with encoder.
    inc = patch.encoder.Increment();
    if(inc > 0)
    {
        if ((selected_file_index+1) < file_cnt_)
        {
          selected_file_index++;
        }
    }
    else if(inc < 0)
    {
      if (selected_file_index > 0)
      {
        selected_file_index--;
      }
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
      
      prevCtrlVal[1] = ctrlVal[1];
    }

    if ((prevCtrlVal[2] < (ctrlVal[2]-delta)) || (prevCtrlVal[2] > (ctrlVal[2]+delta)))
    {
      
      prevCtrlVal[2] = ctrlVal[2];
    }
    if ((prevCtrlVal[3] < (ctrlVal[3]-delta)) || (prevCtrlVal[3] > (ctrlVal[3]+delta)))
    {
      
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
        

        out[0][i] = 0;
        out[1][i] = 0;

        // Out 3 and 4 are just wet
        out[2][i] = 0;
        out[3][i] = 0;
    }
}

char* param_names[4] = {"dry:","wet:", "decay:", "diffusion:"};

void UpdateOled()
{
    //patch.DisplayControls(false);
#if 1
    patch.display.Fill(false);

    test::sprintf(buf,"%s (%d)", "SamplePlayer", file_cnt_);
    patch.display.SetCursor(0,0);
    patch.display.WriteString(buf, Font_6x8, true);
    
    if (file_cnt_==0)
    {
      patch.display.SetCursor(0, 10+0*10);
      test::sprintf(buf, "%s", sd_debug_msg.c_str());
      patch.display.WriteString(buf, Font_7x10, true);
    } else
    {
      for (int i=0;i<4;i++)
      {
        patch.display.SetCursor(0, 10+i*10);
        const char* selected = (i==selected_file_index)? ">" : " ";
        if (i<file_cnt_)
        {
          test::sprintf(buf, "%s%s", selected, file_info_[i].name);
        } else
        {
          test::sprintf(buf, " <empty>");
        }
        patch.display.WriteString(buf, Font_7x10, true);
      }
    }

    patch.display.Update();
#endif
}




int main(void)
{
    float samplerate;
    patch.Init();
    samplerate = patch.AudioSampleRate();


    lpParam.Init(patch.controls[3], 20, 20000, ::daisy::Parameter::LOGARITHMIC);

    //briefly display the module name
    std::string str = "SamplePlayer";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    patch.display.Update();
    patch.DelayMs(1000);




/////////////////
    file_cnt_ = 0;
    FRESULT result = FR_OK;
    FILINFO fno;
    DIR     dir;
    char *  fn;
    file_sel_ = 0;
    
    sd.Init();
    // Init Fatfs
    dsy_fatfs_init();
    // Mount SD Card
    f_mount(&SDFatFS, SDPath, 1);
    // Open Dir and scan for files.
    if(f_opendir(&dir, SDPath) == FR_OK)
    {
      sd_debug_msg = "sd: no files";
      do
      {
          result = f_readdir(&dir, &fno);
          // Exit if bad read or NULL fname
          if(result != FR_OK || fno.fname[0] == 0)
              break;
          // Skip if its a directory or a hidden file.
          if(fno.fattrib & (AM_HID | AM_DIR))
              continue;
          // Now we'll check if its .wav and add to the list.
          fn = fno.fname;
          if(file_cnt_ < kMaxFiles - 1)
          {
              if(strstr(fn, ".wav") || strstr(fn, ".WAV"))
              {
                  strcpy(file_info_[file_cnt_].name, fn);
                  file_cnt_++;
                  // For now lets break anyway to test.
                  //                break;
              }
          }
          else
          {
              break;
          }
      } while(result == FR_OK);
    }
/////////////////



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

