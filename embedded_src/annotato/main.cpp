#include <stdio.h>
#include <math.h>
#include "MLX90640_API.h"
#include "hand.h"

namespace annotato {
void printPPM(FILE *fp, float temperature[], int nx, int ny, float minVal, float range)
{
    fprintf(fp, "P3\n");
    fprintf(fp, "%d %d\n", nx, ny);
    fprintf(fp, "255\n");
    
    for (int y = 0; y < ny; y++) {
        for (int x = 0; x < nx; x++) {
            float t __attribute((annotate("scalar()"))) = temperature[(nx - 1 - x) + y * nx];
            float pixel __attribute((annotate("scalar()"))) = ((t - minVal) / range);
  
            int r = 255.5f * (
                      (0.375f <= pixel) && (pixel < 0.625f) ? (pixel - 0.375f) / 0.25f :
                      (0.625f <= pixel) && (pixel < 0.875f) ? 1.0f :
                      (0.875f <= pixel)                     ? (1.125f - pixel) / 0.25f : 0.0f);
            int g = 255.5f * (
                      (0.125f <= pixel) && (pixel < 0.375f) ? (pixel - 0.125f) / 0.25f :
                      (0.375f <= pixel) && (pixel < 0.625f) ? 1.0f :
                      (0.625f <= pixel) && (pixel < 0.875f) ? (0.875f - pixel) / 0.25f : 0.0f);  
            int b = 255.5f * (
                                           (pixel < 0.125f) ? (pixel + 0.125f) / 0.25f :
                      (0.125f <= pixel) && (pixel < 0.375f) ? 1.0f :
                      (0.375f <= pixel) && (pixel < 0.625f) ? (0.625f - pixel) / 0.25f : 0.0f);                
        
            fprintf(fp, "%d %d %d ", r, g, b);
        }
        fprintf(fp, "\n");
    }
}
}

using namespace annotato;

struct result {
    int min;
    int max;
};

struct result_f {
    float min;
    float max;
};

struct result_combo {
    struct result_f floatRes;
    struct result intRes;
};

extern "C" struct result_combo run_annotato()
{
    if (MLX90640_ExtractParameters(eeprom))
        return {};
    
    const float ta_shift __attribute((annotate("scalar()"))) = 8.f; //Default shift for MLX90640 in open air
    const float emissivity __attribute((annotate("scalar()"))) = 0.95f;
    const float minRange __attribute((annotate("scalar()"))) = 15.f;
    
    const int nx = 32, ny = 24;
    float temperature[nx*ny] __attribute((annotate("scalar()")));
    
    float Ta __attribute((annotate("scalar()"))) = MLX90640_GetTa(subframe1);
    float tr __attribute((annotate("scalar()"))) = Ta - ta_shift;
    MLX90640_CalculateTo(subframe1, emissivity, tr, temperature);
    
    Ta = MLX90640_GetTa(subframe2);
    tr = Ta - ta_shift;
    MLX90640_CalculateTo(subframe2, emissivity, tr, temperature);
    
    float minVal __attribute((annotate("scalar()"))) = temperature[0];
    float maxVal __attribute((annotate("scalar()"))) = temperature[0];
    for(int i = 1; i < nx * ny; i++) {
        minVal = fmin(minVal, temperature[i]);
        maxVal = fmax(maxVal, temperature[i]);
    }
    float range __attribute((annotate("scalar()"))) = fmax(minRange, maxVal - minVal);
    
    #ifdef __x86_64__
    FILE *fp = fopen("thermalmap.ppm", "w");
    if (fp == NULL)
      return {};
    printPPM(fp, temperature, nx, ny, minVal, range);
    fclose(fp);
    
    fprintf(stdout, "min = %d max = %d\n",
           static_cast<int>(minVal),
           static_cast<int>(maxVal));
    #endif
    
    return {{minVal, maxVal}, {static_cast<int>(minVal), static_cast<int>(maxVal)}};
}

#ifdef __x86_64__
int main(int argc, char** argv)
{
    run_annotato();
    return 0;
}
#endif

