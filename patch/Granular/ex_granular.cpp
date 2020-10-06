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

#define kMaxFiles 64
int active=0;

b3ReadWavFile wavFileReaders[kMaxFiles];
int selected_file_index=0;

b3WavTicker grains[kMaxFiles];
double timeSinceLastGrain = 0.;


float grain_spawn_position = 0.f;
float grain_spawn_position_rand_range = 0.024f;
float grain_pitch = 1.f;
float grain_pitch_rand_range = 0.f;
float grain_duration = .6f;
float grain_duration_rand_range = 0.25f;
float grain_spawn_interval = 0.03f;
float grain_spawn_interval_rand_range = 0.02f;
double spawn_interval = grain_spawn_interval;
size_t file_cnt_=0, file_sel_=0;
double rnd=-1;

//max grain around in range 17-24
#define MAX_GRAINS 17
void spawnGrain()
{
   //find an available slot
  int available_index = -1;
  for (int i=0;i<MAX_GRAINS;i++)
  {
    if (grains[i].finished_)
    {
     available_index = i;
     break; 
    }
  }
  if (available_index>=0)
  {
    grains[available_index] = wavFileReaders[selected_file_index].createWavTicker(samplerate);
    double r = ((double)rand() / (RAND_MAX));
    double spawn_position = grain_spawn_position + grain_spawn_position_rand_range * r;
    double maxNumFrames = double(wavFileReaders[selected_file_index].getNumFrames());
    grains[available_index].time_ = spawn_position * maxNumFrames;
    grains[available_index].starttime_ = grains[available_index].time_;
    r = ((double)rand() / (RAND_MAX));
    double duration = grain_duration+grain_duration_rand_range * r;
    double endTime = grains[available_index].time_ + samplerate * duration;
    if (endTime>maxNumFrames)
      endTime=maxNumFrames;
    grains[available_index].endtime_ = endTime;
    
    grains[available_index].wavindex = selected_file_index;
    rnd = ((double)rand() / (RAND_MAX)); 
    grains[available_index].speed_ = grain_pitch*(rnd < 0.5 ? 1. : 2.);
    grains[available_index].finished_ = false;
  }
}

namespace test {

#include "printf.h"
#include "printf.c"
};

using namespace daisy;
using namespace daisysp;

static SdmmcHandler sd;


//64 bytes should be enough
char buf[64];



WavFileInfo             file_info_[kMaxFiles];
int file_sizes[kMaxFiles];
MemoryDataSource dataSources[kMaxFiles];
#define WAV_FILENAME_MAX  256 /**< Maximum LFN (set to same in FatFs (ffconf.h) */

int num_frames = -1;
std::string sd_debug_msg="no sdcard";


static DaisyPatch patch;
::daisy::Parameter lpParam;
static float drylevel, send;

bool gUpdateOled = true;
bool gRisingEdge = true;
bool gGateOut = false;
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


float ctrlVal[4];
float prevCtrlVal[4];




