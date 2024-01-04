#include "pch.h"
#include "mandelbrot_parameters.h"

const std::string mandelbrot_parameter_info::MANDELBROT_FRAGMENT_SHADER =
"#version 450                                                                            \n"
"layout(location = 0) in vec3 inputColor;                                                \n"
"layout(location = 0) out vec4 outputColor;                                              \n"
"                                                                                        \n"
"layout(push_constant) uniform constants                                                 \n"
"{                                                                                       \n"
"    float top;                                                                          \n"
"    float left;                                                                         \n"
"    float right;                                                                        \n"
"    float bottom;                                                                       \n"
"    float surface_width;                                                                \n"
"    float surface_height;                                                               \n"
"    float bailout_radius;                                                               \n"
"    uint max_iterations;                                                                \n"
"    uint fill_color;                                                                    \n"
"    float gradient_period_factor;                                                       \n"
"    uint gradient_length;                                                               \n"
"    uint gradient[21];                                                                  \n"
"} PushConstants;                                                                        \n"
"                                                                                        \n"
"void main()                                                                             \n"
"{                                                                                       \n"
"    float top = PushConstants.top;                                                      \n"
"    float left = PushConstants.left;                                                    \n"
"    float right = PushConstants.right;                                                  \n"
"    float bottom = PushConstants.bottom;                                                \n"
"    float surface_width = PushConstants.surface_width;                                  \n"
"    float surface_height = PushConstants.surface_height;                                \n"
"    float surface_x = gl_FragCoord.x;                                                   \n"
"    float surface_y = gl_FragCoord.y;                                                   \n"
"                                                                                        \n"
"    float cr = mix(left, right, surface_x/surface_width);                               \n"
"    float ci = mix(top, bottom, surface_y/surface_height);                              \n"
"    float zr = 0.0f;                                                                    \n"
"    float zi = 0.0f;                                                                    \n"
"                                                                                        \n"
"    uint max_iteration = PushConstants.max_iterations;                                  \n"
"    float bailout_radius = PushConstants.bailout_radius;                                \n"
"    float m1 = 0.0f;                                                                    \n"
"    float m2 = 0.0f;                                                                    \n"
"    uint iteration = 0;                                                                 \n"
"                                                                                        \n"
"    // Count the number of iterations until z exceeds the bailout radius.               \n"
"    // m1 is the square magnitude of z on the iteration just before bailout.            \n"
"    // m2 is the square magnitude of z on the iteration of bailout.                     \n"
"    for (uint i = 0 ; i < max_iteration ; i++)                                          \n"
"    {                                                                                   \n"
"        if (m2 < bailout_radius)                                                        \n"
"        {                                                                               \n"
"            float zr2 = zr*zr;                                                          \n"
"            float zi2 = zi*zi;                                                          \n"
"                                                                                        \n"
"            float zr_next = zr2 - zi2 + cr;                                             \n"
"            float zi_next = 2*zr*zi + ci;                                               \n"
"            zr = zr_next;                                                               \n"
"            zi = zi_next;                                                               \n"
"            m1 = m2;                                                                    \n"
"            m2 = zr2 + zi2;                                                             \n"
"            iteration = iteration + 1;                                                  \n"
"        }                                                                               \n"
"    }                                                                                   \n"
"                                                                                        \n"
"    if (iteration < max_iteration)                                                      \n"
"    {                                                                                   \n"
"        // Approach:                                                                    \n"
"        // Linearly interpolate the real-valued bailout iteration                       \n"
"        // based on m1, the bailout radius, and m2.                                     \n"
"                                                                                        \n"
"        // Interpolating over log(m1), log(bailout_radius), and log(m2)                 \n"
"        // gives us a smoother gradient.                                                \n"
"                                                                                        \n"
"        // delta = The difference between the real-valued iteration                     \n"
"        //         and the integer iteration.                                           \n"
"                                                                                        \n"
"        float invm1 = 1.0f / m1;                                                        \n"
"        float delta = 1.0f - log(bailout_radius * invm1) / log(m2 * invm1);             \n"
"                                                                                        \n"
"        uint length = PushConstants.gradient_length;                                    \n"
"                                                                                        \n"
"        // The gradient repeats every P iterations.                                     \n"
"        // This is known as the gradient period.                                        \n"
"        // Below is a silly attempt to scale P according to                             \n"
"        // the iteration number, so that colors don't cycle too fast                    \n"
"        // the closer we get to the mandelbrot edge.                                    \n"
"                                                                                        \n"
"        float F = PushConstants.gradient_period_factor;                                 \n"
"        float T = float(iteration) - delta;                                             \n"
"        float M = float(max_iteration);                                                 \n"
"        float L = float(length);                                                        \n"
"        float P = mix(L, M*F, (T-1.0f)/(M-1.0f));                                       \n"
"        float K = floor(T/P);                                                           \n"
"                                                                                        \n"
"        // Now calculate where the real-valued iteration T                              \n"
"        // lies within the gradient.                                                    \n"
"        float t_mod_p = T - K*P;                                                        \n"
"        float hue = (t_mod_p / P) * L;                                                  \n"
"        float epsilon = hue - floor(hue);                                               \n"
"                                                                                        \n"
"        int c1_index = int(floor(hue));                                                 \n"
"        int c2_index = int(floor(hue + 1)) % int(length);                               \n"
"        uint c1 = PushConstants.gradient[c1_index];                                     \n"
"        uint c2 = PushConstants.gradient[c2_index];                                     \n"
"                                                                                        \n"
"        uint c1r_hex = (c1 >> 16) & 0xFF;                                               \n"
"        uint c1g_hex = (c1 >> 8) & 0xFF;                                                \n"
"        uint c1b_hex = (c1) & 0xFF;                                                     \n"
"                                                                                        \n"
"        uint c2r_hex = (c2 >> 16) & 0xFF;                                               \n"
"        uint c2g_hex = (c2 >> 8) & 0xFF;                                                \n"
"        uint c2b_hex = (c2) & 0xFF;                                                     \n"
"                                                                                        \n"
"        float r1 = float(c1r_hex) / 255.0f;                                             \n"
"        float g1 = float(c1g_hex) / 255.0f;                                             \n"
"        float b1 = float(c1b_hex) / 255.0f;                                             \n"
"        float r2 = float(c2r_hex) / 255.0f;                                             \n"
"        float g2 = float(c2g_hex) / 255.0f;                                             \n"
"        float b2 = float(c2b_hex) / 255.0f;                                             \n"
"                                                                                        \n"
"        float r = r1 + (r2 - r1) * epsilon;                                             \n"
"        float g = g1 + (g2 - g1) * epsilon;                                             \n"
"        float b = b1 + (b2 - b1) * epsilon;                                             \n"
"                                                                                        \n"
"        outputColor = vec4(r, g, b, 1.0f);                                              \n"
"    }                                                                                   \n"
"    else                                                                                \n"
"    {                                                                                   \n"
"        uint fill_color = PushConstants.fill_color;                                     \n"
"        uint ired = (fill_color >> 16) & 0xFF;                                          \n"
"        uint igreen = (fill_color >> 8) & 0xFF;                                         \n"
"        uint iblue = (fill_color) & 0xFF;                                               \n"
"                                                                                        \n"
"        float r_out = float(ired) / 255.0f;                                             \n"
"        float g_out = float(igreen) / 255.0f;                                           \n"
"        float b_out = float(iblue) / 255.0f;                                            \n"
"                                                                                        \n"
"        outputColor = vec4(r_out, g_out, b_out, 1.0f);                                  \n"
"    }                                                                                   \n"
"}                                                                                       \n"
"                                                                                        \n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
;

