#ifndef SAMPLE_PLAYER_APP_H
#define SAMPLE_PLAYER_APP_H

#include "../daisy_synth_app.h"

#include "daisysp.h"
#include "daisy_patch.h"
#include <string>
#include "btAlignedObjectArray.h"
#include "b3ReadWavFile.h"
#include "daisy_core.h"
#include "fatfs.h"

#define kMaxFiles 64

class SamplePlayerApp : public DaisySynthApp
{
  daisy::DaisyPatch& m_patch;
  daisy::SdmmcHandler& m_sd_handler;
  std::string m_title;
  float m_ctrlVal[4];
  float m_prevCtrlVal[4];
  b3ReadWavFile m_wavFileReaders[kMaxFiles];
  b3WavTicker m_wavTickers[kMaxFiles];
  daisy::WavFileInfo m_file_info_[kMaxFiles];
  int m_file_sizes[kMaxFiles];
  MemoryDataSource m_dataSources[kMaxFiles];
  int m_selected_file_index;
  int m_risingEdge;
  int m_active_voices;
  int m_file_cnt_;
  bool m_gateOut;
  float m_playback_speed;
  std::string m_sd_debug_msg;
  void loadWavFiles();
  
  public:
    
  SamplePlayerApp(daisy::DaisyPatch& patch, daisy::SdmmcHandler& sd_handler);
  
  virtual ~SamplePlayerApp();
  
  virtual void Init();
  
  virtual void Exit();
  
  virtual void AudioTickCallback(float ctrlVal[4], float **in, float **out, size_t size);
  
  virtual void UpdateOled();
  
  virtual void MainLoopCallback();
  
  virtual const std::string& GetName() const
  {
    return m_title;
  }
  
};

#endif //SAMPLE_PLAYER_APP_H