static void VerbCallback(float **in, float **out, size_t size)
{
    timeSinceLastGrain += 0.001;
    if (timeSinceLastGrain > spawn_interval)
    {
      spawnGrain();
      timeSinceLastGrain = 0;
      double r = ((double)rand() / (RAND_MAX));
      spawn_interval = grain_spawn_interval + r * grain_spawn_interval_rand_range;
    }
    send = 1.0;
    float dryL, dryR, wetL, wetR, sendL, sendR;
     // read some controls
    
    patch.UpdateAnalogControls();
    patch.DebounceControls();

    if (patch.encoder.RisingEdge() || patch.gate_input[DaisyPatch::GateInput::GATE_IN_1].Trig())
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
  	      out[0][i] = 0.f;
  	      out[1][i] = 0.f;
  	      out[2][i] = 0;
          out[3][i] = 0;
      }      
  	  active=0;
  	  grain_spawn_position = ctrlVal[0];
  	  grain_spawn_position_rand_range = ctrlVal[1];
  	  grain_spawn_interval = ctrlVal[2];
      grain_pitch = ctrlVal[3];
      {
        for (int g=0;g<file_cnt_;g++)
        {
          //somewhere around 22-24 is the current maximum, so clamp at 20 active samples
          if (!grains[g].finished_ && active < 20)
          {
            active++;
            
            float volume_gain = 1./10;//ctrlVal[1]*1./12.;
            
            if (!grains[g].finished_)
            {
              //todo: pitch shift
              double speed = grains[g].speed_;
              double volume = grains[g].env_volume2() * volume_gain;
              int wavindex = grains[g].wavindex;
              wavFileReaders[wavindex].tick(&grains[g], dataSources[wavindex], speed, volume, size, out[0], out[1]);
            }
          }
        }
      }
      //out[0][i] = 0;
      //out[1][i] = 0;
      
      // Out 3 and 4 are silent
      
      
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

    test::sprintf(buf,"%s (%d)", "DaisyGranular", file_cnt_);
    patch.display.SetCursor(0,0);
    patch.display.WriteString(buf, Font_6x8, true);
    
    if (file_cnt_==0)
    {
      patch.display.SetCursor(0, 10+0*10);
      test::sprintf(buf, "%s", sd_debug_msg.c_str());
      patch.display.WriteString(buf, Font_7x10, true);
    } else
    {
      int start = selected_file_index-2;
      if (start<0)
        start = 0;
      for (int i=0;i<4;i++)
      {
        
        patch.display.SetCursor(0, 10+i*10);
        const char* selected = ((i+start)==selected_file_index)? ">" : " ";
        if (i<file_cnt_)
        {
          test::sprintf(buf, "%s%s", selected, file_info_[i+start].name);
        } else
        {
          test::sprintf(buf, " <empty>");
        }
        patch.display.WriteString(buf, Font_7x10, true);
      }
      patch.display.SetCursor(0, 10+4*10);
      
      
      test::sprintf(buf, "a=%d, ", active, grain_spawn_position,grain_pitch);//grain_spawn_position);
      
      patch.display.WriteString(buf, Font_7x10, true);
      
      
    }

    patch.display.Update();
#endif
}




int main(void)
{
    
    patch.Init();
    samplerate = patch.AudioSampleRate();
    patch.DelayMs(200);

    lpParam.Init(patch.controls[3], 20, 20000, ::daisy::Parameter::LOGARITHMIC);

    //briefly display the module name
    std::string str = "DaisyGranular";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    
    //test::sprintf(buf,"CTRL1 for speed");
    //patch.display.SetCursor(0,10);
    //patch.display.WriteString(buf, Font_6x8, true);
    //test::sprintf(buf,"CTRL2 for volume");
    //patch.display.SetCursor(0,20);
    //patch.display.WriteString(buf, Font_6x8, true);
    test::sprintf(buf,"ENC to play & select");
    patch.display.SetCursor(0,30);
    patch.display.WriteString(buf, Font_6x8, true);
    //test::sprintf(buf,"GATE OUT to GATE IN1");
    //patch.display.SetCursor(0,40);
    //patch.display.WriteString(buf, Font_6x8, true);
    //test::sprintf(buf,"will loop sound");
    //patch.display.SetCursor(0,50);
    //patch.display.WriteString(buf, Font_6x8, true);
    patch.display.Update();
    




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
    patch.DelayMs(1000);
    // Mount SD Card
    f_mount(&SDFatFS, SDPath, 1);
    patch.DelayMs(1000);
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
                        grains[file_cnt_] = wavFileReaders[file_cnt_].createWavTicker(samplerate);
                        //start each sound inactive
                        grains[file_cnt_].finished_ = true;
                        file_cnt_++;
                      }
                      
                    }
                    f_close(&SDFile);
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
      patch.DelayMs(20);

      if (gRisingEdge)
      {
        spawnGrain();
        gRisingEdge = false;
      }      
      if (gGateOut)
      {
         dsy_gpio_toggle(&patch.gate_output);
         patch.DelayMs(1);
         dsy_gpio_toggle(&patch.gate_output);
         gGateOut = false;
      }
      if (gUpdateOled)
      {
         UpdateOled();
      }
    }

}

