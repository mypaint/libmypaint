/* libmypaint - The MyPaint Brush Library
 * Copyright (C) 2007-2008 Martin Renold <martinxyz@gmx.ch>
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

#ifndef HELPERS_C
#define HELPERS_C

#include <config.h>

#include <assert.h>
#include <stdint.h>
#include <math.h>

#include "helpers.h"

float rand_gauss (RngDouble * rng)
{
  double sum = 0.0;
  sum += rng_double_next(rng);
  sum += rng_double_next(rng);
  sum += rng_double_next(rng);
  sum += rng_double_next(rng);
  return sum * 1.73205080757 - 3.46410161514;
}

float
circular_wrap(float num, float max) {
	
	if (num >= max){
		num = fmod(num, max);
	} else if (num < 0.0) {
		num = max - fabs(fmod(num,max));
	}
	if (num == max) {
		num = 0.0;
	}
	
	return num;
}

// stolen from GIMP (gimpcolorspace.c)
// (from gimp_rgb_to_hsv)
void
rgb_to_hsv_float (float *r_ /*h*/, float *g_ /*s*/, float *b_ /*v*/)
{
  float max, min, delta;
  float h, s, v;
  float r, g, b;

  h = 0.0; // silence gcc warning

  r = *r_;
  g = *g_;
  b = *b_;

  r = CLAMP(r, 0.0, 1.0);
  g = CLAMP(g, 0.0, 1.0);
  b = CLAMP(b, 0.0, 1.0);

  max = MAX3(r, g, b);
  min = MIN3(r, g, b);

  v = max;
  delta = max - min;

  if (delta > 0.0001)
    {
      s = delta / max;

      if (r == max)
        {
          h = (g - b) / delta;
          if (h < 0.0)
            h += 6.0;
        }
      else if (g == max)
        {
          h = 2.0 + (b - r) / delta;
        }
      else if (b == max)
        {
          h = 4.0 + (r - g) / delta;
        }

      h /= 6.0;
    }
  else
    {
      s = 0.0;
      h = 0.0;
    }

  *r_ = h;
  *g_ = s;
  *b_ = v;
}

// (from gimp_hsv_to_rgb)
void
hsv_to_rgb_float (float *h_, float *s_, float *v_)
{
  int    i;
  double f, w, q, t;
  float h, s, v;
  float r, g, b;
  r = g = b = 0.0; // silence gcc warning

  h = *h_;
  s = *s_;
  v = *v_;

  h = h - floor(h);
  s = CLAMP(s, 0.0, 1.0);
  v = CLAMP(v, 0.0, 1.0);

  double hue;

  if (s == 0.0)
    {
      r = v;
      g = v;
      b = v;
    }
  else
    {
      hue = h;

      if (hue == 1.0)
        hue = 0.0;

      hue *= 6.0;

      i = (int) hue;
      f = hue - i;
      w = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));

      switch (i)
        {
        case 0:
          r = v;
          g = t;
          b = w;
          break;
        case 1:
          r = q;
          g = v;
          b = w;
          break;
        case 2:
          r = w;
          g = v;
          b = t;
          break;
        case 3:
          r = w;
          g = q;
          b = v;
          break;
        case 4:
          r = t;
          g = w;
          b = v;
          break;
        case 5:
          r = v;
          g = w;
          b = q;
          break;
        }
    }

  *h_ = r;
  *s_ = g;
  *v_ = b;
}

// (from gimp_rgb_to_hsl)
void
rgb_to_hsl_float (float *r_, float *g_, float *b_)
{
  double max, min, delta;

  float h, s, l;
  float r, g, b;

  // silence gcc warnings
  h=0;

  r = *r_;
  g = *g_;
  b = *b_;

  r = CLAMP(r, 0.0, 1.0);
  g = CLAMP(g, 0.0, 1.0);
  b = CLAMP(b, 0.0, 1.0);

  max = MAX3(r, g, b);
  min = MIN3(r, g, b);

  l = (max + min) / 2.0;

  if (max == min)
    {
      s = 0.0;
      h = 0.0; //GIMP_HSL_UNDEFINED;
    }
  else
    {
      if (l <= 0.5)
        s = (max - min) / (max + min);
      else
        s = (max - min) / (2.0 - max - min);

      delta = max - min;

      if (delta == 0.0)
        delta = 1.0;

      if (r == max)
        {
          h = (g - b) / delta;
        }
      else if (g == max)
        {
          h = 2.0 + (b - r) / delta;
        }
      else if (b == max)
        {
          h = 4.0 + (r - g) / delta;
        }

      h /= 6.0;

      if (h < 0.0)
        h += 1.0;
    }

  *r_ = h;
  *g_ = s;
  *b_ = l;
}

static double
hsl_value (double n1,
           double n2,
           double hue)
{
  double val;

  if (hue > 6.0)
    hue -= 6.0;
  else if (hue < 0.0)
    hue += 6.0;

  if (hue < 1.0)
    val = n1 + (n2 - n1) * hue;
  else if (hue < 3.0)
    val = n2;
  else if (hue < 4.0)
    val = n1 + (n2 - n1) * (4.0 - hue);
  else
    val = n1;

  return val;
}


/**
 * gimp_hsl_to_rgb:
 * @hsl: A color value in the HSL colorspace
 * @rgb: The value converted to a value in the RGB colorspace
 *
 * Convert a HSL color value to an RGB color value.
 **/
