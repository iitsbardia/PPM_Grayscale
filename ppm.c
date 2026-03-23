#include "ppm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* read a PPM image from a file */
int ppm_read(const char *path, ppm_image_t *img) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    //  check the format for "P6"
    char buffer[3];
    fscanf(f, "%2s", buffer);
    if (strcmp(buffer, "P6") != 0) {
        fclose(f);
        return -1;
    }

    //  check for comments and skip them
    int c = fgetc(f);  //  fgetc(): read a single character
    while (c == '\n' || c == '\r' || c == ' ') c = fgetc(f);
    while (c == '#') {
        while (c != '\n') c = fgetc(f);
        c = fgetc(f);
    }
    ungetc(c, f);

    //read image size information
    fscanf(f, "%d %d %d", &img->width, &img->height, &img->max_val);
    fgetc(f);
    int npixels = img->width * img->height * 3;
    img->pixels = malloc(npixels); // alloc memory form image
    if (!img->pixels) { 
        fclose(f); 
        return -1; 
    }
    fread(img->pixels, 1, npixels, f);

    fclose(f);
    return 0;
}

/* write a PPM image to a file */
int ppm_write(const char *path, const ppm_image_t *img) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    // write the header
    fprintf(f, "P6\n%d %d\n%d\n", img->width, img->height, img->max_val);
    // write the pixel data
    fwrite(img->pixels, 1, img->width * img->height * 3, f);

    fclose(f);
    return 0;
}

/* free the memory allocated for the PPM image */
void ppm_free(ppm_image_t *img) {
    free(img->pixels); // free the memory allocated for pixels
    img->pixels = NULL;
}

/* apply grayscale formula to each pixel */
void ppm_grayscale(unsigned char *pixels, int n_pixels) {
    int i;
    for (i = 0; i < n_pixels; i++) {
        // get the RGB values
        unsigned char r = pixels[i * 3 + 0];
        unsigned char g = pixels[i * 3 + 1];
        unsigned char b = pixels[i * 3 + 2];
        // apply the grayscale formula
        unsigned char y = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
        // set the RGB values to the grayscale value
        pixels[i * 3 + 0] = y;
        pixels[i * 3 + 1] = y;
        pixels[i * 3 + 2] = y;
    }
}
