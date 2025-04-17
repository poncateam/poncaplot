#include "appBase.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_MSC_VER)
#  pragma warning (disable: 4505) // don't warn about dead code in stb_image.h
#elif defined(__GNUC__)
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#include "stb_image_write.h"

#include <algorithm> // transform
#include <cmath> // floor


namespace poncaplot{
    void write_image(int w, int h, float *texture, const std::string &filename) {
        {
            auto buffer = new char[w * h * 4];

            // Converts a linear value in the range [0, 1] to an sRGB value in
            // the range [0, 255].
            // Source : https://github.com/PetterS/opencv_srgb_gamma/blob/master/srgb.h
            std::transform(texture, texture + w * h * 4,
                           buffer,
                           [](float linear) -> char {
                               float srgb;
                               if (linear <= 0.0031308f) {
                                   srgb = linear * 12.92f;
                               } else {
                                   srgb = 1.055f * std::powf(linear, 1.0f / 2.4f) - 0.055f;
                               }
                               return srgb * 255.f;
                           });
            stbi_write_png(filename.c_str(), w, h,
                           4, buffer, w * 4);
            stbi_image_free(buffer);
        }
    };
}