void
hsl_to_rgb_float (float *h_, float *s_, float *l_)
{
  float h, s, l;
  float r, g, b;

  h = *h_;
  s = *s_;
  l = *l_;

  h = h - floor(h);
  s = CLAMP(s, 0.0, 1.0);
  l = CLAMP(l, 0.0, 1.0);

  if (s == 0)
    {
      /*  achromatic case  */
      r = l;
      g = l;
      b = l;
    }
  else
    {
      double m1, m2;

      if (l <= 0.5)
        m2 = l * (1.0 + s);
      else
        m2 = l + s - l * s;

      m1 = 2.0 * l - m2;

      r = hsl_value (m1, m2, h * 6.0 + 2.0);
      g = hsl_value (m1, m2, h * 6.0);
      b = hsl_value (m1, m2, h * 6.0 - 2.0);
    }

  *h_ = r;
  *s_ = g;
  *l_ = b;
}

void
rgb_to_hcy_float (float *r_, float *g_, float *b_) {
	
	float _HCY_RED_LUMA = 0.3;
	float _HCY_GREEN_LUMA = 0.59;
	float _HCY_BLUE_LUMA = 0.11;
	float h, c, y;
	float r, g, b;
	float p, n, d;

	r = *r_;
	g = *g_;
	b = *b_;

	// Luma is just a weighted sum of the three components.
	y = _HCY_RED_LUMA*r + _HCY_GREEN_LUMA*g + _HCY_BLUE_LUMA*b;

	// Hue. First pick a sector based on the greatest RGB component, then add
	// the scaled difference of the other two RGB components.
	p = MAX3(r, g, b);
	n = MIN3(r, g, b);
	d = p - n; // An absolute measure of chroma: only used for scaling

	if (n == p){
		h = 0.0;
	} else if (p == r){
		h = (g - b)/d;
		if (h < 0){
			h += 6.0;
		}
	} else if (p == g){
		h = ((b - r)/d) + 2.0;
	} else {  // p==b
		h = ((r - g)/d) + 4.0;
	}
	h /= 6.0;
	h = fmod(h,1.0);

	// Chroma, relative to the RGB gamut envelope.
	if ((r == g) && (g == b)){
		// Avoid a division by zero for the achromatic case.
		c = 0.0;
	} else {
		// For the derivation, see the GLHS paper.
		c = MAX((y-n)/y, (p-y)/(1-y));
	}

	*r_ = h;
	*g_ = c;
	*b_ = y;
}

void
hcy_to_rgb_float (float *h_, float *c_, float *y_) {
	
	float _HCY_RED_LUMA = 0.3;
	float _HCY_GREEN_LUMA = 0.59;
	float _HCY_BLUE_LUMA = 0.11;
	float h, c, y;
	float r, g, b;
	float th, tm;

	h = *h_;
	c = *c_;
	y = *y_;

	h = h - floor(h);
	c = CLAMP(c, 0.0, 1.0);
	y = CLAMP(y, 0.0, 1.0);

	if (c == 0)	{
	  /*  achromatic case  */
	  r = y;
	  g = y;
	  b = y;
	}

	h = fmod(h, 1.0);
	h *= 6.0;

	if (h < 1){
		// implies (p==r and h==(g-b)/d and g>=b)
		th = h;
		tm = _HCY_RED_LUMA + _HCY_GREEN_LUMA * th;
	} else if (h < 2) {
		// implies (p==g and h==((b-r)/d)+2.0 and b<r)
		th = 2.0 - h;
		tm = _HCY_GREEN_LUMA + _HCY_RED_LUMA * th;
	} else if (h < 3){
		// implies (p==g and h==((b-r)/d)+2.0 and b>=g)
		th = h - 2.0;
		tm = _HCY_GREEN_LUMA + _HCY_BLUE_LUMA * th;
	} else if (h < 4) {
		// implies (p==b and h==((r-g)/d)+4.0 and r<g)
		th = 4.0 - h;
		tm = _HCY_BLUE_LUMA + _HCY_GREEN_LUMA * th;
	} else if (h < 5){
		// implies (p==b and h==((r-g)/d)+4.0 and r>=g)
		th = h - 4.0;
		tm = _HCY_BLUE_LUMA + _HCY_RED_LUMA * th;
	} else {
		// implies (p==r and h==(g-b)/d and g<b)
		th = 6.0 - h;
		tm = _HCY_RED_LUMA + _HCY_BLUE_LUMA * th;
	}

	float n,p,o;
	// Calculate the RGB components in sorted order
	if (tm >= y){
		p = y + y*c*(1-tm)/tm;
		o = y + y*c*(th-tm)/tm;
		n = y - (y*c);
	}else{
		p = y + (1-y)*c;
		o = y + (1-y)*c*(th-tm)/(1-tm);
		n = y - (1-y)*c*tm/(1-tm);
	}

	// Back to RGB order
	if (h < 1){
		r = p;
		g = o;
		b = n;
	} else if (h < 2){
		r = o;
		g = p;
		b = n;
	} else if (h < 3){
		r = n;
		g = p;
		b = o;
	} else if (h < 4){
		r = n;
		g = o;
		b = p;
	} else if (h < 5){
		r = o;
		g = n;
		b = p;
	}else{ 
		r = p;
		g = n;
		b = o;
	}

	*h_ = r;
	*c_ = g;
	*y_ = b;
}

#endif //HELPERS_C
