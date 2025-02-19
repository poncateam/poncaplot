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
            std::transform(texture, texture + w * h * 4,
                           buffer,
                           [](float in) -> char {
                               return char(std::floor(in * 255.f));
                           });
            stbi_write_png(filename.c_str(), w, h,
                           4, buffer, w * 4);
            stbi_image_free(buffer);
        }
    };
}

