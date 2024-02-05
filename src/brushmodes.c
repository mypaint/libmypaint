/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2007-2014 Martin Renold <martinxyz@gmx.ch> et. al
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

#include "config.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "fastapprox/fastpow.h"

#include "brushmodes.h"
#include "helpers.h"

// parameters to those methods:
//
// rgba: A pointer to 16bit rgba data with premultiplied alpha.
//       The range of each components is limited from 0 to 2^15.
//
// mask: Contains the dab shape, that is, the intensity of the dab at
//       each pixel. Usually rendering is done for one tile at a
//       time. The mask is LRE encoded to jump quickly over regions
//       that are not affected by the dab.
//
// opacity: overall strength of the blending mode. Has the same
//          influence on the dab as the values inside the mask.


// We are manipulating pixels with premultiplied alpha directly.
// This is an "over" operation (opa = topAlpha).
// In the formula below, topColor is assumed to be premultiplied.
//
//               opa_a      <   opa_b      >
// resultAlpha = topAlpha + (1.0 - topAlpha) * bottomAlpha
// resultColor = topColor + (1.0 - topAlpha) * bottomColor
//

void draw_dab_pixels_BlendMode_Normal (uint16_t * mask,
                                       uint16_t * rgba,
                                       uint16_t color_r,
                                       uint16_t color_g,
                                       uint16_t color_b,
                                       uint16_t opacity) {

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      rgba[3] = opa_a + opa_b * rgba[3] / (1<<15);
      rgba[0] = (opa_a*color_r + opa_b*rgba[0])/(1<<15);
      rgba[1] = (opa_a*color_g + opa_b*rgba[1])/(1<<15);
      rgba[2] = (opa_a*color_b + opa_b*rgba[2])/(1<<15);

    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

void draw_dab_pixels_BlendMode_Normal_Paint (uint16_t * mask,
                                       uint16_t * rgba,
                                       uint16_t color_r,
                                       uint16_t color_g,
                                       uint16_t color_b,
                                       uint16_t opacity) {

  // convert top to spectral.  Already straight color
  float spectral_a[10] = {0};
  rgb_to_spectral((float)color_r / (1 << 15), (float)color_g / (1 << 15), (float)color_b / (1 << 15), spectral_a);
  // pigment-mode does not like very low opacity, probably due to rounding
  // errors with int->float->int round-trip.  Once we convert to pure
  // float engine this might be fixed.  For now enforce a minimum opacity:
  opacity = MAX(opacity, 150);

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      // optimization- if background has 0 alpha we can just do normal additive
      // blending since there is nothing to mix with.
      if (rgba[3] <= 0) {
        rgba[3] = opa_a + opa_b * rgba[3] / (1<<15);
        rgba[0] = (opa_a*color_r + opa_b*rgba[0])/(1<<15);
        rgba[1] = (opa_a*color_g + opa_b*rgba[1])/(1<<15);
        rgba[2] = (opa_a*color_b + opa_b*rgba[2])/(1<<15);
        continue;
      }
      //alpha-weighted ratio for WGM (sums to 1.0)
      float fac_a = (float)opa_a / (opa_a + opa_b * rgba[3] / (1<<15));
      float fac_b = 1.0 - fac_a;

      //convert bottom to spectral.  Un-premult alpha to obtain reflectance
      //color noise is not a problem since low alpha also implies low weight
      float spectral_b[10] = {0};

      rgb_to_spectral((float)rgba[0] / rgba[3], (float)rgba[1] / rgba[3], (float)rgba[2] / rgba[3], spectral_b);

      // mix to the two spectral reflectances using WGM
      float spectral_result[10] = {0};
      for (int i=0; i<10; i++) {
        spectral_result[i] = fastpow(spectral_a[i], fac_a) * fastpow(spectral_b[i], fac_b);
      }

      // convert back to RGB and premultiply alpha
      float rgb_result[3] = {0};
      spectral_to_rgb(spectral_result, rgb_result);
      rgba[3] = opa_a + opa_b * rgba[3] / (1<<15);

      for (int i=0; i<3; i++) {
        rgba[i] =(rgb_result[i] * rgba[3]) + 0.5;
      }
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

//Posterize.  Basically exactly like GIMP's posterize
//reduces colors by adjustable amount (posterize_num).
//posterize the canvas, then blend that via opacity
//does not affect alpha

void draw_dab_pixels_BlendMode_Posterize (uint16_t * mask,
                                       uint16_t * rgba,
                                       uint16_t opacity,
                                       uint16_t posterize_num) {

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
     
      float r = (float)rgba[0] / (1<<15);
      float g = (float)rgba[1] / (1<<15);
      float b = (float)rgba[2] / (1<<15);

      uint32_t post_r = (1<<15) * ROUND(r * posterize_num) / posterize_num;
      uint32_t post_g = (1<<15) * ROUND(g * posterize_num) / posterize_num;
      uint32_t post_b = (1<<15) * ROUND(b * posterize_num) / posterize_num;
      
      uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      rgba[0] = (opa_a*post_r + opa_b*rgba[0])/(1<<15);
      rgba[1] = (opa_a*post_g + opa_b*rgba[1])/(1<<15);
      rgba[2] = (opa_a*post_b + opa_b*rgba[2])/(1<<15);

    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

// Colorize: apply the source hue and saturation, retaining the target
// brightness. Same thing as in the PDF spec addendum, and upcoming SVG
// compositing drafts. Colorize should be used at either 1.0 or 0.0, values in
// between probably aren't very useful. This blend mode retains the target
// alpha, and any pure whites and blacks in the target layer.

#define MAX3(a, b, c) ((a)>(b)?MAX((a),(c)):MAX((b),(c)))
#define MIN3(a, b, c) ((a)<(b)?MIN((a),(c)):MIN((b),(c)))

// For consistency, these are the values used by MyPaint's Color and
// Luminosity layer blend modes, which in turn are defined by
// http://dvcs.w3.org/hg/FXTF/rawfile/tip/compositing/index.html.
// Same as ITU Rec. BT.601 (SDTV) rounded to 2 decimal places.

static const float LUMA_RED_COEFF   = 0.2126 * (1<<15);
static const float LUMA_GREEN_COEFF = 0.7152 * (1<<15);
static const float LUMA_BLUE_COEFF  = 0.0722 * (1<<15);

// See also http://en.wikipedia.org/wiki/YCbCr


/* Returns the sRGB luminance of an RGB triple, expressed as scaled ints. */

#define LUMA(r,g,b) \
   ((r)*LUMA_RED_COEFF + (g)*LUMA_GREEN_COEFF + (b)*LUMA_BLUE_COEFF)


/*
 * Sets the output RGB triple's luminance to that of the input, retaining its
 * colour. Inputs and outputs are scaled ints having factor 2**-15, and must
 * not store premultiplied alpha.
 */

inline static void
set_rgb16_lum_from_rgb16(const uint16_t topr,
                         const uint16_t topg,
                         const uint16_t topb,
                         uint16_t *botr,
                         uint16_t *botg,
                         uint16_t *botb)
{
    // Spec: SetLum()
    // Colours potentially can go out of band to both sides, hence the
    // temporary representation inflation.
    const uint16_t botlum = LUMA(*botr, *botg, *botb) / (1<<15);
    const uint16_t toplum = LUMA(topr, topg, topb) / (1<<15);
    const int16_t diff = botlum - toplum;
    int32_t r = topr + diff;
    int32_t g = topg + diff;
    int32_t b = topb + diff;

    // Spec: ClipColor()
    // Clip out of band values
    int32_t lum = LUMA(r, g, b) / (1<<15);
    int32_t cmin = MIN3(r, g, b);
    int32_t cmax = MAX3(r, g, b);
    if (cmin < 0) {
        r = lum + (((r - lum) * lum) / (lum - cmin));
        g = lum + (((g - lum) * lum) / (lum - cmin));
        b = lum + (((b - lum) * lum) / (lum - cmin));
    }
    if (cmax > (1<<15)) {
        r = lum + (((r - lum) * ((1<<15)-lum)) / (cmax - lum));
        g = lum + (((g - lum) * ((1<<15)-lum)) / (cmax - lum));
        b = lum + (((b - lum) * ((1<<15)-lum)) / (cmax - lum));
    }
#ifdef HEAVY_DEBUG
    assert((0 <= r) && (r <= (1<<15)));
    assert((0 <= g) && (g <= (1<<15)));
    assert((0 <= b) && (b <= (1<<15)));
#endif

    *botr = r;
    *botg = g;
    *botb = b;
}


// The method is an implementation of that described in the official Adobe "PDF
// Blend Modes: Addendum" document, dated January 23, 2006; specifically it's
// the "Color" nonseparable blend mode. We do however use different
// coefficients for the Luma value.

void
draw_dab_pixels_BlendMode_Color (uint16_t *mask,
                                 uint16_t *rgba, // b=bottom, premult
                                 uint16_t color_r,  // }
                                 uint16_t color_g,  // }-- a=top, !premult
                                 uint16_t color_b,  // }
                                 uint16_t opacity)
{
  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      // De-premult
      uint16_t r, g, b;
      const uint16_t a = rgba[3];
      r = g = b = 0;
      if (rgba[3] != 0) {
        r = ((1<<15)*((uint32_t)rgba[0])) / a;
        g = ((1<<15)*((uint32_t)rgba[1])) / a;
        b = ((1<<15)*((uint32_t)rgba[2])) / a;
      }

      // Apply luminance
      set_rgb16_lum_from_rgb16(color_r, color_g, color_b, &r, &g, &b);

      // Re-premult
      r = ((uint32_t) r) * a / (1<<15);
      g = ((uint32_t) g) * a / (1<<15);
      b = ((uint32_t) b) * a / (1<<15);

      // And combine as normal.
      uint32_t opa_a = mask[0] * opacity / (1<<15); // topAlpha
      uint32_t opa_b = (1<<15) - opa_a; // bottomAlpha
      rgba[0] = (opa_a*r + opa_b*rgba[0])/(1<<15);
      rgba[1] = (opa_a*g + opa_b*rgba[1])/(1<<15);
      rgba[2] = (opa_a*b + opa_b*rgba[2])/(1<<15);
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

// This blend mode is used for smudging and erasing.  Smudging
// allows to "drag" around transparency as if it was a color.  When
// smuding over a region that is 60% opaque the result will stay 60%
// opaque (color_a=0.6).  For normal erasing color_a is set to 0.0
// and color_r/g/b will be ignored. This function can also do normal
// blending (color_a=1.0).
//
void draw_dab_pixels_BlendMode_Normal_and_Eraser (uint16_t * mask,
                                                  uint16_t * rgba,
                                                  uint16_t color_r,
                                                  uint16_t color_g,
                                                  uint16_t color_b,
                                                  uint16_t color_a,
                                                  uint16_t opacity) {

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      opa_a = opa_a * color_a / (1<<15);
      rgba[3] = opa_a + opa_b * rgba[3] / (1<<15);
      rgba[0] = (opa_a*color_r + opa_b*rgba[0])/(1<<15);
      rgba[1] = (opa_a*color_g + opa_b*rgba[1])/(1<<15);
      rgba[2] = (opa_a*color_b + opa_b*rgba[2])/(1<<15);

    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};


// Fast sigmoid-like function with constant offsets, used to get a
// fairly smooth transition between additive and spectral blending.
float spectral_blend_factor(float x) {
  const float ver_fac = 1.65; // vertical compression factor
  const float hor_fac = 8.0f; // horizontal compression factor
  const float hor_offs = 3.0f; // horizontal offset (slightly left of center)
  const float b = x * hor_fac - hor_offs;
  return 0.5 + b / (1 + fabsf(b) * ver_fac);
}

void draw_dab_pixels_BlendMode_Normal_and_Eraser_Paint (uint16_t * mask,
                                                  uint16_t * rgba,
                                                  uint16_t color_r,
                                                  uint16_t color_g,
                                                  uint16_t color_b,
                                                  uint16_t color_a,
                                                  uint16_t opacity) {

  // Convert input color to spectral, it is not premultiplied
  float spectral_a[10] = {0};
  rgb_to_spectral(
    (float)color_r / (1<<15),
    (float)color_g / (1<<15),
    (float)color_b / (1<<15),
    spectral_a
    );

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      const uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      const uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      const uint32_t opa_a2 = opa_a * color_a / (1<<15); // erase-adjusted alpha
      const uint32_t opa_out = opa_a2 + opa_b * rgba[3] / (1<<15);

      uint32_t rgb[3] = {0, 0, 0};

      // Spectral blending does not handle low transparency well, so we try to patch that
      // up by using mostly additive mixing for lower canvas alphas, gradually moving to
      // full spectral blending at mostly opaque pixels.
      //
      // This does not solve all problems with low opacity, and it creates some new ones
      // when mixing bright low-opacity colors into dark low-opacity colors, but the new
      // artifacts are not as tough to deal with as the old dark-fringe artifacts.
      float spectral_factor = CLAMP(spectral_blend_factor((float)rgba[3] / (1<<15)), 0.0f, 1.0f);
      float additive_factor = 1.0 - spectral_factor;

      if (additive_factor) {
        rgb[0] = (opa_a2 * color_r + opa_b * rgba[0]) / (1 << 15);
        rgb[1] = (opa_a2 * color_g + opa_b * rgba[1]) / (1 << 15);
        rgb[2] = (opa_a2 * color_b + opa_b * rgba[2]) / (1 << 15);
      }

      if (spectral_factor && rgba[3] != 0) {
        // Convert straightened tile pixel color to a spectral
        float spectral_b[10] = {0};
        rgb_to_spectral(
          (float)rgba[0] / rgba[3],
          (float)rgba[1] / rgba[3],
          (float)rgba[2] / rgba[3],
          spectral_b
          );

        float fac_a = (float)opa_a / (opa_a + opa_b * rgba[3] / (1 << 15));
        fac_a *= (float)color_a / (1 << 15);
        float fac_b = 1.0 - fac_a;

        // Mix input and tile pixel colors using WGM
        float spectral_result[10] = {0};
        for (int i = 0; i < 10; i++) {
          spectral_result[i] =
              fastpow(spectral_a[i], fac_a) * fastpow(spectral_b[i], fac_b);
        }

        // Convert back to RGB
        float rgb_result[3] = {0};
        spectral_to_rgb(spectral_result, rgb_result);

        for (int i = 0; i < 3; i++) {
          rgb[i] = (additive_factor * rgb[i]) + (spectral_factor * rgb_result[i] * opa_out);
        }
      }

      rgba[3] = opa_out;
      for (int i = 0; i < 3; i++) {
        rgba[i] = rgb[i];
      }
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

// This is BlendMode_Normal with locked alpha channel.
//
void draw_dab_pixels_BlendMode_LockAlpha (uint16_t * mask,
                                          uint16_t * rgba,
                                          uint16_t color_r,
                                          uint16_t color_g,
                                          uint16_t color_b,
                                          uint16_t opacity) {

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      
      opa_a *= rgba[3];
      opa_a /= (1<<15);
          
      rgba[0] = (opa_a*color_r + opa_b*rgba[0])/(1<<15);
      rgba[1] = (opa_a*color_g + opa_b*rgba[1])/(1<<15);
      rgba[2] = (opa_a*color_b + opa_b*rgba[2])/(1<<15);
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

void draw_dab_pixels_BlendMode_LockAlpha_Paint (uint16_t * mask,
                                          uint16_t * rgba,
                                          uint16_t color_r,
                                          uint16_t color_g,
                                          uint16_t color_b,
                                          uint16_t opacity) {

  // convert top to spectral.  Already straight color
  float spectral_a[10] = {0};
  rgb_to_spectral((float)color_r / (1<<15), (float)color_g / (1<<15), (float)color_b / (1<<15), spectral_a);
  opacity = MAX(opacity, 150);

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      uint32_t opa_a = mask[0]*(uint32_t)opacity/(1<<15); // topAlpha
      uint32_t opa_b = (1<<15)-opa_a; // bottomAlpha
      opa_a *= rgba[3];
      opa_a /= (1<<15);
      if (rgba[3] <= 0) {
        rgba[0] = (opa_a*color_r + opa_b*rgba[0])/(1<<15);
        rgba[1] = (opa_a*color_g + opa_b*rgba[1])/(1<<15);
        rgba[2] = (opa_a*color_b + opa_b*rgba[2])/(1<<15);
        continue;
      }
      float fac_a = (float)opa_a / (opa_a + opa_b * rgba[3] / (1<<15));
      float fac_b = 1.0 - fac_a;
      float spectral_b[10] = {0};
      rgb_to_spectral((float)rgba[0] / rgba[3], (float)rgba[1] / rgba[3], (float)rgba[2] / rgba[3], spectral_b);

      // mix to the two spectral colors using WGM
      float spectral_result[10] = {0};
      for (int i=0; i<10; i++) {
        spectral_result[i] = fastpow(spectral_a[i], fac_a) * fastpow(spectral_b[i], fac_b);
      }
      // convert back to RGB
      float rgb_result[3] = {0};
      spectral_to_rgb(spectral_result, rgb_result);

      for (int i=0; i<3; i++) {
        rgba[i] =(rgb_result[i] * rgba[3]) + 0.5;
      }
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};

void get_color_pixels_legacy (
    uint16_t * mask,
    uint16_t * rgba,
    float * sum_weight,
    float * sum_r,
    float * sum_g,
    float * sum_b,
    float * sum_a
    )
{
    // The sum of a 64x64 tile fits into a 32 bit integer, but the sum
    // of an arbitrary number of tiles may not fit. We assume that we
    // are processing a single tile at a time, so we can use integers.
    // But for the result we need floats.
    uint32_t weight = 0;
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t a = 0;

    while (1) {
        for (; mask[0]; mask++, rgba+=4) {
            uint32_t opa = mask[0];
            weight += opa;
            r      += opa*rgba[0]/(1<<15);
            g      += opa*rgba[1]/(1<<15);
            b      += opa*rgba[2]/(1<<15);
            a      += opa*rgba[3]/(1<<15);

        }
        if (!mask[1]) break;
        rgba += mask[1];
        mask += 2;
    }

    // convert integer to float outside the performance critical loop
    *sum_weight += weight;
    *sum_r += r;
    *sum_g += g;
    *sum_b += b;
    *sum_a += a;
};

// Sum up the color/alpha components inside the masked region.
// Called by get_color().
//
// The sample interval guarantees that every n pixels are sampled in
// the provided mask segment.
// Setting the interval to 1 means that all pixels will be sampled,
// but note that this may result in large rounding errors.
//
// The sample rate is the probability of any pixel being sampled,
// with the exception of the guaranteed ones. Range: 0.0..1.0.
// The random sample rate can be set to 0, in which case no random
// sampling will occur.
void get_color_pixels_accumulate (uint16_t * mask,
                                  uint16_t * rgba,
                                  float * sum_weight,
                                  float * sum_r,
                                  float * sum_g,
                                  float * sum_b,
                                  float * sum_a,
                                  float paint,
                                  uint16_t sample_interval,
                                  float random_sample_rate
                                  ) {
  // Fall back to legacy sampling if using static 0 paint setting
  // Indicated by passing a negative paint factor (normal range 0..1)
  if (paint < 0.0) {
      get_color_pixels_legacy(mask, rgba, sum_weight, sum_r, sum_g, sum_b, sum_a);
      return;
  }

  // Sample the canvas as additive and subtractive
  // According to paint parameter
  // Average the results normally
  // Only sample a partially random subset of pixels

  float avg_spectral[10] = {0};
  float avg_rgb[3] = {*sum_r, *sum_g, *sum_b};
  if (paint > 0.0f) {
    rgb_to_spectral(*sum_r, *sum_g, *sum_b, avg_spectral);
  }

  // Rolling counter determining which pixels to sample
  // This sampling _is_ biased (but hopefully not too bad).
  // Ideally, the selection of pixels to be sampled should
  // be determined before this function is called.
  uint16_t interval_counter = 0;
  const int random_sample_threshold = (int)(random_sample_rate * RAND_MAX);

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      // Sample every n pixels, and a percentage of the rest.
      // At least one pixel (the first) will always be sampled.
      if (interval_counter == 0 || rand() < random_sample_threshold) {

        float a = (float)mask[0] * rgba[3] / (1 << 30);
        float alpha_sums = a + *sum_a;
        *sum_weight += (float)mask[0] / (1 << 15);
        float fac_a, fac_b;
        fac_a = fac_b = 1.0f;
        if (alpha_sums > 0.0f) {
          fac_a = a / alpha_sums;
          fac_b = 1.0 - fac_a;
        }
        if (paint > 0.0f && rgba[3] > 0) {
          float spectral[10] = {0};
          rgb_to_spectral((float)rgba[0] / rgba[3], (float)rgba[1] / rgba[3], (float)rgba[2] / rgba[3], spectral);

          for (int i = 0; i < 10; i++) {
            avg_spectral[i] = fastpow(spectral[i], fac_a) * fastpow(avg_spectral[i], fac_b);
          }
        }
        if (paint < 1.0f && rgba[3] > 0) {
          for (int i = 0; i < 3; i++) {
            avg_rgb[i] = (float)rgba[i] * fac_a / rgba[3] + (float)avg_rgb[i] * fac_b;
          }
        }
        *sum_a += a;
      }
      interval_counter = (interval_counter + 1) % sample_interval;
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
  // Convert the spectral average to rgb and write the result
  // back weighted with the rgb average.
  float spec_rgb[3] = {0};
  spectral_to_rgb(avg_spectral, spec_rgb);

  *sum_r = spec_rgb[0] * paint + (1.0 - paint) * avg_rgb[0];
  *sum_g = spec_rgb[1] * paint + (1.0 - paint) * avg_rgb[1];
  *sum_b = spec_rgb[2] * paint + (1.0 - paint) * avg_rgb[2];
};
