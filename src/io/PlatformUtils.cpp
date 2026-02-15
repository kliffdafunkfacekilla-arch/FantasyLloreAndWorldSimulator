// clang-format off
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
// clang-format on
#include "../../include/PlatformUtils.hpp"
#include <string>

namespace PlatformUtils {

std::string OpenFileDialog() {
  OPENFILENAMEA ofn;
  char szFile[260] = {0};

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter =
      "Heightmap (PNG, JPG)\0*.png;*.jpg;*.jpeg\0All Files\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileNameA(&ofn) == TRUE) {
    return std::string(ofn.lpstrFile);
  }
  return "";
}

std::string OpenFolderDialog() {
  BROWSEINFOA bi = {0};
  bi.lpszTitle = "Select Obsidian Vault Folder";
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
  LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

  if (pidl != 0) {
    char path[MAX_PATH];
    if (SHGetPathFromIDListA(pidl, path)) {
      IMalloc *imalloc = 0;
      if (SUCCEEDED(SHGetMalloc(&imalloc))) {
        imalloc->Free(pidl);
        imalloc->Release();
      }
      return std::string(path);
    }
  }
  return "";
}

std::string GetExecutablePath() {
  char buffer[MAX_PATH];
  GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  return std::string(buffer).substr(0, pos);
}

} // namespace PlatformUtils
