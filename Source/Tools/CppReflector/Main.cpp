#if WIN32
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#endif
#include <iostream>
#include <clang-c/Index.h>
#include "Kaleido3D.h"
#include <KTL/String.hpp>
#include <KTL/DynArray.hpp>
#include "Reflector.h"
using namespace std;

ostream& operator<<(ostream& stream, const CXString& str)
{
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

int main(int argc, const char* argv[])
{
  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
    index,
    "C:/Users/dsotsen/Documents/Project/kaleido3d/Include/Interface/IRHI.h", nullptr, 0,
    nullptr, 0,
    CXTranslationUnit_None);
  k3d::Cursor cur(clang_getTranslationUnitCursor(unit));
  auto childs = cur.GetChildren();
  for (auto c : childs)
  {
    auto string = c.GetName();
    (string);
  }
 /* clang_visitChildren(
    cursor,
    [](CXCursor c, CXCursor parent, CXClientData client_data)
  {
    cout << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
      << clang_getCursorKindSpelling(clang_getCursorKind(c)) << clang_getCursorUSR(c) << "'\n";
    return CXChildVisit_Recurse;
  },
    nullptr);*/
  if (unit == nullptr)
  {
    cerr << "Unable to parse translation unit. Quitting." << endl;
    exit(-1);
  }

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
  return 0;
}