
#include "dummy_app.h"
#include "daisysp.h"
#include "daisy_patch.h"
namespace test {
#include "printf.h"
};


using namespace daisy;
using namespace daisysp;

DummyApp::DummyApp(DaisyPatch& patch, const std::string& title)
  :m_patch(patch),
   m_title(title)
{
  for (int i=0;i<4;i++)
  {
    m_ctrlVal[i] = 0.f;
  }
}

DummyApp::~DummyApp()
{
}
  
void DummyApp::Init()
{
}
  
void DummyApp::Exit()
{
}
  
void DummyApp::AudioTickCallback(float ctrlVal[4], float **in, float **out, size_t size)
{
  for (int i=0;i<4;i++)
  {
    m_ctrlVal[i] = ctrlVal[i];
  }
  
  for (size_t i = 0; i < size; i++)
  {
      out[0][i] = 0;
      out[1][i] = 0;
      out[2][i] = 0;
      out[3][i] = 0;
  }
}
  
void DummyApp::UpdateOled()
{
  const char* param_names[4] = {"Ctrl0:","Ctrl1:", "Ctrl2:", "Ctrl3:"};
  
  //64 bytes should be enough
  static char buf[64];
  m_patch.display.Fill(false);

  test::sprintf(buf,"%s", m_title.c_str());
  m_patch.display.SetCursor(0,0);
  m_patch.display.WriteString(buf, Font_7x10, true);
 
 
  for (int i=0;i<4;i++)
  {
    m_patch.display.SetCursor(0, 10+i*10);
    test::sprintf(buf, "%s:%1.5f", param_names[i], m_ctrlVal[i]);
    m_patch.display.WriteString(buf, Font_7x10, true);
  }
  m_patch.display.Update();
}

  
  
void DummyApp::MainLoopCallback()
{
  
}
