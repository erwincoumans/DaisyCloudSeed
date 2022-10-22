#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

class CloudSeedApp
{
  daisy::DaisyPatch& m_patch;
  public:
    
  CloudSeedApp(daisy::DaisyPatch& patch);
  
  
  void Init();
  
  void AudioTickCallback(float ctrlVal[4], const float * const*in, float **out, size_t size);
 
  void UpdateOled();
  
  void MainLoopCallback();
  
  
};
