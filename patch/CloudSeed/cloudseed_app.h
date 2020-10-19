#include "../daisy_synth_app.h"
#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

class CloudSeedApp : public DaisySynthApp
{
  daisy::DaisyPatch& m_patch;
  std::string m_name;
  public:
    
  CloudSeedApp(daisy::DaisyPatch& patch);
  
  virtual ~CloudSeedApp();
  
  virtual void Init();
  
  virtual void Exit();
  
  virtual void AudioTickCallback(float ctrlVal[4], float **in, float **out, size_t size);
 
  virtual void UpdateOled();
  
  virtual void MainLoopCallback();
  
  virtual const std::string& GetName() const
  {
    return m_name;
  }
  
};