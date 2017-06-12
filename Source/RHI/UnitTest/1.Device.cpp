#include <Kaleido3D.h>
#include <Core/App.h>
#include <Core/Message.h>
#include <Interface/IRHI.h>
#include <Core/Module.h>
#include <RHI/Vulkan/Public/IVkRHI.h>
#include <RHI/Metal/Public/IMetalRHI.h>

using namespace k3d;

class UnitTestRHIDevice : public App
{
public:
	explicit UnitTestRHIDevice(kString const & appName)
		: App(appName, 1920, 1080)
	{}

	UnitTestRHIDevice(kString const & appName, uint32 width, uint32 height)
		: App(appName, width, height)
	{}

	bool OnInit() override;
	void OnDestroy() override;
	void OnProcess(Message & msg) override;

private:

#if K3DPLATFORM_OS_WIN || K3DPLATFORM_OS_ANDROID
	SharedPtr<IVkRHI> m_pRHI;
#else
	SharedPtr<IMetalRHI> m_pRHI;
#endif
	k3d::NGFXDeviceRef m_TestDevice;
};

K3D_APP_MAIN(UnitTestRHIDevice);

using namespace k3d;

bool UnitTestRHIDevice::OnInit()
{
	App::OnInit();
#if K3DPLATFORM_OS_WIN || K3DPLATFORM_OS_ANDROID
	m_pRHI = StaticPointerCast<IVkRHI>(ACQUIRE_PLUGIN(RHI_Vulkan));
	if (m_pRHI)
	{
		m_pRHI->Initialize("UnitTestRHIDevice", false);
		m_pRHI->Start();
    auto pFactory = m_pRHI->GetFactory();
    DynArray<k3d::NGFXDeviceRef> Devices;
    pFactory->EnumDevices(Devices);
		m_TestDevice = Devices[0];
	}
#else
    auto pMtlRHI = StaticPointerCast<IMetalRHI>(ACQUIRE_PLUGIN(RHI_Metal));
    if(pMtlRHI)
    {
        pMtlRHI->Start();
        m_TestDevice = pMtlRHI->GetPrimaryDevice();
    }
#endif
	return true;
}

void UnitTestRHIDevice::OnDestroy()
{
	App::OnDestroy();
}

void UnitTestRHIDevice::OnProcess(Message & msg)
{
}
