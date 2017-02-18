/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2007-2011 Martin Renold <martinxyz@gmx.ch>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#if MYPAINT_CONFIG_USE_GLIB
#include <glib.h>
#include <glib/mypaint-brush.h>
#endif

#include "mypaint-brush.h"

#include "mypaint-brush-settings.h"
#include "mypaint-mapping.h"
#include "helpers.h"
#include "rng-double.h"

#ifdef HAVE_JSON_C
// Allow the C99 define from json.h
#undef TRUE
#undef FALSE
#include <json.h>
#endif // HAVE_JSON_C

#ifdef _MSC_VER
#if _MSC_VER < 1700     // Visual Studio 2012 and later has isfinite and roundf
  #include <float.h>
  static inline int    isfinite(double x) { return _finite(x); }
  static inline float  roundf  (float  x) { return x >= 0.0f ? floorf(x + 0.5f) : ceilf(x - 0.5f); }
#endif
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ACTUAL_RADIUS_MIN 0.2
#define ACTUAL_RADIUS_MAX 1000 // safety guard against radius like 1e20 and against rendering overload with unexpected brush dynamics
#define SPD 16777216
#define WIDTH 36

float *RGBSPD[SPD];
float smudge_buckets[256][9];


float T_MATRIX[3][36] = {{5.47813E-05, 0.000184722, 0.000935514, 0.003096265, 0.009507714, 0.017351596, 0.022073595, 0.016353161, 0.002002407, -0.016177731, -0.033929391, -0.046158952, -0.06381706, -0.083911194, -0.091832385, -0.08258148, -0.052950086, -0.012727224, 0.037413037, 0.091701812, 0.147964686, 0.181542886, 0.210684154, 0.210058081, 0.181312094, 0.132064724, 0.093723787, 0.057159281, 0.033469657, 0.018235464, 0.009298756, 0.004023687, 0.002068643, 0.00109484, 0.000454231, 0.000255925},
{-4.65552E-05, -0.000157894, -0.000806935, -0.002707449, -0.008477628, -0.016058258, -0.02200529, -0.020027434, -0.011137726, 0.003784809, 0.022138944, 0.038965605, 0.063361718, 0.095981626, 0.126280277, 0.148575844, 0.149044804, 0.14239936, 0.122084916, 0.09544734, 0.067421931, 0.035691251, 0.01313278, -0.002384996, -0.009409573, -0.009888983, -0.008379513, -0.005606153, -0.003444663, -0.001921041, -0.000995333, -0.000435322, -0.000224537, -0.000118838, -4.93038E-05, -2.77789E-05},
{0.00032594, 0.001107914, 0.005677477, 0.01918448, 0.060978641, 0.121348231, 0.184875618, 0.208804428, 0.197318551, 0.147233899, 0.091819086, 0.046485543, 0.022982618, 0.00665036, -0.005816014, -0.012450334, -0.015524259, -0.016712927, -0.01570093, -0.013647887, -0.011317812, -0.008077223, -0.005863171, -0.003943485, -0.002490472, -0.001440876, -0.000852895, -0.000458929, -0.000248389, -0.000129773, -6.41985E-05, -2.71982E-05, -1.38913E-05, -7.35203E-06, -3.05024E-06, -1.71858E-06}};




/* The Brush class stores two things:
   b) settings: constant during a stroke (eg. size, spacing, dynamics, color selected by the user)
   a) states: modified during a stroke (eg. speed, smudge colors, time/distance to next dab, position filter states)

   FIXME: Actually those are two orthogonal things. Should separate them:
          a) brush settings class that is saved/loaded/selected  (without states)
          b) brush core class to draw the dabs (using an instance of the above)

   In python, there are two kinds of instances from this: a "global
   brush" which does the cursor tracking, and the "brushlist" where
   the states are ignored. When a brush is selected, its settings are
   copied into the global one, leaving the state intact.
 */


/**
  * MyPaintBrush:
  *
  * The MyPaint brush engine class.
  */
struct MyPaintBrush {

    gboolean print_inputs; // debug menu
    // for stroke splitting (undo/redo)
    double stroke_total_painting_time;
    double stroke_current_idling_time;

    // the states (get_state, set_state, reset) that change during a stroke
    float states[MYPAINT_BRUSH_STATES_COUNT];
    double random_input;
    float skip;
    float skip_last_x;
    float skip_last_y;
    float skipped_dtime;
    RngDouble * rng;

    // Those mappings describe how to calculate the current value for each setting.
    // Most of settings will be constant (eg. only their base_value is used).
    MyPaintMapping * settings[MYPAINT_BRUSH_SETTINGS_COUNT];

    // the current value of all settings (calculated using the current state)
    float settings_value[MYPAINT_BRUSH_SETTINGS_COUNT];

    // see also brushsettings.py

    // cached calculation results
    float speed_mapping_gamma[2];
    float speed_mapping_m[2];
    float speed_mapping_q[2];

    gboolean reset_requested;
#ifdef HAVE_JSON_C
    json_object *brush_json;
#endif
    int refcount;
};


void settings_base_values_have_changed (MyPaintBrush *self);

#include "glib/mypaint-brush.c"

/**
  * mypaint_brush_new:
  *
  * Create a new MyPaint brush engine instance.
  * Initial reference count is 1. Release references using mypaint_brush_unref()
  */
MyPaintBrush *
mypaint_brush_new(void)
{
    MyPaintBrush *self = (MyPaintBrush *)malloc(sizeof(MyPaintBrush));

    self->refcount = 1;
    int i=0;
    for (i=0; i<MYPAINT_BRUSH_SETTINGS_COUNT; i++) {
      self->settings[i] = mypaint_mapping_new(MYPAINT_BRUSH_INPUTS_COUNT);
    }
    self->rng = rng_double_new(1000);
    self->random_input = 0;
    self->skip = 0;
    self->skip_last_x = 0;
    self->skip_last_y = 0;
    self->skipped_dtime = 0;
    self->print_inputs = FALSE;

    for (i=0; i<MYPAINT_BRUSH_STATES_COUNT; i++) {
      self->states[i] = 0;
    }
    mypaint_brush_new_stroke(self);

    settings_base_values_have_changed(self);

    self->reset_requested = TRUE;

#ifdef HAVE_JSON_C
    self->brush_json = json_object_new_object();
#endif

    return self;
}

void
brush_free(MyPaintBrush *self)
{
    for (int i=0; i<MYPAINT_BRUSH_SETTINGS_COUNT; i++) {
        mypaint_mapping_free(self->settings[i]);
    }
    rng_double_free (self->rng);
    self->rng = NULL;

#ifdef HAVE_JSON_C
    if (self->brush_json) {
        json_object_put(self->brush_json);
    }
#endif

    free(self);
}

/**
  * mypaint_brush_unref: (skip)
  *
  * Decrease the reference count. Will be freed when it hits 0.
  */
void
mypaint_brush_unref(MyPaintBrush *self)
{
    self->refcount--;
    if (self->refcount == 0) {
        brush_free(self);
    }
}
/**
  * mypaint_brush_ref: (skip)
  *
  * Increase the reference count.
  */
void
mypaint_brush_ref(MyPaintBrush *self)
{
    self->refcount++;
}

/**
  * mypaint_brush_get_total_stroke_painting_time:
  *
  * Return the total amount of painting time for the current stroke.
  */
double
mypaint_brush_get_total_stroke_painting_time(MyPaintBrush *self)
{
    return self->stroke_total_painting_time;
}

/**
  * mypaint_brush_set_print_inputs:
  *
  * Enable/Disable printing of brush engine inputs on stderr. Intended for debugging only.
  */
void
mypaint_brush_set_print_inputs(MyPaintBrush *self, gboolean enabled)
{
    self->print_inputs = enabled;
}

/**
  * mypaint_brush_reset:
  *
  * Reset the current brush engine state.
  * Used when the next mypaint_brush_stroke_to() call is not related to the current state.
  * Note that the reset request is queued and changes in state will only happen on next stroke_to()
  */
void
mypaint_brush_reset(MyPaintBrush *self)
{
    self->reset_requested = TRUE;
}

/**
  * mypaint_brush_new_stroke:
  *
  * Start a new stroke.
  */
void
mypaint_brush_new_stroke(MyPaintBrush *self)
{
    self->stroke_current_idling_time = 0;
    self->stroke_total_painting_time = 0;
}

/**
  * mypaint_brush_set_base_value:
  *
  * Set the base value of a brush setting.
  */
void
mypaint_brush_set_base_value(MyPaintBrush *self, MyPaintBrushSetting id, float value)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    mypaint_mapping_set_base_value(self->settings[id], value);

    settings_base_values_have_changed (self);
}

/**
  * mypaint_brush_get_base_value:
  *
  * Get the base value of a brush setting.
  */
float
mypaint_brush_get_base_value(MyPaintBrush *self, MyPaintBrushSetting id)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    return mypaint_mapping_get_base_value(self->settings[id]);
}

/**
  * mypaint_brush_set_mapping_n:
  *
  * Set the number of points used for the dynamics mapping between a #MyPaintBrushInput and #MyPaintBrushSetting.
  */
void
mypaint_brush_set_mapping_n(MyPaintBrush *self, MyPaintBrushSetting id, MyPaintBrushInput input, int n)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    mypaint_mapping_set_n(self->settings[id], input, n);
}

/**
  * mypaint_brush_get_mapping_n:
  *
  * Get the number of points used for the dynamics mapping between a #MyPaintBrushInput and #MyPaintBrushSetting.
  */
int
mypaint_brush_get_mapping_n(MyPaintBrush *self, MyPaintBrushSetting id, MyPaintBrushInput input)
{
    return mypaint_mapping_get_n(self->settings[id], input);
}

/**
  * mypaint_brush_is_constant:
  *
  * Returns TRUE if the brush has no dynamics for the given #MyPaintBrushSetting
  */
gboolean
mypaint_brush_is_constant(MyPaintBrush *self, MyPaintBrushSetting id)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    return mypaint_mapping_is_constant(self->settings[id]);
}

/**
  * mypaint_brush_get_inputs_used_n:
  *
  * Returns how many inputs are used for the dynamics of a #MyPaintBrushSetting
  */
