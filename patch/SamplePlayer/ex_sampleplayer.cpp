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

int samplerate;
btAlignedObjectArray<int> bla;
#include <string>

#define kMaxFiles 8

b3ReadWavFile wavFileReaders[kMaxFiles];
b3WavTicker wavTickers[kMaxFiles];
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

WavFileInfo             file_info_[kMaxFiles];
int file_sizes[kMaxFiles];
MemoryDataSource dataSources[kMaxFiles];
#define WAV_FILENAME_MAX  256 /**< Maximum LFN (set to same in FatFs (ffconf.h) */
int selected_file_index=0;
int num_frames = -1;
std::string sd_debug_msg="no sdcard";


static DaisyPatch patch;
::daisy::Parameter lpParam;
static float drylevel, send;

bool gUpdateOled = true;
bool gRisingEdge = true;

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
    
    if (!gRisingEdge)
    {
      
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
         
          wavFileReaders[selected_file_index].tick(0, &wavTickers[selected_file_index], dataSources[selected_file_index], (ctrlVal[0]-0.5)*4.0);
          out[0][i] = wavTickers[selected_file_index].lastFrame_[0];
          out[1][i] = wavTickers[selected_file_index].lastFrame_[1];
          //out[0][i] = 0;
          //out[1][i] = 0;
          
          // Out 3 and 4 are silent
          out[2][i] = 0;
          out[3][i] = 0;
      }
    }
    else
    {
      for (size_t i = 0; i < size; i++)
      {
          out[0][i] = 0;
          out[1][i] = 0;
          out[2][i] = 0;
          out[3][i] = 0;
      }
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
      patch.display.SetCursor(0, 10+4*10);
      int nf = wavFileReaders[selected_file_index].getNumFrames();
      int sz = sizeof(signed short int);
      //test::sprintf(buf, "mem:%d", pool_index);//wavTickers[selected_file_index].time_);
      test::sprintf(buf, "sr:%d", samplerate);//wavTickers[selected_file_index].time_);
      
      patch.display.WriteString(buf, Font_7x10, true);
      
      
    }

    patch.display.Update();
#endif
}




int main(void)
{
    
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
                  // For now lets break anyway to test.
                  //                break;
                  size_t bytesread;
                  if(f_open(&SDFile, file_info_[file_cnt_].name, (FA_OPEN_EXISTING | FA_READ))
                     == FR_OK)
                  {
                    file_sizes[file_cnt_] = f_size(&SDFile);
                    char* memoryBuffer = 0;
                    int memorySize= 0;
                    UINT size = file_sizes[file_cnt_];
                    size_t bytesread;
                    memoryBuffer = (char*) custom_pool_allocate(size);
                    if (memoryBuffer)
                    {
                      UINT bytesRead;
                      // Read the whole WAV file
                      if(f_read(&SDFile,(void *)memoryBuffer,size,&bytesread) == FR_OK)
                      {
                        memorySize = size;
                        dataSources[file_cnt_] = MemoryDataSource(memoryBuffer, memorySize);
                        wavFileReaders[file_cnt_].getWavInfo(dataSources[file_cnt_]);
                        num_frames = wavFileReaders[file_cnt_].getNumFrames();
                        wavFileReaders[file_cnt_].resize();
                        wavTickers[file_cnt_] = wavFileReaders[file_cnt_].createWavTicker(samplerate);
                        file_cnt_++;
                      }
                      f_close(&SDFile);
                    }
                  }
                  
              }
          }
          else
          {
              break;
          }
      } while(result == FR_OK);
    }
    f_closedir(&dir);
    
    gRisingEdge = false;
/////////////////
    patch.StartAdc();
    patch.StartAudio(VerbCallback);



    while(1) 
    {
      
      patch.DelayMs(100);

      if (gRisingEdge)
      {
        wavTickers[selected_file_index].finished_ = false;
        wavTickers[selected_file_index].time_ = 0;
        gRisingEdge = false;
      }

      if (gUpdateOled)
      {
        UpdateOled();
      }
    }

}

