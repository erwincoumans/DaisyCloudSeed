#include "cloudyreverb_app.h"


#include "daisysp.h"
#include "daisy_patch.h"
namespace test {
#include "../SamplePlayer/printf.h"
};

using namespace daisy;
using namespace daisysp;

#include "clouds/dsp/frame.h"
//#include "clouds/dsp/fx/reverb.h"
//using namespace clouds;

#include "rings/dsp/fx/reverb.h"
using namespace rings;

Reverb clouds_reverb;
uint16_t reverb_buffer[65536];

CloudyReverbApp ::CloudyReverbApp (daisy::DaisyPatch& patch)
  :m_patch(patch),
  m_name("Cloudy Reverb")
{
}

CloudyReverbApp ::~CloudyReverbApp ()
{
}
  


void CloudyReverbApp ::Init()
{
  float samplerate = m_patch.AudioSampleRate();
  clouds_reverb.Init(reverb_buffer);
}
  
void CloudyReverbApp ::Exit()
{
  
}
  
float prevCtrlVal[4] = {0};

void CloudyReverbApp ::AudioTickCallback(float ctrlVal[4], const float * const*in, float **out, size_t size)
{
    prevCtrlVal[0] = ctrlVal[0];
    float delta = 0.01;
    if ((prevCtrlVal[1] < (ctrlVal[1]-delta)) || (prevCtrlVal[1] > (ctrlVal[1]+delta)))
    {
      //reverb->SetParameter(::Parameter::MainOut, ctrlVal[1]);
      prevCtrlVal[1] = ctrlVal[1];
    }

    if ((prevCtrlVal[2] < (ctrlVal[2]-delta)) || (prevCtrlVal[2] > (ctrlVal[2]+delta)))
    {
      //reverb->SetParameter(::Parameter::LineDecay, ctrlVal[2]);
      prevCtrlVal[2] = ctrlVal[2];
    }
    if ((prevCtrlVal[3] < (ctrlVal[3]-delta)) || (prevCtrlVal[3] > (ctrlVal[3]+delta)))
    {
      //reverb->SetParameter(::Parameter::LateDiffusionFeedback, ctrlVal[3]);
      prevCtrlVal[3] = ctrlVal[3];
    }

  float send = 1.0;
  float drylevel = 1.;
  float dryL, dryR, sendL, sendR;

  float ins_left[48];
  float ins_right[48];
  for (size_t i = 0; i < size; i++)
  {
    // Read Inputs (only stereo in are used)
    ins_left[i] = in[0][i];
    ins_right[i]= in[1][i];
  
    clouds_reverb.set_amount(ctrlVal[1]);
    clouds_reverb.set_diffusion(ctrlVal[3]);
    clouds_reverb.set_time(ctrlVal[2]);// 0.5f + (0.49f * patch_position));
    clouds_reverb.set_input_gain(ctrlVal[0]);
    clouds_reverb.set_lp(0.3f);// : 0.6f);

    clouds_reverb.Process(&ins_left[i], &ins_right[i], 1);
    
    out[0][i] = ins_left[i];
    out[1][i] = ins_right[i];
    // Out 3 and 4 are just wet
    out[2][i] = 0;
    out[3][i] = 0;
  }
}
  
void CloudyReverbApp ::UpdateOled()
{
  //64 bytes should be enough
  static char buf[64];
  const  char* param_names[4] = {"dry:","wet:", "decay:", "diffusion:"};
  m_patch.display.Fill(false);

  test::sprintf(buf,"%s", "Cloudy Reverb");
  m_patch.display.SetCursor(0,0);
  m_patch.display.WriteString(buf, Font_6x8, true);
    

  for (int i=0;i<4;i++)
  {
    //two circuits
    m_patch.display.SetCursor(0, 10+i*10);
    test::sprintf(buf, "%s:%1.5f", param_names[i], prevCtrlVal[i]);
    m_patch.display.WriteString(buf, Font_7x10, true);
  }

  m_patch.display.Update();
}
  
void CloudyReverbApp ::MainLoopCallback()
{
  
}