int
mypaint_brush_get_inputs_used_n(MyPaintBrush *self, MyPaintBrushSetting id)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    return mypaint_mapping_get_inputs_used_n(self->settings[id]);
}

/**
  * mypaint_brush_set_mapping_point:
  *
  * Set a X,Y point of a dynamics mapping.
  * The index must be within the number of points set using mypaint_brush_set_mapping_n()
  */
void
mypaint_brush_set_mapping_point(MyPaintBrush *self, MyPaintBrushSetting id, MyPaintBrushInput input, int index, float x, float y)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    mypaint_mapping_set_point(self->settings[id], input, index, x, y);
}

/**
 * mypaint_brush_get_mapping_point:
 * @x: (out): Location to return the X value
 * @y: (out): Location to return the Y value
 *
 * Get a X,Y point of a dynamics mapping.
 **/
void
mypaint_brush_get_mapping_point(MyPaintBrush *self, MyPaintBrushSetting id, MyPaintBrushInput input, int index, float *x, float *y)
{
    assert (id >= 0 && id < MYPAINT_BRUSH_SETTINGS_COUNT);
    mypaint_mapping_get_point(self->settings[id], input, index, x, y);
}

/**
 * mypaint_brush_get_state:
 *
 * Get an internal brush engine state.
 * Normally used for debugging, but can be used to implement record & replay functionality.
 **/
float
mypaint_brush_get_state(MyPaintBrush *self, MyPaintBrushState i)
{
    assert (i >= 0 && i < MYPAINT_BRUSH_STATES_COUNT);
    return self->states[i];
}

/**
 * mypaint_brush_set_state:
 *
 * Set an internal brush engine state.
 * Normally used for debugging, but can be used to implement record & replay functionality.
 **/
void
mypaint_brush_set_state(MyPaintBrush *self, MyPaintBrushState i, float value)
{
    assert (i >= 0 && i < MYPAINT_BRUSH_STATES_COUNT);
    self->states[i] = value;
}


// C fmodf function is not "arithmetic modulo"; it doesn't handle negative dividends as you might expect
// if you expect 0 or a positive number when dealing with negatives, use
// this function instead.
static inline float mod(float a, float N)
{
    float ret = a - N * floor (a / N);
    return ret;
}


// Returns the smallest angular difference
static inline float
smallest_angular_difference(float angleA, float angleB)
{
    float a;
    a = angleB - angleA;
    a = mod((a + 180), 360) - 180;
    a += (a>180) ? -360 : (a<-180) ? 360 : 0;
    return a;
}

//state for loading rgb.txt
int rgb_loaded = 0;

