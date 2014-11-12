Band equalizer using FFT
========================

Introduction
------------
This project is meant to be an implementation of simple band equalizer, with specifiable number of bands controlable by user, with usage of Fast Fourier Transform in C language. Input can be either raw data or sound file in WAV format, the same applies for output.

Usage
-----
```
Usage: program -f in_file [-w] [-r denom] [-k list] [-d level]
   -f in_file: set the name of an input file to "in_file"

   -w:         input file is in WAV format

   -r denom:   set Octave bands control to [1/denom] (must be in range [1; 24] by standard ISO)
        (default value is 1)

   -k list:    list defines configuration of virtual knobs separated by commas, every knob has 3 properties:
        band ID:  integer representing specific band, interval depends on choosen Octave fraction (-r option)
        function: must be one of "f" for flat, "p" for peak, or "n" for next
        gain:     integer value from range [-24; 24] (in dB) with, or without its sign
        EXAMPLE:  -k 1f+20,7n-24,42p+21 (use flat function applied to the first band with gain 20dB, etc.)

   -d level:   changes debug level to "level", smaller value means more info
        (default value is 90, used range is [1; 100])
```

Windowing
---------
In order to process bigger input files successfully, we use so called window function. In equalizer.c, you can find three examples of them, implemented are called Planck, Tukey, and Hamming. In this program, only the Planck window function is used as it provided the best results on sample tests.
For further details about this functionality, see Window functions bellow in the Links section.

Requirements
------------
 - **gnuplot** - used to simplify graphical output
 - **Octave/Matlab** - if you want to use generated data

Links
-----
 - Equalization quide
   - http://www.zytrax.com/tech/audio/equalization.html
 - Wave format specification
   - https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 - Window functions
   - http://en.wikipedia.org/wiki/Window_function#A_list_of_window_functions
