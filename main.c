#include <stdio.h>
#include <stdlib.h>

#include "filters.h"

int main(int argc, char *argv[]) 
{
  if (argc < 3) {
     fprintf(stderr, "Usage:\n   %s <script_file> <image1> [image2 .... ]\n", argv[0]);
     exit(EXIT_FAILURE);
  }
  for (int i=2; i<argc; i++) {
     fprintf(stderr, "Processing %s\n", argv[i]); 
     image_t layer = read_JPEG_file(argv[i]);
     printf("w=%d h=%d stride=%d\n", layer.width, layer.height, layer.stride);
     if (!layer.rc) continue;
     rect_t zone = {0, 0, layer.width/2, layer.height};
     printf("BW\n");
    // bw(layer, zone);
     printf("Gauss\n");
     //gauss(layer, 20, 0.0, zone);
     //unsharp(layer, 20, 1.05, zone);
     //contrast(layer, 20, zone);
     //brightness(layer, -80, zone);
     //gamma_correction(layer, 2.0f, zone);
     //invert(layer, zone);
     //temperature(layer, 20, zone);
     //tint(layer, 20, zone);
     //exposure(layer, 1.1f, zone);
     // kelvin_temperature(layer,13000.0f, 1.0f, zone);
     adjust_color_saturation(layer, 0.3f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, zone);
     //saturation(layer, 1.0f, zone);
     //vibrance(layer, 0.5f, zone);
     //colorize(layer, vec3_init(COLOR_MAX, 0, 0), 0.5, 0.0, zone);
     write_JPEG_file("output.jpg", layer.image, layer.width, layer.height, 90);
     layer_free(layer);
  }
  return EXIT_SUCCESS;
}