//function to make it easy to blend normal and subtractive color blending modes in linear and non-linear modes
//a is the current smudge state, b is the get_color (get=1) or the brush color (get=0)
//mixing smudge_state+get_color is slightly different than mixing brush_color with smudge_color
//so I've used the bool'get' parameter to differentiate them.
static inline float * mix_colors(MyPaintBrush *self, float *a, float *b, float fac, float gamma, float normsub, float spectral, float get)
{
  float rgbmixnorm[4] = {0};
  float rgbmixsub[4] = {0};
  float rgbmix[4] = {0};
  float spectralmix[4] = {0};
  float spectralmixnorm[4] = {0};
  float spectralmixsub[4] = {0};
  static float result[4] = {0};
  //normsub is the ratio of normal to subtractive
  normsub = CLAMP(normsub, 0.0f, 1.0f);
  //ratio of spectral to RGB
  spectral = CLAMP(spectral, 0.0f, 1.0f);

  if (gamma < 1.0) {
    gamma = 1.0;
  }
  float ar=a[0];
  float ag=a[1];
  float ab=a[2];
  float aa=a[3];
  float br=b[0];
  float bg=b[1];
  float bb=b[2];
  float ba=b[3];

  
  //weird if 100% alpha the get_color is 0,1,0 (bug?)
  //I have seen "green" paint get in the mix
  //This may be unnecessary now that I'm using alpha in the subtactive mode
  //set get_color to the brush color as a compromise
  if (b[3] == 0.0 && get == 1) {

    float h = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_COLOR_H]);
    float s = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_COLOR_S]);
    float v = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_COLOR_V]);  
    hsv_to_rgb_float (&h, &s, &v);

    b[0] = h;
    b[1] = s;
    b[2] = v;
  }

  //do RGB mode (3 lights)
  if (spectral < 1.0) {


    //convert to linear rgb only if gamma is not 1.0 to avoid redundancy and rounding errors
    if (gamma != 1.0) {
      srgb_to_rgb_float(&ar, &ag, &ab, gamma);
      srgb_to_rgb_float(&br, &bg, &bb, gamma);
    }


    //do the mix
    //normal
    if (normsub < 1.0) {
      //premultiply alpha when getting color from canvas
      //the color in the smudge_state (a) is already premultiplied
      float bra =br;
      float bga =bg;
      float bba =bb;
      if (get == 1) {
        bra *=ba;
        bga *=ba;
        bba *=ba;
      }

      rgbmixnorm[0] = fac * ar + (1-fac) * bra;
      rgbmixnorm[1] = fac * ag + (1-fac) * bga;
      rgbmixnorm[2] = fac * ab + (1-fac) * bba;
    }
    //subtractive
    
    
    
    float alpha_b;  //either brush_a or get_color_a
    if (get == 0) {
      alpha_b = 1;
    } else {
      alpha_b = ba;
    }
    
    //calculate a different smudge ratio for subtractive modes
    //may not be coherent but it looks good
    float paint_a_ratio, paint_b_ratio;
    float subfac = fac;
    float alpha_sum = aa + alpha_b;
    if (alpha_sum >0.0) {
      paint_a_ratio = (aa/alpha_sum)*fac;
      paint_b_ratio = (ba/alpha_sum)*(1-fac);
      paint_a_ratio /=(paint_a_ratio+paint_b_ratio);
      paint_b_ratio /=(paint_a_ratio+paint_b_ratio);
    }
    
    float paint_ratio_sum = paint_a_ratio + paint_b_ratio;
    if (paint_ratio_sum > 0.0) {
      subfac = paint_a_ratio/(paint_ratio_sum);
    }
    
    if (normsub > 0.0) {
    
      rgbmixsub[0] = powf(MAX(ar, 0.0001), subfac) * powf(MAX(br, 0.0001),(1-subfac));
      rgbmixsub[1] = powf(MAX(ag, 0.0001), subfac) * powf(MAX(bg, 0.0001),(1-subfac));
      rgbmixsub[2] = powf(MAX(ab, 0.0001), subfac) * powf(MAX(bb, 0.0001),(1-subfac));
    }

    //do eraser_target alpha for smudge+brush color
    if (get == 0) {
      rgbmixnorm[0] /=ba;
      rgbmixnorm[1] /=ba;
      rgbmixnorm[2] /=ba;
    }    
 
    ar = rgbmixnorm[0];
    ag = rgbmixnorm[1];
    ab = rgbmixnorm[2];

    //convert back to sRGB
    if (gamma != 1.0) {
      rgb_to_srgb_float(&ar, &ag, &ab, gamma);
    }
    
    rgbmixnorm[0] = ar;
    rgbmixnorm[1] = ag;
    rgbmixnorm[2] = ab;        

    ar = rgbmixsub[0];
    ag = rgbmixsub[1];
    ab = rgbmixsub[2];


    //convert back to sRGB
    if (gamma != 1.0) {
      rgb_to_srgb_float(&ar, &ag, &ab, gamma);
    }
    
    rgbmixsub[0] = ar;
    rgbmixsub[1] = ag;
    rgbmixsub[2] = ab;

    //combine normal and subtractive RGB modes
    int i = 0;
    for (i=0; i < 4; i++) {  
      rgbmix[i] = CLAMP((((1-normsub)*rgbmixnorm[i]) + (normsub*rgbmixsub[i])), 0.0f, 1.0f);
    }
    rgbmix[3] = CLAMP(fac * aa + (1-fac) * ba, 0.0, 1.0); 
  }
  

  //Spectral Method devised by Scott Burns (36 lights)
  //for smudge_status + get_color, set get parameter to 1
  //clamp to ensure SPD lookup is not out of array bounds
  //rgb_loaded=2 means rgb.txt is missing
  if (spectral > 0.0 && rgb_loaded == 2) {
    //rgb.txt is missing, just do RGB mixing instead
    float *result;
    result = mix_colors(self, a, b, fac, gamma, normsub, 0, get);
    return result;
  }
  if (spectral > 0.0 && rgb_loaded != 2) {
    ar=CLAMP(a[0], 0.0f, 1.0f);
    ag=CLAMP(a[1], 0.0f, 1.0f);
    ab=CLAMP(a[2], 0.0f, 1.0f);
    aa=CLAMP(a[3], 0.0f, 1.0f);
    
    br=CLAMP(b[0], 0.0f, 1.0f);
    bg=CLAMP(b[1], 0.0f, 1.0f);
    bb=CLAMP(b[2], 0.0f, 1.0f);
    ba=CLAMP(b[3], 0.0f, 1.0f);

    
    
    //gamma can't be less than 2.4 since that is what the SPD table was calculated for
    //gamma above 2.4 is obviously "wrong" for that table, but shouldn't segfault
    //in fact gamma like 2.41, 2.42 can have a pleasing lightening effect
    if (gamma < 2.4) {
      gamma = 2.4;
    }
    //if rgb.txt is missing don't try this function again
    if( access( "rgb.txt", R_OK ) == -1 ) {
      rgb_loaded = 2;
      printf("\n rgb.txt is missing. Cannot use Subtractive smudge mode\n ");
      float *result;
      result = mix_colors(self, a, b, fac, gamma, normsub, 0, get);
      return result;
    } 

    //load rgb data the first time this subtractive mode is invoked.
    if (rgb_loaded != 1 && rgb_loaded != 2) {
      
      int p=0;
      for (p=0; p < SPD; p++) {
        RGBSPD[p] = (float *)malloc(WIDTH * sizeof(float));
      }
            
      char buffer[1024] ;
      char *record,*line;
      int i=0,j=0;

      FILE *fstream = fopen("rgb.txt","r");
      if(fstream == NULL)
      {
        printf("\n file opening failed ");

      }
      while((line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
      {
        record = strtok(line,",");
        while(record != NULL)
      {
        RGBSPD[i][j++] = atof(record) ;
        record = strtok(NULL,",");
      }
      ++i;
      j=0;
      }
      fclose (fstream);
      rgb_loaded=1;
    }
    
    //convert RGB to nearest integer index in huge SPD array by convert rgb base 256 to base 10
    //This should get the closest 
    int rgb_index_a = ((roundf(ar *255) * 256 * 256) + (roundf(ag *255)) * 256) + roundf(ab *255);
    int rgb_index_b = ((roundf(br *255) * 256 * 256) + (roundf(bg *255)) * 256) + roundf(bb *255);

    double new_spd_norm[36] = {0};
    double new_spd_sub[36] = {0};

    //adjust the mix ratio (fac) according to the alpha levels.  More transparent=weaker paint
    
    //When mixing brush color with smudge, use 100% opacity for brush
    //This is consistent with the normal mixing method MyPaint already uses
    //and without this you might not ever reach brush color. 
    
    float alpha_b;  //either brush_a or get_color_a
    if (get == 0) {
      alpha_b = 1;
    } else {
      alpha_b = ba;
    }
    
    //calculate a different smudge ratio for subtractive modes
    //may not be coherent but it looks good
    float paint_a_ratio, paint_b_ratio;
    float subfac = fac;
    float alpha_sum = aa + alpha_b;
    if (alpha_sum >0.0) {
      paint_a_ratio = (aa/alpha_sum)*fac;
      paint_b_ratio = (ba/alpha_sum)*(1-fac);
      paint_a_ratio /=(paint_a_ratio+paint_b_ratio);
      paint_b_ratio /=(paint_a_ratio+paint_b_ratio);
    }
    
    float paint_ratio_sum = paint_a_ratio + paint_b_ratio;
    if (paint_ratio_sum > 0.0) {
      subfac = paint_a_ratio/(paint_ratio_sum);
    }
    //mixing part
    int j=0;
    for (j=0; j < 36; j++) {
      //mix the two SPDs to get new color SPD- added and weighted geometric mean for subtractive
      if (normsub < 1.0) {
        new_spd_norm[j] = RGBSPD[rgb_index_a][j] * fac + RGBSPD[rgb_index_b][j] * (1-fac);
      }
      if (normsub > 0.0) {
        new_spd_sub[j] = powf(RGBSPD[rgb_index_a][j], (subfac)) * powf(RGBSPD[rgb_index_b][j], (1-subfac));
      }
    }
    //convert new_spd to rgb
    //multiply T_MATRIX by new_spd to get spectralmix as new rgb values
    //T_MATRIX  and RGBSPD is precalculated for D65 light source
    //Other light sources would need entirely new tables created
    
    if (normsub < 1.0) {
      //normal
      int k=0;
      for (k=0; k < 3; k++) {
        int l=0;
        for (l=0; l < 36; l++) {
          spectralmixnorm[k] +=  (T_MATRIX[k][l] * new_spd_norm[l]);
        }
        spectralmixnorm[k] = CLAMP(spectralmixnorm[k], 0.0f, 1.0f);
      }
      //convert back to sRGB
      
      ar = spectralmixnorm[0];
      ag = spectralmixnorm[1];
      ab = spectralmixnorm[2];

      rgb_to_srgb_float(&ar, &ag, &ab, gamma);
    
  
      spectralmixnorm[0] = ar;
      spectralmixnorm[1] = ag;
      spectralmixnorm[2] = ab;
    }
    
    if (normsub > 0.0) {
      //do subtractive
      int k=0;
      for (k=0; k < 3; k++) {
        int l=0;
        for (l=0; l < 36; l++) {
          spectralmixsub[k] +=  (T_MATRIX[k][l] * new_spd_sub[l]);
        }
        spectralmixsub[k] = CLAMP(spectralmixsub[k], 0.0f, 1.0f);
      }
      //convert back to sRGB
      
      ar = spectralmixsub[0];
      ag = spectralmixsub[1];
      ab = spectralmixsub[2];

      rgb_to_srgb_float(&ar, &ag, &ab, gamma);
    
  
      spectralmixsub[0] = ar;
      spectralmixsub[1] = ag;
      spectralmixsub[2] = ab;
    
    }
    
    //mix spectral normal and subtractive results
    int i = 0;
    for (i=0; i < 4; i++) {  
      spectralmix[i] = (((1-normsub)*spectralmixnorm[i]) + (normsub*spectralmixsub[i]));
    }
    
    spectralmix[3] = CLAMP(fac * aa + (1-fac) * ba, 0.0, 1.0);
    
  }
  //mix rgbmix and spectralmix-
  int i = 0;
  for (i=0; i < 4; i++) {  
    result[i] = CLAMP((((1-spectral)*rgbmix[i]) + (spectral*spectralmix[i])), 0.0f, 1.0f);
  }
  
  //compare result to the smudge_state (a) and tweak the chroma and luma
  //based on hue angle difference
  if (self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_DESATURATION] != 0 || self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_DARKEN] != 0) {
    float smudge_h = a[0];
    float smudge_c = a[1];
    float smudge_y = a[2];
    
    float result_h = result[0];
    float result_c = result[1];
    float result_y = result[2];
    
    rgb_to_hcy_float (&smudge_h, &smudge_c, &smudge_y);
    rgb_to_hcy_float (&result_h, &result_c, &result_y);
    
    //set our Brightness of the mix according to mode result. and also process saturation
    //Don't process achromatic colors (HCY has 3 achromatic states C=0 or Y=1 or Y=0)
    if (result_c != 0 && smudge_c != 0 && result_y != 0 && smudge_y != 0 && result_y != 1 && smudge_y != 1) {  

      //determine hue diff, proportional to smudge ratio
      //if fac is .5 the hueratio should be 1. When fac closer to 0 and closer to 1 should decrease towards zero
      //why- because if smudge is 0 or 1, only 100% of one of the brush or smudge color will be used so there is no comparison to make.
      //when fac is 0.5 the smudge and brush are mixed 50/50, so the huedifference should be respected 100%
      float huediff_sat;
      float huediff_bright;
      float hueratio = (0.5 - fabs(0.5 - fac)) / 0.5;
      float anglediff = fabs(smallest_angular_difference(result_h*360, smudge_h*360)/360);
      
          
      //calculate the adjusted hue difference and apply that to the saturation level and/or brightness
      //if smudge_desaturation setting is zero, the huediff will be zero.  Likewise when smudge (fac) is 0 or 1, the huediff will be zero.
      huediff_sat =  anglediff * self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_DESATURATION] * hueratio;
      //do the same for brightness
      huediff_bright = anglediff * self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_DARKEN] * hueratio;
      
      //do the desaturation.  More strongly saturated colors will reduce S more than less saturated colors
      result_c = result_c*(1-huediff_sat);
      
      //attempt to simulate subtractive mode by darkening colors if they are different hues
      result_y = result_y*(1-huediff_bright);
      
      //convert back to RGB
      hcy_to_rgb_float (&result_h, &result_c, &result_y);
      
      result[0] = result_h;
      result[1] = result_c;
      result[2] = result_y;
    }
  }

  return result; 
}

  // returns the fraction still left after t seconds
  float exp_decay (float T_const, float t)
  {
    // the argument might not make mathematical sense (whatever.)
    if (T_const <= 0.001) {
      return 0.0;
    }

    const float arg = -t / T_const;
    return expf(arg);
  }


  void settings_base_values_have_changed (MyPaintBrush *self)
  {
    // precalculate stuff that does not change dynamically

    // Precalculate how the physical speed will be mapped to the speed input value.
    // The forumla for this mapping is:
    //
    // y = log(gamma+x)*m + q;
    //
    // x: the physical speed (pixels per basic dab radius)
    // y: the speed input that will be reported
    // gamma: parameter set by ths user (small means a logarithmic mapping, big linear)
    // m, q: parameters to scale and translate the curve
    //
    // The code below calculates m and q given gamma and two hardcoded constraints.
    //
    int i=0;
    for (i=0; i<2; i++) {
      float gamma;
      gamma = mypaint_mapping_get_base_value(self->settings[(i==0)?MYPAINT_BRUSH_SETTING_SPEED1_GAMMA:MYPAINT_BRUSH_SETTING_SPEED2_GAMMA]);
      gamma = expf(gamma);

      float fix1_x, fix1_y, fix2_x, fix2_dy;
      fix1_x = 45.0;
      fix1_y = 0.5;
      fix2_x = 45.0;
      fix2_dy = 0.015;

      float m, q;
      float c1;
      c1 = log(fix1_x+gamma);
      m = fix2_dy * (fix2_x + gamma);
      q = fix1_y - m*c1;

      self->speed_mapping_gamma[i] = gamma;
      self->speed_mapping_m[i] = m;
      self->speed_mapping_q[i] = q;
    }
  }

  // This function runs a brush "simulation" step. Usually it is
  // called once or twice per dab. In theory the precision of the
  // "simulation" gets better when it is called more often. In
  // practice this only matters if there are some highly nonlinear
  // mappings in critical places or extremely few events per second.
  //
  // note: parameters are is dx/ddab, ..., dtime/ddab (dab is the number, 5.0 = 5th dab)
  void update_states_and_setting_values (MyPaintBrush *self, float step_ddab, float step_dx, float step_dy, float step_dpressure, float step_declination, float step_ascension, float step_dtime, float step_viewzoom, float step_viewrotation)
  {
    float pressure;
    float inputs[MYPAINT_BRUSH_INPUTS_COUNT];
    float viewzoom;
    float viewrotation;
    float gridmap_scale;
    float gridmap_scale_x;
    float gridmap_scale_y;

    if (step_dtime < 0.0) {
      printf("Time is running backwards!\n");
      step_dtime = 0.001;
    } else if (step_dtime == 0.0) {
      // FIXME: happens about every 10th start, workaround (against division by zero)
      step_dtime = 0.001;
    }

    self->states[MYPAINT_BRUSH_STATE_X]        += step_dx;
    self->states[MYPAINT_BRUSH_STATE_Y]        += step_dy;
    self->states[MYPAINT_BRUSH_STATE_PRESSURE] += step_dpressure;

    self->states[MYPAINT_BRUSH_STATE_DECLINATION] += step_declination;
    self->states[MYPAINT_BRUSH_STATE_ASCENSION] += step_ascension;
    
    self->states[MYPAINT_BRUSH_STATE_VIEWZOOM] = step_viewzoom;
    self->states[MYPAINT_BRUSH_STATE_VIEWROTATION] = mod((step_viewrotation * 180.0 / M_PI) + 180.0, 360.0) -180.0;
    gridmap_scale = expf(self->settings_value[MYPAINT_BRUSH_SETTING_GRIDMAP_SCALE]);
    gridmap_scale_x = self->settings_value[MYPAINT_BRUSH_SETTING_GRIDMAP_SCALE_X];
    gridmap_scale_y = self->settings_value[MYPAINT_BRUSH_SETTING_GRIDMAP_SCALE_Y];
    self->states[MYPAINT_BRUSH_STATE_GRIDMAP_X] = mod(fabsf(self->states[MYPAINT_BRUSH_STATE_ACTUAL_X] * gridmap_scale_x), (gridmap_scale * 256.0)) / (gridmap_scale * 256.0) * 256.0;
    self->states[MYPAINT_BRUSH_STATE_GRIDMAP_Y] = mod(fabsf(self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y] * gridmap_scale_y), (gridmap_scale * 256.0)) / (gridmap_scale * 256.0) * 256.0;
    
    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_X] < 0.0) {
      self->states[MYPAINT_BRUSH_STATE_GRIDMAP_X] = 256.0 - self->states[MYPAINT_BRUSH_STATE_GRIDMAP_X];
    }

    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y] < 0.0) {
      self->states[MYPAINT_BRUSH_STATE_GRIDMAP_Y] = 256.0 - self->states[MYPAINT_BRUSH_STATE_GRIDMAP_Y];
    }

    float base_radius = expf(mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC]));
    
    //first iteration is zero, set to 1, then flip to -1, back and forth
    //useful for Anti-Art's mirrored offset but could be useful elsewhere
    if (self->states[MYPAINT_BRUSH_STATE_FLIP] == 0) {
      self->states[MYPAINT_BRUSH_STATE_FLIP] = +1;
    } else {
      self->states[MYPAINT_BRUSH_STATE_FLIP] *= -1;
    }

    // FIXME: does happen (interpolation problem?)
    if (self->states[MYPAINT_BRUSH_STATE_PRESSURE] <= 0.0) self->states[MYPAINT_BRUSH_STATE_PRESSURE] = 0.0;
    pressure = self->states[MYPAINT_BRUSH_STATE_PRESSURE];

    { // start / end stroke (for "stroke" input only)
      if (!self->states[MYPAINT_BRUSH_STATE_STROKE_STARTED]) {
        if (pressure > mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_STROKE_THRESHOLD]) + 0.0001) {
          // start new stroke
          //printf("stroke start %f\n", pressure);
          self->states[MYPAINT_BRUSH_STATE_STROKE_STARTED] = 1;
          self->states[MYPAINT_BRUSH_STATE_STROKE] = 0.0;
        }
      } else {
        if (pressure <= mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_STROKE_THRESHOLD]) * 0.9 + 0.0001) {
          // end stroke
          //printf("stroke end\n");
          self->states[MYPAINT_BRUSH_STATE_STROKE_STARTED] = 0;
        }
      }
    }

    // now follows input handling

    float norm_dx, norm_dy, norm_dist, norm_speed;
    //adjust speed with viewzoom
    norm_dx = step_dx / step_dtime *self->states[MYPAINT_BRUSH_STATE_VIEWZOOM];
    norm_dy = step_dy / step_dtime *self->states[MYPAINT_BRUSH_STATE_VIEWZOOM];

    norm_speed = hypotf(norm_dx, norm_dy);
    //norm_dist should relate to brush size, whereas norm_speed should not
    norm_dist = hypotf(step_dx / step_dtime / base_radius, step_dy / step_dtime / base_radius) * step_dtime;

    inputs[MYPAINT_BRUSH_INPUT_PRESSURE] = pressure * expf(mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_PRESSURE_GAIN_LOG]));
    inputs[MYPAINT_BRUSH_INPUT_SPEED1] = log(self->speed_mapping_gamma[0] + self->states[MYPAINT_BRUSH_STATE_NORM_SPEED1_SLOW])*self->speed_mapping_m[0] + self->speed_mapping_q[0], 0.0, 4.0;
    inputs[MYPAINT_BRUSH_INPUT_SPEED2] = log(self->speed_mapping_gamma[1] + self->states[MYPAINT_BRUSH_STATE_NORM_SPEED2_SLOW])*self->speed_mapping_m[1] + self->speed_mapping_q[1], 0.0, 4.0;
    
    inputs[MYPAINT_BRUSH_INPUT_RANDOM] = self->random_input;
    inputs[MYPAINT_BRUSH_INPUT_STROKE] = MIN(self->states[MYPAINT_BRUSH_STATE_STROKE], 1.0);
    //correct direction for varying view rotation
    inputs[MYPAINT_BRUSH_INPUT_DIRECTION] = fmodf(atan2f (self->states[MYPAINT_BRUSH_STATE_DIRECTION_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_DX])/(2*M_PI)*360 + self->states[MYPAINT_BRUSH_STATE_VIEWROTATION] + 180.0, 180.0);
    inputs[MYPAINT_BRUSH_INPUT_DIRECTION_ANGLE] = fmodf (atan2f(self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) / (2 * M_PI) * 360 + 180 + self->states[MYPAINT_BRUSH_STATE_VIEWROTATION] + 180.0, 360.0) ;
    inputs[MYPAINT_BRUSH_INPUT_TILT_DECLINATION] = self->states[MYPAINT_BRUSH_STATE_DECLINATION];
    //correct ascension for varying view rotation, use custom mod
    inputs[MYPAINT_BRUSH_INPUT_TILT_ASCENSION] = mod(self->states[MYPAINT_BRUSH_STATE_ASCENSION] + self->states[MYPAINT_BRUSH_STATE_VIEWROTATION] + 180.0, 360.0) - 180.0;
    inputs[MYPAINT_BRUSH_INPUT_VIEWZOOM] = (mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC])) - logf(base_radius * 1 / self->states[MYPAINT_BRUSH_STATE_VIEWZOOM]);
    inputs[MYPAINT_BRUSH_INPUT_ATTACK_ANGLE] = smallest_angular_difference(self->states[MYPAINT_BRUSH_STATE_ASCENSION], mod(atan2f(self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) / (2 * M_PI) * 360 + 90, 360));
    inputs[MYPAINT_BRUSH_INPUT_BRUSH_RADIUS] = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC]);
    inputs[MYPAINT_BRUSH_INPUT_GRIDMAP_X] = CLAMP(self->states[MYPAINT_BRUSH_STATE_GRIDMAP_X], 0.0, 256.0);
    inputs[MYPAINT_BRUSH_INPUT_GRIDMAP_Y] = CLAMP(self->states[MYPAINT_BRUSH_STATE_GRIDMAP_Y], 0.0, 256.0);

    inputs[MYPAINT_BRUSH_INPUT_CUSTOM] = self->states[MYPAINT_BRUSH_STATE_CUSTOM_INPUT];
    if (self->print_inputs) {
      printf("press=% 4.3f, speed1=% 4.4f\tspeed2=% 4.4f\tstroke=% 4.3f\tcustom=% 4.3f\tviewzoom=% 4.3f\tviewrotation=% 4.3f\tasc=% 4.3f\tdir=% 4.3f\tdec=% 4.3f\tdabang=% 4.3f\tgridmapx=% 4.3f\tgridmapy=% 4.3fX=% 4.3f\tY=% 4.3f\n", (double)inputs[MYPAINT_BRUSH_INPUT_PRESSURE], (double)inputs[MYPAINT_BRUSH_INPUT_SPEED1], (double)inputs[MYPAINT_BRUSH_INPUT_SPEED2], (double)inputs[MYPAINT_BRUSH_INPUT_STROKE], (double)inputs[MYPAINT_BRUSH_INPUT_CUSTOM], (double)inputs[MYPAINT_BRUSH_INPUT_VIEWZOOM], (double)self->states[MYPAINT_BRUSH_STATE_VIEWROTATION], (double)inputs[MYPAINT_BRUSH_INPUT_TILT_ASCENSION], (double)inputs[MYPAINT_BRUSH_INPUT_DIRECTION], (double)inputs[MYPAINT_BRUSH_INPUT_TILT_DECLINATION], (double)self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_ANGLE], (double)inputs[MYPAINT_BRUSH_INPUT_GRIDMAP_X], (double)inputs[MYPAINT_BRUSH_INPUT_GRIDMAP_Y], (double)self->states[MYPAINT_BRUSH_STATE_ACTUAL_X], (double)self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y]);
    }
    // FIXME: this one fails!!!
    //assert(inputs[MYPAINT_BRUSH_INPUT_SPEED1] >= 0.0 && inputs[MYPAINT_BRUSH_INPUT_SPEED1] < 1e8); // checking for inf

    int i=0;
    for (i=0; i<MYPAINT_BRUSH_SETTINGS_COUNT; i++) {
      self->settings_value[i] = mypaint_mapping_calculate(self->settings[i], (inputs));
    }

    {
      float fac = 1.0 - exp_decay (self->settings_value[MYPAINT_BRUSH_SETTING_SLOW_TRACKING_PER_DAB], step_ddab);
      self->states[MYPAINT_BRUSH_STATE_ACTUAL_X] += (self->states[MYPAINT_BRUSH_STATE_X] - self->states[MYPAINT_BRUSH_STATE_ACTUAL_X]) * fac;
      self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y] += (self->states[MYPAINT_BRUSH_STATE_Y] - self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y]) * fac;
    }

    { // slow speed
      float fac;
      fac = 1.0 - exp_decay (self->settings_value[MYPAINT_BRUSH_SETTING_SPEED1_SLOWNESS], step_dtime);
      self->states[MYPAINT_BRUSH_STATE_NORM_SPEED1_SLOW] += (norm_speed - self->states[MYPAINT_BRUSH_STATE_NORM_SPEED1_SLOW]) * fac;
      fac = 1.0 - exp_decay (self->settings_value[MYPAINT_BRUSH_SETTING_SPEED2_SLOWNESS], step_dtime);
      self->states[MYPAINT_BRUSH_STATE_NORM_SPEED2_SLOW] += (norm_speed - self->states[MYPAINT_BRUSH_STATE_NORM_SPEED2_SLOW]) * fac;
    }

    { // slow speed, but as vector this time

      // FIXME: offset_by_speed should be removed.
      //   Is it broken, non-smooth, system-dependent math?!
      //   A replacement could be a directed random offset.

      float time_constant = expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS]*0.01)-1.0;
      // Workaround for a bug that happens mainly on Windows, causing
      // individual dabs to be placed far far away. Using the speed
      // with zero filtering is just asking for trouble anyway.
      if (time_constant < 0.002) time_constant = 0.002;
      float fac = 1.0 - exp_decay (time_constant, step_dtime);
      self->states[MYPAINT_BRUSH_STATE_NORM_DX_SLOW] += (norm_dx - self->states[MYPAINT_BRUSH_STATE_NORM_DX_SLOW]) * fac;
      self->states[MYPAINT_BRUSH_STATE_NORM_DY_SLOW] += (norm_dy - self->states[MYPAINT_BRUSH_STATE_NORM_DY_SLOW]) * fac;
    }

    { // orientation (similar lowpass filter as above, but use dabtime instead of wallclock time)
      //adjust speed with viewzoom
      float dx = step_dx *self->states[MYPAINT_BRUSH_STATE_VIEWZOOM];
      float dy = step_dy *self->states[MYPAINT_BRUSH_STATE_VIEWZOOM];



      float step_in_dabtime = hypotf(dx, dy); // FIXME: are we recalculating something here that we already have?
      float fac = 1.0 - exp_decay (exp(self->settings_value[MYPAINT_BRUSH_SETTING_DIRECTION_FILTER]*0.5)-1.0, step_in_dabtime);

      float dx_old = self->states[MYPAINT_BRUSH_STATE_DIRECTION_DX];
      float dy_old = self->states[MYPAINT_BRUSH_STATE_DIRECTION_DY];
      
      // 360 Direction
	  self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX] += (dx - self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) * fac;
	  self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY] += (dy - self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY]) * fac;
      
      // use the opposite speed vector if it is closer (we don't care about 180 degree turns)
      if (SQR(dx_old-dx) + SQR(dy_old-dy) > SQR(dx_old-(-dx)) + SQR(dy_old-(-dy))) {
        dx = -dx;
        dy = -dy;
      }
      self->states[MYPAINT_BRUSH_STATE_DIRECTION_DX] += (dx - self->states[MYPAINT_BRUSH_STATE_DIRECTION_DX]) * fac;
      self->states[MYPAINT_BRUSH_STATE_DIRECTION_DY] += (dy - self->states[MYPAINT_BRUSH_STATE_DIRECTION_DY]) * fac;
    }

    { // custom input
      float fac;
      fac = 1.0 - exp_decay (self->settings_value[MYPAINT_BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS], 0.1);
      self->states[MYPAINT_BRUSH_STATE_CUSTOM_INPUT] += (self->settings_value[MYPAINT_BRUSH_SETTING_CUSTOM_INPUT] - self->states[MYPAINT_BRUSH_STATE_CUSTOM_INPUT]) * fac;
    }

    { // stroke length
      float frequency;
      float wrap;
      frequency = expf(-self->settings_value[MYPAINT_BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC]);
      self->states[MYPAINT_BRUSH_STATE_STROKE] += norm_dist * frequency;
      // can happen, probably caused by rounding
      if (self->states[MYPAINT_BRUSH_STATE_STROKE] < 0) self->states[MYPAINT_BRUSH_STATE_STROKE] = 0;
      wrap = 1.0 + self->settings_value[MYPAINT_BRUSH_SETTING_STROKE_HOLDTIME];
      if (self->states[MYPAINT_BRUSH_STATE_STROKE] > wrap) {
        if (wrap > 9.9 + 1.0) {
          // "inifinity", just hold stroke somewhere >= 1.0
          self->states[MYPAINT_BRUSH_STATE_STROKE] = 1.0;
        } else {
          self->states[MYPAINT_BRUSH_STATE_STROKE] = fmodf(self->states[MYPAINT_BRUSH_STATE_STROKE], wrap);
          // just in case
          if (self->states[MYPAINT_BRUSH_STATE_STROKE] < 0) self->states[MYPAINT_BRUSH_STATE_STROKE] = 0;
        }
      }
    }

    // calculate final radius
    float radius_log;
    radius_log = self->settings_value[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC];
    self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] = expf(radius_log);
    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] < ACTUAL_RADIUS_MIN) self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] = ACTUAL_RADIUS_MIN;
    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] > ACTUAL_RADIUS_MAX) self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] = ACTUAL_RADIUS_MAX;

    // aspect ratio (needs to be caluclated here because it can affect the dab spacing)
    self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_RATIO] = self->settings_value[MYPAINT_BRUSH_SETTING_ELLIPTICAL_DAB_RATIO];
    //correct dab angle for view rotation
    self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_ANGLE] = mod(self->settings_value[MYPAINT_BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE] - self->states[MYPAINT_BRUSH_STATE_VIEWROTATION] + 180.0, 180.0) - 180.0;
  }

  // Called only from stroke_to(). Calculate everything needed to
  // draw the dab, then let the surface do the actual drawing.
  //
  // This is only gets called right after update_states_and_setting_values().
  // Returns TRUE if the surface was modified.
  gboolean prepare_and_draw_dab (MyPaintBrush *self, MyPaintSurface * surface)
  {
    float x, y, opaque;
    float radius;

    // ensure we don't get a positive result with two negative opaque values
    if (self->settings_value[MYPAINT_BRUSH_SETTING_OPAQUE] < 0) self->settings_value[MYPAINT_BRUSH_SETTING_OPAQUE] = 0;
    opaque = self->settings_value[MYPAINT_BRUSH_SETTING_OPAQUE] * self->settings_value[MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY];
    opaque = CLAMP(opaque, 0.0, 1.0);
    //if (opaque == 0.0) return FALSE; <-- cannot do that, since we need to update smudge state.
    if (self->settings_value[MYPAINT_BRUSH_SETTING_OPAQUE_LINEARIZE]) {
      // OPTIMIZE: no need to recalculate this for each dab
      float alpha, beta, alpha_dab, beta_dab;
      float dabs_per_pixel;
      // dabs_per_pixel is just estimated roughly, I didn't think hard
      // about the case when the radius changes during the stroke
      dabs_per_pixel = (
                        mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS]) +
                        mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_DABS_PER_BASIC_RADIUS])
                        ) * 2.0;

      // the correction is probably not wanted if the dabs don't overlap
      if (dabs_per_pixel < 1.0) dabs_per_pixel = 1.0;

      // interpret the user-setting smoothly
      dabs_per_pixel = 1.0 + mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_OPAQUE_LINEARIZE])*(dabs_per_pixel-1.0);

      // see doc/brushdab_saturation.png
      //      beta = beta_dab^dabs_per_pixel
      // <==> beta_dab = beta^(1/dabs_per_pixel)
      alpha = opaque;
      beta = 1.0-alpha;
      beta_dab = powf(beta, 1.0/dabs_per_pixel);
      alpha_dab = 1.0-beta_dab;
      opaque = alpha_dab;
    }

    x = self->states[MYPAINT_BRUSH_STATE_ACTUAL_X];
    y = self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y];

    float base_radius = expf(mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC]));

    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_X]) {
      x += self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_X] * base_radius * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
    }

    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_Y]) {
      y += self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_Y] * base_radius * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
    }
    
    //Anti_Art offsets tweaked by BrienD.  Adjusted with ANGLE_ADJ and OFFSET_MULTIPLIER
    
    //offset to one side of direction
    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE]) {
      x += cos((fmodf ((atan2f(self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) ) / (2 * M_PI) * 360 - 90, 360.0) + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ])* M_PI / 180) * base_radius * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
      y += sin((fmodf ((atan2f(self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) ) / (2 * M_PI) * 360 - 90, 360.0) + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ])* M_PI / 180) * base_radius * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
      
    }
    //offset to one side of ascension angle
    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ASC]) {
      x += cos((self->states[MYPAINT_BRUSH_STATE_ASCENSION] + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ]) * M_PI / 180) * base_radius * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ASC] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
      y += sin((self->states[MYPAINT_BRUSH_STATE_ASCENSION] + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ]) * M_PI / 180) * base_radius * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ASC] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);

    }
    
    //offset mirrored to sides of direction
    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2]) {
      
      if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2] < 0) {
        self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2] = 0;
      }
      x += cos((fmodf ((atan2f(self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) ) / (2 * M_PI) * 360 - 90, 360.0) + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ])* M_PI / 180) * base_radius * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]) * self->states[MYPAINT_BRUSH_STATE_FLIP];
      y += sin((fmodf ((atan2f(self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DY], self->states[MYPAINT_BRUSH_STATE_DIRECTION_ANGLE_DX]) ) / (2 * M_PI) * 360 - 90, 360.0) + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ])* M_PI / 180) * base_radius * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]) * self->states[MYPAINT_BRUSH_STATE_FLIP];
      
    }

    //offset mirrored to sides of ascension angle
    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2_ASC]) {
      
      if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2_ASC] < 0) {
        self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2_ASC] = 0;
      }  
      x += cos((self->states[MYPAINT_BRUSH_STATE_ASCENSION] + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ]) * M_PI / 180) * base_radius * self->states[MYPAINT_BRUSH_STATE_FLIP] * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2_ASC] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
      y += sin((self->states[MYPAINT_BRUSH_STATE_ASCENSION] + self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_ADJ]) * M_PI / 180) * base_radius * self->states[MYPAINT_BRUSH_STATE_FLIP] * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_ANGLE_2_ASC] * expf(self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_MULTIPLIER]);
    }

    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_BY_SPEED]) {
      x += self->states[MYPAINT_BRUSH_STATE_NORM_DX_SLOW] * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_BY_SPEED] * 0.1 / self->states[MYPAINT_BRUSH_STATE_VIEWZOOM];
      y += self->states[MYPAINT_BRUSH_STATE_NORM_DY_SLOW] * self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_BY_SPEED] * 0.1 / self->states[MYPAINT_BRUSH_STATE_VIEWZOOM];
    }

    if (self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_BY_RANDOM]) {
      float amp = self->settings_value[MYPAINT_BRUSH_SETTING_OFFSET_BY_RANDOM];
      if (amp < 0.0) amp = 0.0;
      x += rand_gauss (self->rng) * amp * base_radius;
      y += rand_gauss (self->rng) * amp * base_radius;
    }


    radius = self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS];
    if (self->settings_value[MYPAINT_BRUSH_SETTING_RADIUS_BY_RANDOM]) {
      float radius_log, alpha_correction;
      // go back to logarithmic radius to add the noise
      radius_log  = self->settings_value[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC];
      radius_log += rand_gauss (self->rng) * self->settings_value[MYPAINT_BRUSH_SETTING_RADIUS_BY_RANDOM];
      radius = expf(radius_log);
      radius = CLAMP(radius, ACTUAL_RADIUS_MIN, ACTUAL_RADIUS_MAX);
      alpha_correction = self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] / radius;
      alpha_correction = SQR(alpha_correction);
      if (alpha_correction <= 1.0) {
        opaque *= alpha_correction;
      }
    }

    // update smudge color
    if (self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_LENGTH] < 1.0 &&
    // optimization, since normal brushes have smudge_length == 0.5 without actually smudging
    (self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE] != 0.0 || !mypaint_mapping_is_constant(self->settings[MYPAINT_BRUSH_SETTING_SMUDGE])) &&
    //smudge_lock setting to stop updating at start of stroke 
    !(self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_LOCK] > 0.0 && self->states[MYPAINT_BRUSH_STATE_STROKE_STARTED])) {

      float fac = self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_LENGTH];
      if (fac < 0.01) fac = 0.01;
      int px, py;
      px = ROUND(x);
      py = ROUND(y);


      //determine which smudge bucket to use and update
      int bucket = CLAMP(roundf(self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_BUCKET]), 0, 255);

      // Calling get_color() is almost as expensive as rendering a
      // dab. Because of this we use the previous value if it is not
      // expected to hurt quality too much. We call it at most every
      // second dab.
      float r, g, b, a;
      smudge_buckets[bucket][8] *= fac;
      if (smudge_buckets[bucket][8] < (0.5*fac*powf(1000.0, -self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_LENGTH_LOG])) + 0.0000000000000001) {
        if (smudge_buckets[bucket][8] == 0.0) {
          // first initialization of smudge color
          fac = 0.0;
        }
        smudge_buckets[bucket][8] = 1.0;

        float smudge_radius = radius * expf(self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_RADIUS_LOG]);
        smudge_radius = CLAMP(smudge_radius, ACTUAL_RADIUS_MIN, ACTUAL_RADIUS_MAX);
        mypaint_surface_get_color(surface, px, py, smudge_radius, &r, &g, &b, &a);
        
        
        smudge_buckets[bucket][4] = r;
        smudge_buckets[bucket][5] = g;
        smudge_buckets[bucket][6] = b;
        smudge_buckets[bucket][7] = a;
        
      } else {
        r = smudge_buckets[bucket][4];
        g = smudge_buckets[bucket][5];
        b = smudge_buckets[bucket][6];
        a = smudge_buckets[bucket][7];
      }
      // updated the smudge color (stored with premultiplied alpha)
      float smudge_state[4] = {smudge_buckets[bucket][0], smudge_buckets[bucket][1], smudge_buckets[bucket][2], smudge_buckets[bucket][3]};
      float smudge_get[4] = {r, g, b, a};

      float *smudge_new;
      smudge_new = mix_colors(self, smudge_state, smudge_get, fac, self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_GAMMA], self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_NORMAL_SUB], self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_SPECTRAL], 1);
      
      smudge_buckets[bucket][0] = smudge_new[0];
      smudge_buckets[bucket][1] = smudge_new[1];
      smudge_buckets[bucket][2] = smudge_new[2];
      smudge_buckets[bucket][3] = smudge_new[3];
      
      //update all the states
      self->states[MYPAINT_BRUSH_STATE_SMUDGE_RA] = smudge_buckets[bucket][0];
      self->states[MYPAINT_BRUSH_STATE_SMUDGE_GA] = smudge_buckets[bucket][1];
      self->states[MYPAINT_BRUSH_STATE_SMUDGE_BA] = smudge_buckets[bucket][2];
      self->states[MYPAINT_BRUSH_STATE_SMUDGE_A] = smudge_buckets[bucket][3];
      self->states[MYPAINT_BRUSH_STATE_LAST_GETCOLOR_R] = smudge_buckets[bucket][4];
      self->states[MYPAINT_BRUSH_STATE_LAST_GETCOLOR_G] = smudge_buckets[bucket][5];
      self->states[MYPAINT_BRUSH_STATE_LAST_GETCOLOR_B] = smudge_buckets[bucket][6];
      self->states[MYPAINT_BRUSH_STATE_LAST_GETCOLOR_A] = smudge_buckets[bucket][7];
      self->states[MYPAINT_BRUSH_STATE_LAST_GETCOLOR_RECENTNESS] = smudge_buckets[bucket][8];

      
    }

    // color part
    float color_h = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_COLOR_H]);
    float color_s = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_COLOR_S]);
    float color_v = mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_COLOR_V]);
    float eraser_target_alpha = 1.0;
    float brush_h, brush_c, brush_y, smudge_h, smudge_c, smudge_y;

    if (self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE] > 0.0) {
      float fac = self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE];
      hsv_to_rgb_float (&color_h, &color_s, &color_v);

      //determine which smudge bucket to use when mixing with brush color
      int bucket = CLAMP(roundf(self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_BUCKET]), 0, 255);
            
      if (fac > 1.0) fac = 1.0;
        // If the smudge color somewhat transparent, then the resulting
        // dab will do erasing towards that transparency level.
        // see also ../doc/smudge_math.png
        eraser_target_alpha = (1-fac)*1.0 + fac*smudge_buckets[bucket][3];
        // fix rounding errors (they really seem to happen in the previous line)
        eraser_target_alpha = CLAMP(eraser_target_alpha, 0.0, 1.0);
        if (eraser_target_alpha > 0) {
          
          float smudge_state[4] = {smudge_buckets[bucket][0], smudge_buckets[bucket][1], smudge_buckets[bucket][2], smudge_buckets[bucket][3]};
          float brush_color[4] = {color_h, color_s, color_v, eraser_target_alpha};
          float *color_new;
          
          color_new = mix_colors(self, smudge_state, brush_color, fac, self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_GAMMA], self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_NORMAL_SUB], self->settings_value[MYPAINT_BRUSH_SETTING_SMUDGE_SPECTRAL], 0);  
          
          color_h = color_new[0];
          color_s = color_new[1];
          color_v = color_new[2];
       
        } else {
          // we are only erasing; the color does not matter
          color_h = 1.0;
          color_s = 0.0;
          color_v = 0.0;
        }
        
      rgb_to_hsv_float (&color_h, &color_s, &color_v);
    }

    // eraser
    if (self->settings_value[MYPAINT_BRUSH_SETTING_ERASER]) {
      eraser_target_alpha *= (1.0-self->settings_value[MYPAINT_BRUSH_SETTING_ERASER]);
    }

    // HSV color change
    color_h += self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_H];
    color_s += color_s * color_v * self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_HSV_S];
    color_v += self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_V];

    // HSL color change
    if (self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_L] || self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_HSL_S]) {
      // (calculating way too much here, can be optimized if neccessary)
      // this function will CLAMP the inputs
      hsv_to_rgb_float (&color_h, &color_s, &color_v);
      rgb_to_hsl_float (&color_h, &color_s, &color_v);
      color_v += self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_L];
      color_s += color_s * MIN(fabsf(1.0 - color_v), fabsf(color_v)) * 2.0
        * self->settings_value[MYPAINT_BRUSH_SETTING_CHANGE_COLOR_HSL_S];
      hsl_to_rgb_float (&color_h, &color_s, &color_v);
      rgb_to_hsv_float (&color_h, &color_s, &color_v);
    }

    float hardness = CLAMP(self->settings_value[MYPAINT_BRUSH_SETTING_HARDNESS], 0.0, 1.0);

    // anti-aliasing attempt (works surprisingly well for ink brushes)
    float current_fadeout_in_pixels = radius * (1.0 - hardness);
    float min_fadeout_in_pixels = self->settings_value[MYPAINT_BRUSH_SETTING_ANTI_ALIASING];
    if (current_fadeout_in_pixels < min_fadeout_in_pixels) {
      // need to soften the brush (decrease hardness), but keep optical radius
      // so we tune both radius and hardness, to get the desired fadeout_in_pixels
      float current_optical_radius = radius - (1.0-hardness)*radius/2.0;

      // Equation 1: (new fadeout must be equal to min_fadeout)
      //   min_fadeout_in_pixels = radius_new*(1.0 - hardness_new)
      // Equation 2: (optical radius must remain unchanged)
      //   current_optical_radius = radius_new - (1.0-hardness_new)*radius_new/2.0
      //
      // Solved Equation 1 for hardness_new, using Equation 2: (thanks to mathomatic)
      float hardness_new = ((current_optical_radius - (min_fadeout_in_pixels/2.0))/(current_optical_radius + (min_fadeout_in_pixels/2.0)));
      // Using Equation 1:
      float radius_new = (min_fadeout_in_pixels/(1.0 - hardness_new));

      hardness = hardness_new;
      radius = radius_new;
    }

    // snap to pixel
    float snapToPixel = self->settings_value[MYPAINT_BRUSH_SETTING_SNAP_TO_PIXEL];
    if (snapToPixel > 0.0)
    {
      // linear interpolation between non-snapped and snapped
      float snapped_x = floor(x) + 0.5;
      float snapped_y = floor(y) + 0.5;
      x = x + (snapped_x - x) * snapToPixel;
      y = y + (snapped_y - y) * snapToPixel;

      float snapped_radius = roundf(radius * 2.0) / 2.0;
      if (snapped_radius < 0.5)
        snapped_radius = 0.5;

      if (snapToPixel > 0.9999 )
      {
        snapped_radius -= 0.0001; // this fixes precision issues where
                                  // neighboor pixels could be wrongly painted
      }

      radius = radius + (snapped_radius - radius) * snapToPixel;
    }

    // the functions below will CLAMP most inputs
    hsv_to_rgb_float (&color_h, &color_s, &color_v);
    return mypaint_surface_draw_dab (surface, x, y, radius, color_h, color_s, color_v, opaque, hardness, eraser_target_alpha,
                              self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_RATIO], self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_ANGLE],
                              self->settings_value[MYPAINT_BRUSH_SETTING_LOCK_ALPHA],
                              self->settings_value[MYPAINT_BRUSH_SETTING_COLORIZE]);
  }

  // How many dabs will be drawn between the current and the next (x, y, pressure, +dt) position?
  float count_dabs_to (MyPaintBrush *self, float x, float y, float pressure, float dt)
  {
    float xx, yy;
    float res1, res2, res3;
    float dist;

    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] == 0.0) self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] = expf(mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC]));
    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] < ACTUAL_RADIUS_MIN) self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] = ACTUAL_RADIUS_MIN;
    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] > ACTUAL_RADIUS_MAX) self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] = ACTUAL_RADIUS_MAX;


    // OPTIMIZE: expf() called too often
    float base_radius = expf(mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC]));
    if (base_radius < ACTUAL_RADIUS_MIN) base_radius = ACTUAL_RADIUS_MIN;
    if (base_radius > ACTUAL_RADIUS_MAX) base_radius = ACTUAL_RADIUS_MAX;
    //if (base_radius < 0.5) base_radius = 0.5;
    //if (base_radius > 500.0) base_radius = 500.0;

    xx = x - self->states[MYPAINT_BRUSH_STATE_X];
    yy = y - self->states[MYPAINT_BRUSH_STATE_Y];
    //dp = pressure - pressure; // Not useful?
    // TODO: control rate with pressure (dabs per pressure) (dpressure is useless)

    if (self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_RATIO] > 1.0) {
      // code duplication, see tiledsurface::draw_dab()
      float angle_rad=self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_ANGLE]/360*2*M_PI;
      float cs=cos(angle_rad);
      float sn=sin(angle_rad);
      float yyr=(yy*cs-xx*sn)*self->states[MYPAINT_BRUSH_STATE_ACTUAL_ELLIPTICAL_DAB_RATIO];
      float xxr=yy*sn+xx*cs;
      dist = sqrt(yyr*yyr + xxr*xxr);
    } else {
      dist = hypotf(xx, yy);
    }

    // FIXME: no need for base_value or for the range checks above IF always the interpolation
    //        function will be called before this one
    res1 = dist / self->states[MYPAINT_BRUSH_STATE_ACTUAL_RADIUS] * mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS]);
    res2 = dist / base_radius   * mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_DABS_PER_BASIC_RADIUS]);
    res3 = dt * mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_DABS_PER_SECOND]);
    return res1 + res2 + res3;
  }

  /**
   * mypaint_brush_stroke_to:
   * @dtime: Time since last motion event, in seconds.
   *
   * Should be called once for each motion event.
   *
   * Returns: non-0 if the stroke is finished or empty, else 0.
   */
  int mypaint_brush_stroke_to (MyPaintBrush *self, MyPaintSurface *surface,
                                float x, float y, float pressure,
                                float xtilt, float ytilt, double dtime, float viewzoom, float viewrotation)
  {
    const float max_dtime = 5;

    //printf("%f %f %f %f\n", (double)dtime, (double)x, (double)y, (double)pressure);

    float tilt_ascension = 0.0;
    float tilt_declination = 90.0;
    if (xtilt != 0 || ytilt != 0) {
      // shield us from insane tilt input
      xtilt = CLAMP(xtilt, -1.0, 1.0);
      ytilt = CLAMP(ytilt, -1.0, 1.0);
      assert(isfinite(xtilt) && isfinite(ytilt));

      tilt_ascension = 180.0*atan2(-xtilt, ytilt)/M_PI;
      const float rad = hypot(xtilt, ytilt);
      tilt_declination = 90-(rad*60);

      assert(isfinite(tilt_ascension));
      assert(isfinite(tilt_declination));
    }

    // printf("xtilt %f, ytilt %f\n", (double)xtilt, (double)ytilt);
    // printf("ascension %f, declination %f\n", (double)tilt_ascension, (double)tilt_declination);

    if (pressure <= 0.0) pressure = 0.0;
    if (!isfinite(x) || !isfinite(y) ||
        (x > 1e10 || y > 1e10 || x < -1e10 || y < -1e10)) {
      // workaround attempt for https://gna.org/bugs/?14372
      printf("Warning: ignoring brush::stroke_to with insane inputs (x = %f, y = %f)\n", (double)x, (double)y);
      x = 0.0;
      y = 0.0;
      pressure = 0.0;
      viewzoom = 0.0;
      viewrotation = 0.0;
    }
    // the assertion below is better than out-of-memory later at save time
    assert(x < 1e8 && y < 1e8 && x > -1e8 && y > -1e8);

    if (dtime < 0) printf("Time jumped backwards by dtime=%f seconds!\n", dtime);
    if (dtime <= 0) dtime = 0.0001; // protect against possible division by zero bugs

    /* way too slow with the new rng, and not working any more anyway...
    rng_double_set_seed (self->rng, self->states[MYPAINT_BRUSH_STATE_RNG_SEED]*0x40000000);
    */

    if (dtime > 0.100 && pressure && self->states[MYPAINT_BRUSH_STATE_PRESSURE] == 0) {
      // Workaround for tablets that don't report motion events without pressure.
      // This is to avoid linear interpolation of the pressure between two events.
      mypaint_brush_stroke_to (self, surface, x, y, 0.0, 90.0, 0.0, dtime-0.0001, viewzoom, viewrotation);
      dtime = 0.0001;
    }

    // skip some length of input if requested (for stable tracking noise)
    if (self->skip > 0.001) {
      float dist = hypotf(self->skip_last_x-x, self->skip_last_y-y);
      self->skip_last_x = x;
      self->skip_last_y = y;
      self->skipped_dtime += dtime;
      self->skip -= dist;
      dtime = self->skipped_dtime;

      if (self->skip > 0.001 && !(dtime > max_dtime || self->reset_requested))
        return TRUE;

      // skipped
      self->skip = 0;
      self->skip_last_x = 0;
      self->skip_last_y = 0;
      self->skipped_dtime = 0;
    }


    { // calculate the actual "virtual" cursor position

      // noise first
      if (mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_TRACKING_NOISE])) {
        // OPTIMIZE: expf() called too often
        const float base_radius = expf(mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC]));
        const float noise = base_radius * mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_TRACKING_NOISE]);

        if (noise > 0.001) {
          // we need to skip some length of input to make
          // tracking noise independent from input frequency
          self->skip = 0.5*noise;
          self->skip_last_x = x;
          self->skip_last_y = y;

          // add noise
          x += noise * rand_gauss(self->rng);
          y += noise * rand_gauss(self->rng);
        }
      }

      const float fac = 1.0 - exp_decay (mypaint_mapping_get_base_value(self->settings[MYPAINT_BRUSH_SETTING_SLOW_TRACKING]), 100.0*dtime);
      x = self->states[MYPAINT_BRUSH_STATE_X] + (x - self->states[MYPAINT_BRUSH_STATE_X]) * fac;
      y = self->states[MYPAINT_BRUSH_STATE_Y] + (y - self->states[MYPAINT_BRUSH_STATE_Y]) * fac;
    }

    // draw many (or zero) dabs to the next position

    // see doc/images/stroke2dabs.png
    float dabs_moved = self->states[MYPAINT_BRUSH_STATE_PARTIAL_DABS];
    float dabs_todo = count_dabs_to (self, x, y, pressure, dtime);

    if (dtime > max_dtime || self->reset_requested) {
      self->reset_requested = FALSE;

      // reset skipping
      self->skip = 0;
      self->skip_last_x = 0;
      self->skip_last_y = 0;
      self->skipped_dtime = 0;

      // reset value of random input
      self->random_input = rng_double_next(self->rng);

      //printf("Brush reset.\n");
      int i=0;
      for (i=0; i<MYPAINT_BRUSH_STATES_COUNT; i++) {
        self->states[i] = 0;
      }

      self->states[MYPAINT_BRUSH_STATE_X] = x;
      self->states[MYPAINT_BRUSH_STATE_Y] = y;
      self->states[MYPAINT_BRUSH_STATE_PRESSURE] = pressure;

      // not resetting, because they will get overwritten below:
      //dx, dy, dpress, dtime

      self->states[MYPAINT_BRUSH_STATE_ACTUAL_X] = self->states[MYPAINT_BRUSH_STATE_X];
      self->states[MYPAINT_BRUSH_STATE_ACTUAL_Y] = self->states[MYPAINT_BRUSH_STATE_Y];
      self->states[MYPAINT_BRUSH_STATE_STROKE] = 1.0; // start in a state as if the stroke was long finished

      return TRUE;
    }

    enum { UNKNOWN, YES, NO } painted = UNKNOWN;
    double dtime_left = dtime;

    float step_ddab, step_dx, step_dy, step_dpressure, step_dtime;
    float step_declination, step_ascension, step_viewzoom, step_viewrotation;
    while (dabs_moved + dabs_todo >= 1.0) { // there are dabs pending
      { // linear interpolation (nonlinear variant was too slow, see SVN log)
        float frac; // fraction of the remaining distance to move
        if (dabs_moved > 0) {
          // "move" the brush exactly to the first dab
          step_ddab = 1.0 - dabs_moved; // the step "moves" the brush by a fraction of one dab
          dabs_moved = 0;
        } else {
          step_ddab = 1.0; // the step "moves" the brush by exactly one dab
        }
        frac = step_ddab / dabs_todo;
        step_dx        = frac * (x - self->states[MYPAINT_BRUSH_STATE_X]);
        step_dy        = frac * (y - self->states[MYPAINT_BRUSH_STATE_Y]);
        step_dpressure = frac * (pressure - self->states[MYPAINT_BRUSH_STATE_PRESSURE]);
        step_dtime     = frac * (dtime_left - 0.0);
        // Though it looks different, time is interpolated exactly like x/y/pressure.
        step_declination = frac * (tilt_declination - self->states[MYPAINT_BRUSH_STATE_DECLINATION]);
        step_ascension   = frac * smallest_angular_difference(self->states[MYPAINT_BRUSH_STATE_ASCENSION], tilt_ascension);
        step_viewzoom = viewzoom;
        step_viewrotation = viewrotation;
      }

      update_states_and_setting_values (self, step_ddab, step_dx, step_dy, step_dpressure, step_declination, step_ascension, step_dtime, step_viewzoom, step_viewrotation);
      gboolean painted_now = prepare_and_draw_dab (self, surface);
      if (painted_now) {
        painted = YES;
      } else if (painted == UNKNOWN) {
        painted = NO;
      }

      // update value of random input only when draw the dab
      self->random_input = rng_double_next(self->rng);

      dtime_left   -= step_dtime;
      dabs_todo  = count_dabs_to (self, x, y, pressure, dtime_left);
    }

    {
      // "move" the brush to the current time (no more dab will happen)
      // Important to do this at least once every event, because
      // brush_count_dabs_to depends on the radius and the radius can
      // depend on something that changes much faster than just every
      // dab.

      step_ddab = dabs_todo; // the step "moves" the brush by a fraction of one dab
      step_dx        = x - self->states[MYPAINT_BRUSH_STATE_X];
      step_dy        = y - self->states[MYPAINT_BRUSH_STATE_Y];
      step_dpressure = pressure - self->states[MYPAINT_BRUSH_STATE_PRESSURE];
      step_declination = tilt_declination - self->states[MYPAINT_BRUSH_STATE_DECLINATION];
      step_ascension = smallest_angular_difference(self->states[MYPAINT_BRUSH_STATE_ASCENSION], tilt_ascension);
      step_dtime     = dtime_left;
      step_viewzoom  = viewzoom;
      step_viewrotation = viewrotation;

      //dtime_left = 0; but that value is not used any more

      update_states_and_setting_values (self, step_ddab, step_dx, step_dy, step_dpressure, step_declination, step_ascension, step_dtime, step_viewzoom, step_viewrotation);
    }

    // save the fraction of a dab that is already done now
    self->states[MYPAINT_BRUSH_STATE_PARTIAL_DABS] = dabs_moved + dabs_todo;

    /* not working any more with the new rng...
    // next seed for the RNG (GRand has no get_state() and states[] must always contain our full state)
    self->states[MYPAINT_BRUSH_STATE_RNG_SEED] = rng_double_next(self->rng);
    */

    // stroke separation logic (for undo/redo)

    if (painted == UNKNOWN) {
      if (self->stroke_current_idling_time > 0 || self->stroke_total_painting_time == 0) {
        // still idling
        painted = NO;
      } else {
        // probably still painting (we get more events than brushdabs)
        painted = YES;
        //if (pressure == 0) g_print ("info: assuming 'still painting' while there is no pressure\n");
      }
    }
    if (painted == YES) {
      //if (stroke_current_idling_time > 0) g_print ("idling ==> painting\n");
      self->stroke_total_painting_time += dtime;
      self->stroke_current_idling_time = 0;
      // force a stroke split after some time
      if (self->stroke_total_painting_time > 4 + 3*pressure) {
        // but only if pressure is not being released
        // FIXME: use some smoothed state for dpressure, not the output of the interpolation code
        //        (which might easily wrongly give dpressure == 0)
        if (step_dpressure >= 0) {
          return TRUE;
        }
      }
    } else if (painted == NO) {
      //if (stroke_current_idling_time == 0) g_print ("painting ==> idling\n");
      self->stroke_current_idling_time += dtime;
      if (self->stroke_total_painting_time == 0) {
        // not yet painted, start a new stroke if we have accumulated a lot of irrelevant motion events
        if (self->stroke_current_idling_time > 1.0) {
          return TRUE;
        }
      } else {
        // Usually we have pressure==0 here. But some brushes can paint
        // nothing at full pressure (eg gappy lines, or a stroke that
        // fades out). In either case this is the prefered moment to split.
        if (self->stroke_total_painting_time+self->stroke_current_idling_time > 0.9 + 5*pressure) {
          return TRUE;
        }
      }
    }
    return FALSE;
  }

