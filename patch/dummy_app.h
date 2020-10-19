#ifndef DUMMY_APP_H
#define DUMMY_APP_H

#include "daisy_synth_app.h"

#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

class DummyApp : public DaisySynthApp
{
  daisy::DaisyPatch& m_patch;
  std::string m_title;
  float m_ctrlVal[4];
  public:
    
  DummyApp(daisy::DaisyPatch&, const std::string& title);
  
  virtual ~DummyApp();
  
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

#endif //DUMMY_APP_H
