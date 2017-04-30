#include <clang-c/Index.h>
#include "Kaleido3D.h"
#include <KTL/String.hpp>
#include <KTL/DynArray.hpp>
#include "Reflector.h"

namespace k3d
{

String cxStr(CXString str)
{
  String _Str(clang_getCString(str));
  clang_disposeString(str);
  return _Str;
}

Cursor::Cursor(const CXCursor & c)
  : m_Handle(c)
{
}

String Cursor::GetName() const
{
  return cxStr(clang_getCursorDisplayName(m_Handle));
}

Cursor::List Cursor::GetChildren()
{
  Cursor::List list;

  auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data)
  {
    auto container = static_cast<List *>(data);

    container->Append(cursor);

    if (cursor.kind == CXCursor_LastPreprocessing)
      return CXChildVisit_Break;

    return CXChildVisit_Continue;
  };

  clang_visitChildren(m_Handle, visitor, &list);
  return list;
}


}