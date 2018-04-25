//-----------------------------------------------------------------------------
// USER IMPLEMENTATION
// This file contains compile-time options for ImGui.
// Other options (memory allocation overrides, callbacks, etc.) can be set at runtime via the ImGuiIO structure - ImGui::GetIO().
//-----------------------------------------------------------------------------

#pragma once

#include <ospcommon/vec.h>

//---- Define assertion handler. Defaults to calling assert().
//#define IM_ASSERT(_EXPR)  MyAssert(_EXPR)

//---- Define attributes of all API symbols declarations, e.g. for DLL under Windows.
#ifdef _WIN32
#  ifdef ospray_imgui_EXPORTS
#    define IMGUI_API __declspec(dllexport)
#  else
#    define IMGUI_API __declspec(dllimport)
#  endif
#else
#  define IMGUI_API
#endif

//---- Don't define obsolete functions names. Consider enabling from time to time or when updating to reduce like hood of using already obsolete function/names
//#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

//---- Include imgui_user.h at the end of imgui.h
//#define IMGUI_INCLUDE_IMGUI_USER_H

//---- Don't implement default handlers for Windows (so as not to link with OpenClipboard() and others Win32 functions)
//#define IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCTIONS
//#define IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS

//---- Don't implement test window functionality (ShowTestWindow()/ShowStyleEditor()/ShowUserGuide() methods will be empty)
//---- It is very strongly recommended to NOT disable the test windows. Please read the comment at the top of imgui_demo.cpp to learn why.
//#define IMGUI_DISABLE_TEST_WINDOWS

//---- Don't implement ImFormatString(), ImFormatStringV() so you can reimplement them yourself.
//#define IMGUI_DISABLE_FORMAT_STRING_FUNCTIONS

//---- Pack colors to BGRA instead of RGBA (remove need to post process vertex buffer in back ends)
//#define IMGUI_USE_BGRA_PACKED_COLOR

//---- Implement STB libraries in a namespace to avoid linkage conflicts
//#define IMGUI_STB_NAMESPACE     ImGuiStb

//---- Define constructor and implicit cast operators to convert back<>forth from your math types and ImVec2/ImVec4.
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const ospcommon::vec2f& f) { x = f.x; y = f.y; }                       \
        operator ospcommon::vec2f() const { return ospcommon::vec2f(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const ospcommon::vec4f& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator ospcommon::vec4f() const { return ospcommon::vec4f(x,y,z,w); }

//---- Use 32-bit vertex indices (instead of default: 16-bit) to allow meshes with more than 64K vertices
//#define ImDrawIdx unsigned int

//---- Tip: You can add extra functions within the ImGui:: namespace, here or in your own headers files.
//---- e.g. create variants of the ImGui::Value() helper for your low-level math types, or your own widgets/helpers.
/*
namespace ImGui
{
    void    Value(const char* prefix, const MyMatrix44& v, const char* float_format = NULL);
}
*/

