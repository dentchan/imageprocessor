/**
 *  Fast Local Color Transfer via Dominant Colors Mapping, 2010
 *
 *  Weiming Dong, Guanbo Bao (1),(2), Xiaopeng Zhang (1), Jean-Claude Paul (2)
 * 
 * (1) LIAMA-NLPR-Digital Content Technology Research Center, CAS Institute of Automation, China, (2) INRIA, France
 */
#include "dong2010.h"
#include "color_conversion.h"
#include "permgen.h"
#include "kmeans.h"
#include "shapes.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>

#define DOMINANT_COLORS_NO 8
#define DELTA 15.0f
#define DELTA_S 0.4f
#define DELTA_C 1.0f

static float color_distance(vec3 src, vec3 dst);

void apply_color_dong2010(layer_t source, layer_t dest, rect_t source_zone, rect_t dest_zone) {
 

   /**
    * First, compute dominant colors in both source and destination.
    */
    color_info_t source_dominant_colors[DOMINANT_COLORS_NO];
    color_info_t dest_dominant_colors[DOMINANT_COLORS_NO];
    kmeans(source, source.zone, DOMINANT_COLORS_NO, source_dominant_colors);
    int *segmentation = kmeans(dest, dest.zone, DOMINANT_COLORS_NO, dest_dominant_colors);

    printf("source dominant colors\n");
    for(int j=0; j<DOMINANT_COLORS_NO; j++) vec3_info(source_dominant_colors[j].color);
    printf("dest dominant colors\n");
    for(int j=0; j<DOMINANT_COLORS_NO; j++) vec3_info(dest_dominant_colors[j].color);
    printf("source variance\n");
    for(int j=0; j<DOMINANT_COLORS_NO; j++) vec3_info(source_dominant_colors[j].variance);
    printf("dest variance\n");
    for(int j=0; j<DOMINANT_COLORS_NO; j++) vec3_info(dest_dominant_colors[j].variance);

    /**
     * Find best mapping from source to destination.
     */
    int mapping[DOMINANT_COLORS_NO];
    
    permgen_t p = permgen_init(DOMINANT_COLORS_NO);
    float best_error = FLT_MAX;
    while(permgen_has_more(&p)) {
       int *perm = permgen_get_next(&p);
       float error = 0.0f;
       for(int i=0; i<DOMINANT_COLORS_NO; i++) {
          error += color_distance(source_dominant_colors[perm[i]].color, dest_dominant_colors[i].color);
       }
       if( error < best_error ) {
          best_error = error;
          /**
           * Preserve the best mapping so far.
           */
          for(int i=0; i<DOMINANT_COLORS_NO; i++) mapping[i] = perm[i];
       }
    }

   color_t *image = dest.image;
   int width = dest.width;
   int height = dest.height;
   int color_components = dest.color_components; 
  
  rect_t zone = dest_zone;
  if (zone.maxy==0) return;

  if (zone.minx<0) zone.minx=0;
  if (zone.miny<0) zone.miny=0;
  if (zone.maxx>=width) zone.maxx=width;
  if (zone.maxy>=height) zone.maxy=height;

  float zx = zone.maxx - zone.minx;
  float zy = zone.maxy - zone.miny;
//#pragma omp parallel for
  for(int y=zone.miny; y<zone.maxy; y++)  {
    printf("y=%d\n", y);
    for(int x=zone.minx; x<zone.maxx; x++) {
       int idx = y*width*color_components + x*color_components;
       color_t rc, gc, bc;
       rc = image[idx];
       gc = image[idx+1];
       bc = image[idx+2];

       vec3 rgb = vec3_init(rc/255.0f, gc/255.0f, bc/255.0f);    
       vec3 LMS = RGBtoLMS(rgb);
       vec3 Lab = LMStoLab(LMS);
       float Psum = 0.0f;
       float jDxy[DOMINANT_COLORS_NO];
       for(int jj=0; jj<DOMINANT_COLORS_NO; jj++) jDxy[jj]=0.0f;
       /** Normalize x, y into 0..1 */
       float fx = x / zx;
       float fy = y / zy;
      
       for(int xx=x-1; xx<=x+1; xx++) for(int yy=y-1; yy<=y+1; yy++) {
          //printf("xx=%d yy=%d\n", xx, yy);
          if(xx<0 || yy<0) continue;
          if(xx>=width || yy>=height) continue;
          //if(x==xx && y==yy) continue;
          //int k = yy*width*color_components + xx*color_components;
          /**
           * Neighbour color, region, region color mean, region geometric center.
           */
          //vec3 color_Nxy = vec3_init(image[k]/(float)COLOR_MAX, image[k+1]/(float)COLOR_MAX, image[k+2]/(float)COLOR_MAX);
          int Rj = segmentation[yy*width +xx]; 
          vec3 mu_j = dest_dominant_colors[Rj].color;
          vec3 center = dest_dominant_colors[Rj].center;
          jDxy[Rj] += exp(-powf(fx-center.x,2.0f)-powf(fy-center.y,2)/DELTA_S)*exp(-vec3_dist(Lab, mu_j)/DELTA_C);
          
       }
     
       for(int j=0; j<DOMINANT_COLORS_NO; j++) {
         Psum += jDxy[j];
       }
       vec3 Io = vec3_init(0, 0, 0);
       for(int j=0; j<DOMINANT_COLORS_NO; j++) {
          vec3 sigma_src = source_dominant_colors[mapping[j]].variance;
          vec3 sigma_dst = dest_dominant_colors[j].variance;
          if (sigma_src.x < 0 || sigma_src.y < 0 || sigma_src.z < 0) continue;
          if (sigma_dst.x < 0 || sigma_dst.y < 0 || sigma_dst.z < 0) continue;
          //Pxy[j] = 1.0f/DOMINANT_COLORS_NO;
          float Pxy = jDxy[j] / Psum;
          vec3 delta = vec3_init(
             Pxy * ((sigma_src.x/sigma_dst.x)*(Lab.x - dest_dominant_colors[j].color.x) + source_dominant_colors[mapping[j]].color.x),
             Pxy * ((sigma_src.y/sigma_dst.y)*(Lab.y - dest_dominant_colors[j].color.y) + source_dominant_colors[mapping[j]].color.y),
             Pxy * ((sigma_src.z/sigma_dst.z)*(Lab.z - dest_dominant_colors[j].color.z) + source_dominant_colors[mapping[j]].color.z)
          );


          Io = vec3_add(Io, delta);
          if(y==0 && x==0)  {
            printf("x===0 y===0\n");
            vec3_info(delta);
            printf("dx = - %f + %f\n",  dest_dominant_colors[j].color.x, source_dominant_colors[mapping[j]].color.x);
            printf("dy = - %f + %f\n",  dest_dominant_colors[j].color.y, source_dominant_colors[mapping[j]].color.y);
            printf("dz = - %f + %f\n",  dest_dominant_colors[j].color.z, source_dominant_colors[mapping[j]].color.z);
            //vec3_info(Io);
            printf("===========\n");
          }

       }
             
       vec3 backToRGB = LMStoRGB(LabtoLMS(Io));
       image[idx] = COLOR_MAX * backToRGB.x;
       image[idx+1] = COLOR_MAX * backToRGB.y;
       image[idx+2] = COLOR_MAX * backToRGB.z;

       if (backToRGB.x > 1.0f) image[idx] = COLOR_MAX;
       if (backToRGB.y > 1.0f) image[idx+1] = COLOR_MAX;
       if (backToRGB.z > 1.0f) image[idx+2] = COLOR_MAX;
       if (backToRGB.x < 0.0f) image[idx] = 0;
       if (backToRGB.y < 0.0f) image[idx+1] = 0;
       if (backToRGB.z < 0.0f) image[idx+2] = 0;
 
     }
   }

   for(int j=0; j<DOMINANT_COLORS_NO; j++) {
     rect_t shape_src = {j*20, 0, j*20+19, 20};
     rect_t shape_dst = {j*20, 40, j*20+19, 60};
     draw_filled_rect(dest, LMStoRGB(LabtoLMS(dest_dominant_colors[j].color)), 1.0f, shape_src, blend_normal);
     draw_filled_rect(dest, LMStoRGB(LabtoLMS(source_dominant_colors[mapping[j]].color)), 1.0f, shape_dst, blend_normal);
   }
}

static float color_distance(vec3 src, vec3 dst)
{
   vec3 src_Lab = LMStoLab(RGBtoLMS(src));
   vec3 dst_Lab = LMStoLab(RGBtoLMS(dst));
   float dx2 = pow(src_Lab.x - dst_Lab.x, 2);
   float dy2 = pow(src_Lab.y - dst_Lab.y, 2);
   float dz2 = pow(src_Lab.z - dst_Lab.z, 2);
   return 1 - exp(-sqrt(dx2+dy2+dz2)/DELTA);
}
