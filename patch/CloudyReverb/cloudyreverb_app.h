#include "../daisy_synth_app.h"
#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

class CloudyReverbApp : public DaisySynthApp
{
  daisy::DaisyPatch& m_patch;
  std::string m_name;
  public:
    
  CloudyReverbApp (daisy::DaisyPatch& patch);
  
  virtual ~CloudyReverbApp ();
  
  virtual void Init();
  
  virtual void Exit();
  
  virtual void AudioTickCallback(float ctrlVal[4], const float *const *in, float **out, size_t size);
 
  virtual void UpdateOled();
  
  virtual void MainLoopCallback();
  
  virtual const std::string& GetName() const
  {
    return m_name;
  }
  
};
