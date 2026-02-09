#pragma once
#include "../deps/imgui/imgui.h"

inline void SetupSAGATheme() {
  ImGuiStyle &style = ImGui::GetStyle();

  // 1. Geometry - Softer corners
  style.WindowRounding = 6.0f;
  style.ChildRounding = 6.0f;
  style.FrameRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;

  // 2. Colors - "Deep Space & Gold"
  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
  colors[ImGuiCol_WindowBg] =
      ImVec4(0.10f, 0.10f, 0.13f, 1.00f); // Dark Blue-Grey
  colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.15f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.30f, 0.50f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);

  // Interactive Elements (Gold/Orange Accents)
  colors[ImGuiCol_Button] = ImVec4(0.25f, 0.45f, 0.60f, 0.60f); // Muted Blue
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.55f, 0.70f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.35f, 0.50f, 1.00f);

  colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.35f, 0.50f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.40f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);

  colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.60f, 0.20f, 0.80f); // Gold
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.70f, 0.30f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.60f, 0.20f, 1.00f); // Gold

  colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.55f, 0.70f, 0.80f);
  colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.45f, 0.60f, 1.00f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
}