#ifdef HAVE_JSON_C

// Compat wrapper, for supporting libjson
static gboolean
obj_get(json_object *self, const gchar *key, json_object **obj_out) {
#if JSON_C_MINOR_VERSION >= 10
    return json_object_object_get_ex(self, key, obj_out)
        && (obj_out ? (*obj_out != NULL) : TRUE);
#else
    json_object *o = json_object_object_get(self, key);
    if (obj_out) {
        *obj_out = o;
    }
    return (o != NULL);
#endif
}

static gboolean
update_brush_setting_from_json_object(MyPaintBrush *self,
                                      char *setting_name,
                                      json_object *setting_obj)
{
    MyPaintBrushSetting setting_id = mypaint_brush_setting_from_cname(setting_name);

    if (!(setting_id >= 0 && setting_id < MYPAINT_BRUSH_SETTINGS_COUNT)) {
        fprintf(stderr, "Warning: Unknown setting_id: %d for setting: %s\n",
                setting_id, setting_name);
        return FALSE;
    }

    if (!json_object_is_type(setting_obj, json_type_object)) {
        fprintf(stderr, "Warning: Wrong type for setting: %s\n", setting_name);
        return FALSE;
    }

    // Base value
    json_object *base_value_obj = NULL;
    if (! obj_get(setting_obj, "base_value", &base_value_obj)) {
        fprintf(stderr, "Warning: No 'base_value' field for setting: %s\n", setting_name);
        return FALSE;
    }
    const double base_value = json_object_get_double(base_value_obj);
    mypaint_brush_set_base_value(self, setting_id, base_value);

    // Inputs
    json_object *inputs = NULL;
    if (! obj_get(setting_obj, "inputs", &inputs)) {
        fprintf(stderr, "Warning: No 'inputs' field for setting: %s\n", setting_name);
        return FALSE;
    }
    json_object_object_foreach(inputs, input_name, input_obj) {
        MyPaintBrushInput input_id = mypaint_brush_input_from_cname(input_name);

        if (!json_object_is_type(input_obj, json_type_array)) {
            fprintf(stderr, "Warning: Wrong inputs type for setting: %s\n", setting_name);
            return FALSE;
        }

        const int number_of_mapping_points = json_object_array_length(input_obj);

        mypaint_brush_set_mapping_n(self, setting_id, input_id, number_of_mapping_points);

        for (int i=0; i<number_of_mapping_points; i++) {
            json_object *mapping_point = json_object_array_get_idx(input_obj, i);

            json_object *x_obj = json_object_array_get_idx(mapping_point, 0);
            const float x = json_object_get_double(x_obj);
            json_object *y_obj = json_object_array_get_idx(mapping_point, 1);
            const float y = json_object_get_double(y_obj);

            mypaint_brush_set_mapping_point(self, setting_id, input_id, i, x, y);
        }
    }

    return TRUE;
}

