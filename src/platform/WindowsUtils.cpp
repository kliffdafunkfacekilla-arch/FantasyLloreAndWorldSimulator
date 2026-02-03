#undef WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../include/PlatformUtils.hpp"
#include <commdlg.h>
#include <iostream>
#include <windows.h>


std::string OpenFileDialog() {
  OPENFILENAMEA ofn;
  char szFile[260] = {0};
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Images\0*.png;*.jpg;*.bmp\0All\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileNameA(&ofn) == TRUE) {
    return std::string(szFile);
  }
  return "";
}
