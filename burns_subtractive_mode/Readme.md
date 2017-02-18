# Background information:
http://scottburns.us/subtractive-color-mixture/

http://handprint.com/HP/WCL/color14.html


Demo: https://youtu.be/Qa6V6k6kMIw and https://www.youtube.com/watch?v=RUa8IHNtGb4

Sample: https://flic.kr/p/C5BC5w

# Benefits:
*  Blends colors with 36 spectrums instead of 3 (R,G,B)
*  Realistic curved blending mix lines instead of straight lines
*  More natural "saturation costs" which lend to better color harmony
*  Better control due to linear blending ratios (linear light)
*  Blue+Yellow=Green instead of Grey/White ;-)
*  Blendable modes from RGB to Spectral, as well as Normal versus Subtractive models.  (Spectral does not imply Subtractive, you can mix them around)

# Requirements:  
~12GB of extra disk space, and ~2.4GB of additional memory.
If your system has 4GB of RAM or more it should be usable.  There is some CPU overhead as well but not extreme.

# Instructions:
1.  You need a data file for the RGB to pigment conversions.  You can either download a pre-generated file, 
or generate your own.
Download and unzip this file:
https://drive.google.com/open?id=0B2Rzl_WOUDRiakVXOGhXbFZ3cVk
mirror: https://www.dropbox.com/s/sv87lwrthri43d7/rgb.zip?dl=0
Or, install gnu octave (or matlab) and run the generate.sh script.  This could take several days but you will 
have the rgb.txt file when it is done.

2.  Place this file in the current working directory of where you launch mypaint.  This could be your home folder, or if you 
    launch it from the command line it will be whatever folder you are in when you run mypaint.
3.  If the rgb.txt file is not found, mypaint will print a message to the console and will fall-back to using normal RGB mixing.
4.  Modify a brush and change adjust the "Smudge Subtractive" setting.  0.0 is normal mode, and 1.0 is subtractive.  Also set the Smudge Spectral setting to 1.0 (0 is RGB)
    Make sure the brush uses smudge as well (I usually map smudge to pressure so that light pressure smudges and heavy pressure 
    does not)
5.  The 1st time you use the brush, mypaint will load the rgb.txt file into memory.  This can make mypaint hang for a few minutes.
    Be patient and dismiss any dialogs that might pop up about the application "not responding".
6.  Once that file is loaded, it is smooth sailing until you quit mypaint (never quit drawing! ;-).

# Caveats/Suggestions

* Be aware of RGB mixing paths.  Opacity in every other area of MyPaint will use the standard RGB model, so if you use 
  any of the following opacity methods in conjunction with Spectral Smudge you might get unexpected results.  It's ok to mix 
  the modes and it might be more realistic to do so, but just be aware of these:
  * Hardness < 1.0
  * Pixel Feathering > 0.0 (antialiasing)
  * Brush Opacity < 1.0
  * Brush Opacity Multiply < 1.0
  * radius by random > 0.0
  * layers in general
* sRGB and D65 light source.  The code provided by Scott Burns is set for an sRGB standard, however he describes how you can regenerate the file for another color space such as Adobe RGB, although I think that would also require a different gamma function.  sRGB seems to be plenty of gamut for simulating actual pigments, but it might be interesting to simulate Adobe RGB pigments.  I haven't had an issues using the sRGB data tables with an Adobe RGB monitor, but I don't know if I'd notice subtle problems.
* The lookup table I've generated is 8bpp, whereas MyPaint is actually 15bpp.  This means every lookup is being rounded to the nearest color, without accounting for perceptual issues.  This hasn't really been a noticable issue AFAICT.

# Future

* It'd be great to compute all of this in real-time using whatever colorspace your application is running in.  This would eliminate all the RAM and Disk requirements and allow greater bitdepth, Adobe RGB, etc.  I suspect the only way to do this would to offload everything to a GPU and/or multithread the function.