static gboolean
update_brush_from_json_object(MyPaintBrush *self)
{
    // Check version
    json_object *version_object = NULL;
    if (! obj_get(self->brush_json, "version", &version_object)) {
        fprintf(stderr, "Error: No 'version' field for brush\n");
        return FALSE;
    }
    const int version = json_object_get_int(version_object);
    if (version != 3) {
        fprintf(stderr, "Error: Unsupported brush setting version: %d\n", version);
        return FALSE;
    }

    // Set settings
    json_object *settings = NULL;
    if (! obj_get(self->brush_json, "settings", &settings)) {
        fprintf(stderr, "Error: No 'settings' field for brush\n");
        return FALSE;
    }

    gboolean updated_any = FALSE;
    gboolean updated_all = TRUE;
    json_object_object_foreach(settings, setting_name, setting_obj) {
        if (update_brush_setting_from_json_object(self, setting_name, setting_obj)) {
            updated_any = TRUE;
        }
        else {
            updated_all = FALSE;
        }
    }
    return updated_any;
}
#endif // HAVE_JSON_C

gboolean
mypaint_brush_from_string(MyPaintBrush *self, const char *string)
{
#ifdef HAVE_JSON_C
    json_object *brush_json = NULL;

    if (self->brush_json) {
        // Free
        json_object_put(self->brush_json);
        self->brush_json = NULL;
    }
    if (string) {
        brush_json = json_tokener_parse(string);
    }

    if (brush_json) {
        self->brush_json = brush_json;
        return update_brush_from_json_object(self);
    }
    else {
        self->brush_json = json_object_new_object();
        return FALSE;
    }
#else
    return FALSE;
#endif
}


void
mypaint_brush_from_defaults(MyPaintBrush *self) {
    for (int s = 0; s < MYPAINT_BRUSH_SETTINGS_COUNT; s++) {
        for (int i = 0; i < MYPAINT_BRUSH_INPUTS_COUNT; i++) {
            mypaint_brush_set_mapping_n(self, s, i, 0);
        }

        const float def = mypaint_brush_setting_info(s)->def;
        mypaint_brush_set_base_value(self, s, def);
    }

    mypaint_brush_set_mapping_n(self, MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY, MYPAINT_BRUSH_INPUT_PRESSURE, 2);
    mypaint_brush_set_mapping_point(self, MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY, MYPAINT_BRUSH_INPUT_PRESSURE, 0, 0.0, 0.0);
    mypaint_brush_set_mapping_point(self, MYPAINT_BRUSH_SETTING_OPAQUE_MULTIPLY, MYPAINT_BRUSH_INPUT_PRESSURE, 1, 1.0, 1.0);
}
