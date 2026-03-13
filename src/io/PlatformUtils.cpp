
#include "../../include/PlatformUtils.hpp"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

std::string PlatformUtils::OpenFileDialog() {
  char filename[MAX_PATH];
  OPENFILENAMEA ofn;
  ZeroMemory(&filename, sizeof(filename));
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFilter = "Heightmap Images (*.png;*.jpg;*.bmp)\0*.png;*.jpg;*.bmp\0Any File (*.*)\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrTitle = "Select a Heightmap";
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileNameA(&ofn) == TRUE) {
    return std::string(filename);
  }
  return "";
}

std::string PlatformUtils::OpenFolderDialog() {
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

std::string PlatformUtils::GetExecutablePath() {
  char buffer[MAX_PATH];
  GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string path(buffer);
  size_t pos = path.find_last_of("\\/");
  return (pos != std::string::npos) ? path.substr(0, pos) : "";
}

#else

std::string PlatformUtils::OpenFileDialog() { return ""; }
std::string PlatformUtils::OpenFolderDialog() { return ""; }
std::string PlatformUtils::GetExecutablePath() { return "."; }

#endif
