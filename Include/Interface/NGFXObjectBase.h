#ifndef __NGFXObjectBase_h__
#define __NGFXObjectBase_h__
#pragma once

template <bool ThreadSafe>
struct NGFXRefCounted
{
  NGFXRefCounted() {}

  uint64 AddInternalRef()
  {
    return __k3d_intrinsics__::AtomicIncrement(&m_IntRef);
  }

  uint64 ReleaseInternal()
  {
    uint64 c = __k3d_intrinsics__::AtomicDecrement(&m_IntRef);
    if (m_IntRef == 0)
    {
      delete this;
    }
    return c;
  }

  uint64 Release()
  {
    uint64 c = __k3d_intrinsics__::AtomicDecrement(&m_ExtRef);
    if (m_ExtRef == 0)
    {
      ReleaseInternal();
    }
    return c;
  }

  uint64 AddRef()
  {
    return __k3d_intrinsics__::AtomicIncrement(&m_ExtRef);
  }

private:
  uint64 m_IntRef = 1;
  uint64 m_ExtRef = 1;
};

/* Unthread-safe RefCounted*/
template <>
struct NGFXRefCounted<false>
{
  NGFXRefCounted() {}

  uint64 AddInternalRef()
  {
    m_IntRef++;
    return m_IntRef;
  }

  uint64 ReleaseInternal()
  {
    m_IntRef--;
    uint64 c = m_IntRef;
    if (m_IntRef == 0)
    {
      delete this;
    }
    return c;
  }

  uint64 Release()
  {
    m_ExtRef--;
    uint64 c = m_ExtRef;
    if (m_ExtRef == 0)
    {
      ReleaseInternal();
    }
    return c;
  }

  uint64 AddRef()
  {
    m_ExtRef++;
    return m_ExtRef;
  }

private:
  uint64 m_IntRef = 1;
  uint64 m_ExtRef = 1;
};

template <bool ThreadSafe>
struct NGFXNamedObject : public NGFXRefCounted<ThreadSafe>
{
  virtual void SetName(const char *) {}
};

#endif