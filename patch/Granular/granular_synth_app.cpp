
#include "granular_synth_app.h"
#include "daisysp.h"
#include "fatfs.h"
using namespace daisy;
using namespace daisysp;

extern FatFSInterface fsi;
/** Global File object for working with test file */
extern FIL SDFile;

namespace test {
#include "../SamplePlayer/printf.h"
};


using namespace daisy;
using namespace daisysp;

float grain_spawn_position = 0.f;
float grain_spawn_position_rand_range = 0.05f;
float grain_pitch = 1.f;
float grain_pitch_rand_range = 0.f;
float grain_duration = .6f;
float grain_duration_rand_range_fraction = 0.25f;
float max_grain_duration = 3.0;
float grain_spawn_interval = 0.03f;
float grain_spawn_interval_rand_fraction = 0.25f;
double spawn_interval = grain_spawn_interval;


GranularSynthApp::GranularSynthApp(DaisyPatch& m_patch, daisy::SdmmcHandler& sd_handler)
  :m_patch(m_patch),
   m_sd_handler(sd_handler),
   m_title("Granular Synth"),
   m_selected_file_index(1),
   m_risingEdge(false),
   m_active_voices(0),
   m_gateOut(false),
   m_mode(0),
   m_timeSinceLastGrain(0)
  
{
  for (int i=0;i<4;i++)
  {
    m_ctrlVal[i] = 0.f;
  }
}

GranularSynthApp::~GranularSynthApp()
{
}

#define MAX_GRAINS 17  
//max grain around in range 17-24

void GranularSynthApp::SpawnGrain()
{
   //find an available slot
  int available_index = -1;
  for (int i=0;i<MAX_GRAINS;i++)
  {
    if (m_grains[i].finished_)
    {
     available_index = i;
     break; 
    }
  }
  if (available_index>=0)
  {
    m_grains[available_index] = m_wavFileReaders[m_selected_file_index].createWavTicker(m_samplerate);
    double r = ((double)rand() / (RAND_MAX));
    double spawn_position = grain_spawn_position + grain_spawn_position_rand_range * r;
    double maxNumFrames = double(m_wavFileReaders[m_selected_file_index].getNumFrames());
    m_grains[available_index].time_ = spawn_position * maxNumFrames;
    m_grains[available_index].starttime_ = m_grains[available_index].time_;
    r = ((double)rand() / (RAND_MAX));
    double duration = max_grain_duration*(grain_duration+grain_duration_rand_range_fraction*grain_duration * r);
    double endTime = m_grains[available_index].time_ + m_samplerate * duration;
    if (endTime>maxNumFrames)
      endTime=maxNumFrames;
    m_grains[available_index].endtime_ = endTime;
    
    m_grains[available_index].wavindex = m_selected_file_index;
    double rnd = ((double)rand() / (RAND_MAX)); 
    double pitch = grain_pitch*(1.+rnd*grain_pitch_rand_range);
    if (pitch<0.1)
      pitch=0.1;
    m_grains[available_index].speed_ = pitch;
    m_grains[available_index].finished_ = false;
  }
 
}
  
void GranularSynthApp::Init()
{
  char buf[64];
 //briefly display the module name
  std::string str = "Granular Synth";
  char* cstr = &str[0];
  m_patch.display.WriteString(cstr, Font_7x10, true);
  
  test::sprintf(buf,"ENC toggle size/pitch");
  m_patch.display.SetCursor(0,10);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"CTRL0: play position");
  m_patch.display.SetCursor(0,20);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"CTRL1: play range");
  m_patch.display.SetCursor(0,30);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"CTRL2: spawn interval");
  m_patch.display.SetCursor(0,40);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"CTRL3: grain size/pitch");
  m_patch.display.SetCursor(0,50);
  m_patch.display.WriteString(buf, Font_6x8, true);
  m_patch.display.Update();
    
  loadWavFiles();
}
  
