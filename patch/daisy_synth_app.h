#ifndef DAISY_SYNTH_APP_H
#define DAISY_SYNTH_APP_H

#include <cstddef> //size_t
#include <string>

class DaisySynthApp
{
  public:
    
  virtual ~DaisySynthApp()
  {
  }
  
  virtual void Init() = 0;
  
  virtual void Exit() = 0;
  
  virtual void AudioTickCallback(float ctrlVal[4], const float * const*in, float **out, size_t size)=0;
  
  virtual void UpdateOled() = 0;
  
  virtual void MainLoopCallback()=0;
  
  virtual const std::string& GetName() const = 0;
  
};

#endif //DAISY_SYNTH_APP_H
