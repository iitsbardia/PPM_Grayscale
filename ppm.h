#ifndef PPM_H
#define PPM_H

#include <stdint.h>

typedef struct {
    int      width;
    int      height;
    int      max_val;
    uint8_t *pixels;   // RGB
} ppm_image_t;

int  ppm_read(const char *path, ppm_image_t *img);
int  ppm_write(const char *path, const ppm_image_t *img);
void ppm_free(ppm_image_t *img);
void ppm_grayscale(uint8_t *pixels, int n_pixels);

#endif
