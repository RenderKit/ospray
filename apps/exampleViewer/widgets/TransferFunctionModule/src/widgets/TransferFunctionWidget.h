#pragma once

#include <string>
#include <memory>
#include <vector>
#include <array>
#include <functional>

#include <GLFW/glfw3.h>
#ifdef _WIN32
# undef APIENTRY
# define GLFW_EXPOSE_NATIVE_WIN32
# define GLFW_EXPOSE_NATIVE_WGL
# include <GLFW/glfw3native.h>
#endif

#include "common/math.h"
#include "TransferFunctionModule.h"

namespace tfn {
  namespace tfn_widget {
    class TFN_MODULE_INTERFACE TransferFunctionWidget
    {
    public:
      TransferFunctionWidget(const std::function<size_t()>& sample_num_getter,
			     const std::function<void(const std::vector<float>&,
						      const std::vector<float>&)>&
			     sample_value_setter);	
      ~TransferFunctionWidget();
      TransferFunctionWidget(const TransferFunctionWidget &core);
      TransferFunctionWidget& operator=(const TransferFunctionWidget &core);
      /* Draw the transfer function editor widget, returns true if the
       * transfer function changed
       */
      bool drawUI();
      /* Render the transfer function to a 1D texture that can
       * be applied to volume data
       */
      void render();
      // Load the transfer function in the file passed and set it active
      void load(const std::string &fileName);
      // Save the current transfer function out to the file
      void save(const std::string &fileName) const;
    private:
      // The indices of the transfer function color presets available
      enum ColorMap {
	JET,
	ICE_FIRE,
	COOL_WARM,
	BLUE_RED,
	GRAYSCALE,
      };
      
      // This defines the control point class
      struct ColorPoint {
	float p; // location of the control point [0, 1]
	float r, g, b;
	ColorPoint() {};
        ColorPoint(const float cp, 
		   const float cr, 
		   const float cg, 
		   const float cb):
	  p(cp), r(cr), g(cg), b(cb){}
        ColorPoint(const ColorPoint& c) : p(c.p), r(c.r), g(c.g), b(c.b) {}
	ColorPoint& operator=(const ColorPoint &c) {
	  if (this == &c) { return *this; }
	  p = c.p; r = c.r; g = c.g; b = c.b;
	  return *this;
	}
	// This function gives Hex color for ImGui
	unsigned long GetHex() {
	  return (0xff << 24) +
	    ((static_cast<uint8_t>(b * 255.f) & 0xff) << 16) +
	    ((static_cast<uint8_t>(g * 255.f) & 0xff) << 8) +
	    ((static_cast<uint8_t>(r * 255.f) & 0xff));
	}
      };
      struct OpacityPoint {
	OpacityPoint() {};
        OpacityPoint(const float cp, const float ca) : p(cp), a(ca){}
        OpacityPoint(const OpacityPoint& c) : p(c.p), a(c.a) {}
	OpacityPoint& operator=(const OpacityPoint &c) {
	  if (this == &c) { return *this; }
	  p = c.p; a = c.a;
	  return *this;
	}
	float p; // location of the control point [0, 1]
	float a;
      };

      // TODO
      // This MAYBE the correct way of doing this
      struct TFN {
	std::vector<ColorPoint>   colors;
	std::vector<OpacityPoint> opacity;
	tfn::TransferFunction reader;
      };

    private:
      // function handler
      std::function<size_t()> tfn_sample_num_get;
      std::function<void(const std::vector<float>,
			 const std::vector<float>)> tfn_sample_set;
      // The list of avaliable transfer functions, both built-in and loaded
      std::vector<tfn::TransferFunction> tfn_readers;
      // Current TFN
      std::vector<bool> tfn_editable;
      std::vector<std::vector<ColorPoint>>   tfn_c_list;
      std::vector<std::vector<OpacityPoint>> tfn_o_list;
      std::vector<ColorPoint>*   tfn_c;
      std::vector<OpacityPoint>* tfn_o;
      bool tfn_edit;
      std::vector<std::string> tfn_names;
      // interpolated trasnfer function size
      int tfn_w = 256;
      int tfn_h = 1;      
      // The selected transfer function being shown
      int  tfn_selection;      
      // Track if the function changed and must be re-uploaded.
      // We start by marking it changed to upload the initial palette
      bool   tfn_changed;
      // The 2d palette texture on the GPU for displaying the color map
      // in the UI.
      GLuint tfn_palette;
      // The filename input text buffer
      std::vector<char> tfn_text_buffer;
      // Local functions
      void LoadDefaultMap();
      void SetTFNSelection(int);
    };
  }//::tfn_widget
}//::tfn