void GranularSynthApp::loadWavFiles()
{
  m_sd_debug_msg="no (fat32) sdcard";
  m_file_cnt_ = 0;
  FRESULT result = FR_OK;
  FILINFO fno;
  DIR     dir;
  char *  fn;
  m_samplerate = m_patch.AudioSampleRate();
  m_patch.DelayMs(1000);

  // Get a reference to the SD card file system
  FATFS& fs = fsi.GetSDFileSystem();

  // Mount SD Card
  f_mount(&fs, "/", 1);
  m_patch.DelayMs(1000);
  // Open Dir and scan for files.
  if(f_opendir(&dir, "/") == FR_OK)
  {
    m_sd_debug_msg = "sd: no files";
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
        if(m_file_cnt_ < kMaxFiles - 1)
        {
            if(strstr(fn, ".wav") || strstr(fn, ".WAV"))
            {
                strcpy(m_file_info_[m_file_cnt_].name, fn);
                // For now lets break anyway to test.
                //                break;
                size_t bytesread;
                if(f_open(&SDFile, m_file_info_[m_file_cnt_].name, (FA_OPEN_EXISTING | FA_READ))
                   == FR_OK)
                {
                  m_file_sizes[m_file_cnt_] = f_size(&SDFile);
                  char* memoryBuffer = 0;
                  int memorySize= 0;
                  UINT size = m_file_sizes[m_file_cnt_];
                  size_t bytesread;
                  memoryBuffer = (char*) custom_pool_allocate(size);
                  if (memoryBuffer)
                  {
                    UINT bytesRead;
                    // Read the whole WAV file
                    if(f_read(&SDFile,(void *)memoryBuffer,size,&bytesread) == FR_OK)
                    {
                      memorySize = size;
                      m_dataSources[m_file_cnt_] = MemoryDataSource(memoryBuffer, memorySize);
                      m_wavFileReaders[m_file_cnt_].getWavInfo(m_dataSources[m_file_cnt_]);
                      
                      m_wavFileReaders[m_file_cnt_].resize();
                      m_grains[m_file_cnt_] = m_wavFileReaders[m_file_cnt_].createWavTicker(m_samplerate);
                      //start each sound inactive
                      m_grains[m_file_cnt_].finished_ = true;
                      m_file_cnt_++;
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

}
void GranularSynthApp::Exit()
{
}
  
void GranularSynthApp::AudioTickCallback(float ctrlVal[4], const float *const*in, float **out, size_t size)
{
  for (int i=0;i<4;i++)
  {
    m_ctrlVal[i] = ctrlVal[i];
  }
 
  m_timeSinceLastGrain += 0.001;
  if (m_timeSinceLastGrain > spawn_interval)
  {
    SpawnGrain();
    m_timeSinceLastGrain = 0;
    double r = ((double)rand() / (RAND_MAX));
    double grain_spawn_interval_rand_range = grain_spawn_interval_rand_fraction*grain_spawn_interval;
    spawn_interval = (grain_spawn_interval + r * grain_spawn_interval_rand_range);
  }
  float send = 1.0;
  
  if (m_patch.encoder.RisingEdge() || m_patch.gate_input[DaisyPatch::GateInput::GATE_IN_1].Trig())
  {
    m_risingEdge = true;
  }
  
  if (!m_risingEdge)
  {
    
    int inc = 0;
     // Change selected file with encoder.
    inc = m_patch.encoder.Increment();
    if(inc > 0)
    {
        if ((m_selected_file_index+1) < m_file_cnt_)
        {
          m_selected_file_index++;
        }
    }
    else if(inc < 0)
    {
      if (m_selected_file_index > 0)
      {
        m_selected_file_index--;
      }
    }
    
    float delta = 0.01;
    m_prevCtrlVal[0] = ctrlVal[0];
    
    if ((m_prevCtrlVal[1] < (ctrlVal[1]-delta)) || (m_prevCtrlVal[1] > (ctrlVal[1]+delta)))
    {
      
      m_prevCtrlVal[1] = ctrlVal[1];
    }

    if ((m_prevCtrlVal[2] < (ctrlVal[2]-delta)) || (m_prevCtrlVal[2] > (ctrlVal[2]+delta)))
    {
      
      m_prevCtrlVal[2] = ctrlVal[2];
    }
    if ((m_prevCtrlVal[3] < (ctrlVal[3]-delta)) || (m_prevCtrlVal[3] > (ctrlVal[3]+delta)))
    {
      
      m_prevCtrlVal[3] = ctrlVal[3];
    }
    
    for (size_t i = 0; i < size; i++)
    {
	      out[0][i] = 0.f;
	      out[1][i] = 0.f;
	      out[2][i] = 0;
        out[3][i] = 0;
    }      
	  m_active_voices=0;
	  grain_spawn_position = ctrlVal[0];
	  grain_spawn_position_rand_range = ctrlVal[1];
	  grain_spawn_interval = ctrlVal[2];
	  if (m_mode==1)
	  {
      grain_pitch = ctrlVal[3];
    }
    else
    {
      grain_duration = ctrlVal[3];
    }
    {
      for (int g=0;g<m_file_cnt_;g++)
      {
        //somewhere around 22-24 is the current maximum, so clamp at 20 active samples
        if (!m_grains[g].finished_ && m_active_voices < 20)
        {
          m_active_voices++;
          
          double volume_gain = 1./10;//ctrlVal[1]*1./12.;
          double volume = volume_gain * m_grains[g].env_volume2();
          if (!m_grains[g].finished_)
          {
            //todo: pitch shift
            double speed = m_grains[g].speed_;
            int wavindex = m_grains[g].wavindex;
            m_wavFileReaders[wavindex].tick(&m_grains[g], m_dataSources[wavindex], speed, volume, size, out[0], out[1]);
          }
        }
      }
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
  
void GranularSynthApp::UpdateOled()
{
  char buf[64];
  m_patch.display.Fill(false);

  test::sprintf(buf,"%s (%d)", "Granular Synth", m_file_cnt_);
  m_patch.display.SetCursor(0,0);
  m_patch.display.WriteString(buf, Font_6x8, true);
  
  if (m_file_cnt_==0)
  {
    m_patch.display.SetCursor(0, 10+0*10);
    test::sprintf(buf, "%s", m_sd_debug_msg.c_str());
    m_patch.display.WriteString(buf, Font_7x10, true);
  } else
  {
    int start = m_selected_file_index-2;
    if (start<0)
      start = 0;
    for (int i=0;i<4;i++)
    {
      
      m_patch.display.SetCursor(0, 10+i*10);
      const char* selected = ((i+start)==m_selected_file_index)? ">" : " ";
      if (i<m_file_cnt_)
      {
        test::sprintf(buf, "%s%s", selected, m_file_info_[i+start].name);
      } else
      {
        test::sprintf(buf, " <empty>");
      }
      m_patch.display.WriteString(buf, Font_7x10, true);
    }
    m_patch.display.SetCursor(0, 10+4*10);
    
    if (m_mode==0)
    {
      test::sprintf(buf, "a=%d, dur=%f", m_active_voices, grain_duration);//grain_spawn_position);
      
    } else
    {
      test::sprintf(buf, "a=%d, pit=%f", m_active_voices, grain_pitch);//grain_spawn_position);
    }
    m_patch.display.WriteString(buf, Font_7x10, true);
  }

  m_patch.display.Update();
}

  
  
void GranularSynthApp::MainLoopCallback()
{
   if (m_risingEdge)
    {
      m_mode = 1-m_mode;
      m_risingEdge = false;
    }

    if (m_gateOut)
    {
       dsy_gpio_toggle(&m_patch.gate_output);
       m_patch.DelayMs(1);
       dsy_gpio_toggle(&m_patch.gate_output);
       m_gateOut = false;
    }
}
