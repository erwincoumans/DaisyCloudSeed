
#include "sample_player_app.h"


namespace test {
#include "printf.h"
};


using namespace daisy;
using namespace daisysp;

SamplePlayerApp::SamplePlayerApp(DaisyPatch& m_patch, daisy::SdmmcHandler& sd_handler)
  :m_patch(m_patch),
   m_sd_handler(sd_handler),
   m_title("SamplePlayer"),
   m_selected_file_index(1),
   m_risingEdge(false),
   m_active_voices(0),
   m_gateOut(false)
  
{
  for (int i=0;i<4;i++)
  {
    m_ctrlVal[i] = 0.f;
  }
}

SamplePlayerApp::~SamplePlayerApp()
{
}
  
extern char buf[64];
  
void SamplePlayerApp::Init()
{
  
  //64 bytes should be enough
  
 //briefly display the module name
  std::string str = "SamplePlayer";
  char* cstr = &str[0];
  m_patch.display.WriteString(cstr, Font_7x10, true);
  
  test::sprintf(buf,"CTRL1 for speed");
  m_patch.display.SetCursor(0,10);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"CTRL2 for volume");
  m_patch.display.SetCursor(0,20);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"ENC to play & select");
  m_patch.display.SetCursor(0,30);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"GATE OUT to GATE IN1");
  m_patch.display.SetCursor(0,40);
  m_patch.display.WriteString(buf, Font_6x8, true);
  test::sprintf(buf,"will loop sound");
  m_patch.display.SetCursor(0,50);
  m_patch.display.WriteString(buf, Font_6x8, true);
  m_patch.display.Update();
    
  loadWavFiles();
}
  
void SamplePlayerApp::loadWavFiles()
{
  m_sd_debug_msg="no (fat32) sdcard";
  m_file_cnt_ = 0;
  FRESULT result = FR_OK;
  FILINFO fno;
  DIR     dir;
  char *  fn;
  int samplerate = m_patch.AudioSampleRate();
  m_patch.DelayMs(1000);
  // Mount SD Card
  f_mount(&SDFatFS, SDPath, 1);
  m_patch.DelayMs(1000);
  // Open Dir and scan for files.
  if(f_opendir(&dir, SDPath) == FR_OK)
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
                      m_wavTickers[m_file_cnt_] = m_wavFileReaders[m_file_cnt_].createWavTicker(samplerate);
                      //start each sound inactive
                      m_wavTickers[m_file_cnt_].finished_ = true;
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
void SamplePlayerApp::Exit()
{
}
  
void SamplePlayerApp::AudioTickCallback(float ctrlVal[4], float **in, float **out, size_t size)
{
  for (int i=0;i<4;i++)
  {
    m_ctrlVal[i] = ctrlVal[i];
  }
 
  
  float send = 1.0;
  float dryL, dryR, wetL, wetR, sendL, sendR;
   // read some controls
  
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
	      out[0][i] = 0.;
	      out[1][i] = 0.;
	      out[2][i] = 0.;
        out[3][i] = 0.;
    }      
	  m_active_voices=0;
    m_playback_speed = (ctrlVal[0]-0.5)*2.0;
    {
      for (int g=0;g<m_file_cnt_;g++)
      {
        //somewhere around 22-24 is the current maximum, so clamp at 20 active samples
        if (!m_wavTickers[g].finished_ && m_active_voices < 20)
        {
          m_active_voices++;
        }
      }
      
      for (int g=0;g<m_file_cnt_;g++)
      {
        //somewhere around 22-24 is the current maximum, so clamp at 20 active samples
        if (!m_wavTickers[g].finished_ && m_active_voices < 20)
        {
          float volume = ctrlVal[1]/float(m_active_voices);
          if (!m_wavTickers[g].finished_)
          {
            m_wavFileReaders[g].tick(&m_wavTickers[g], m_dataSources[g], m_playback_speed, volume, size, out[0], out[1]);

            if (m_wavTickers[g].finished_)
            {
              if (g == m_selected_file_index)
                {
                  m_gateOut=true;
                } else
                {
                  if (m_wavTickers[g].time_<0)
                    m_wavTickers[g].time_ = m_wavTickers[g].endtime_;
                  else
                    m_wavTickers[g].time_ = m_wavTickers[g].starttime_;
                  m_wavTickers[g].finished_ = false;                
                }
            }
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
  
void SamplePlayerApp::UpdateOled()
{

    m_patch.display.Fill(false);

    test::sprintf(buf,"%s (%d)", "SamplePlayer", m_file_cnt_);
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
      int nf = m_wavFileReaders[m_selected_file_index].getNumFrames();
      int sz = sizeof(signed short int);
      
      float frac = nf? 100.*m_wavTickers[m_selected_file_index].time_/float(nf) : 0;
      test::sprintf(buf, "s:%.2f,%3.0f\%,a=%d", m_playback_speed, frac, m_active_voices);
      m_patch.display.WriteString(buf, Font_7x10, true);
      
      
    }

    m_patch.display.Update();

}

  
  
void SamplePlayerApp::MainLoopCallback()
{
   if (m_risingEdge)
    {
      
      if (!m_wavTickers[m_selected_file_index].finished_)
      {
        m_wavTickers[m_selected_file_index].finished_ = true;
      } else
      {
        if (m_wavTickers[m_selected_file_index].time_<0)
          m_wavTickers[m_selected_file_index].time_ = m_wavTickers[m_selected_file_index].endtime_;
        else
          m_wavTickers[m_selected_file_index].time_ = m_wavTickers[m_selected_file_index].starttime_;
        m_wavTickers[m_selected_file_index].finished_ = false;
      }
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
