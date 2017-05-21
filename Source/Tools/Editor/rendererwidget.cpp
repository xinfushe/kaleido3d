#include "rendererwidget.h"
#include <QTimer>
#include <QResizeEvent>
#include <Core/LogUtil.h>

using namespace k3d;

RendererWidget::RendererWidget(QWidget *parent)
  : QWidget(parent)
  , Timer(nullptr)
{
  Timer = new QTimer;
  Timer->setInterval(16);
  Timer->setTimerType(Qt::PreciseTimer);
}

RendererWidget::~RendererWidget()
{
  if (Timer)
  {
    Timer->stop();
    delete Timer;
    Timer = nullptr;
  }
}

void RendererWidget::init()
{
  RHI = StaticPointerCast<IVkRHI>(ACQUIRE_PLUGIN(RHI_Vulkan));
  RHI->Initialize("Widget", false);
  RHI->Start();
  auto pFactory = RHI->GetFactory();
  DynArray<DeviceRef> Devices;
  pFactory->EnumDevices(Devices);
  Device = Devices[0];

  Queue = Device->CreateCommandQueue(k3d::ECMD_Graphics);

  k3d::SwapChainDesc Desc = { 
    k3d::EPF_RGBA8Unorm_sRGB,
    width(),
    height(),
    2 };
  Swapchain = pFactory->CreateSwapchain(Queue, (void*)winId(), Desc);

  FrameFence = Device->CreateFence();

  QObject::connect(Timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
  Timer->start();
}

void RendererWidget::resizeEvent(QResizeEvent * pEvent)
{
  QWidget::resizeEvent(pEvent);
  auto size = pEvent->size();
  KLOG(Info, RendererWidget, "Resized %d, %d", size.width(), size.height());
  Swapchain->Resize(size.width(), size.height());
}

void RendererWidget::tickRender()
{
  auto cmdBuffer = Queue->ObtainCommandBuffer(k3d::ECMDUsage_OneShot);
  auto presentImage = Swapchain->GetCurrentTexture();
  cmdBuffer->Transition(presentImage, k3d::ERS_Present);
  cmdBuffer->Present(Swapchain, nullptr);
  cmdBuffer->Commit(FrameFence);
}

void RendererWidget::onTimeOut()
{
  tickRender();
}