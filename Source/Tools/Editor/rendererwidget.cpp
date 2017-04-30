#include "rendererwidget.h"

using namespace k3d;
using namespace rhi;

RendererWidget::RendererWidget(QWidget *parent)
    : QWidget(parent)
{
}

void RendererWidget::init()
{
  RHI = StaticPointerCast<IVkRHI>(ACQUIRE_PLUGIN(RHI_Vulkan));
  RHI->Initialize("Widget", false);
  RHI->Start();
  auto pFactory = RHI->GetFactory();
  DynArray<rhi::DeviceRef> Devices;
  pFactory->EnumDevices(Devices);
  Device = Devices[0];
}
