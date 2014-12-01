function playsound(wav, samplerate)
# Written by Timmmm
# Code from http://stackoverflow.com/questions/1478071/how-do-i-play-a-sound-in-octave
#
# Play a single-channel wave at a certain sample rate (defaults to 44100 Hz).
# Input can be integer, in which case it is assumed to be signed 16-bit, or
# float, in which case it is in the range -1:1.

  if (nargin < 1 || nargin > 2)
    print_usage();
  endif

  if (nargin < 2)
    samplerate = 44100;
  end

  if (!isvector(wav))
    error("playsound: X must be a vector");
  endif

  # Write it as a 16-bit signed, little endian (though the amaaazing docs don't say the endianness)
  # If it is integers we assume it is 16 bit signed. Otherwise we assume in the range -1:1
	if (isfloat(wav))
    X = min(max(wav(:), -1), 1) * 32767; # Why matlab & octave do not have a clip() function... I do not know.
  else
	  X = min(max(wav(:), -32767), 32767) + 32767;
  endif

  unwind_protect
	file = tmpnam();
  fid = fopen(file, "wb");
  fwrite(fid, X, "int16");
  fclose(fid);

  # Making aplay (alsa) the default, because let's be honest: it is still way more reliable than
  # the mess that is pulseaudio.
  if (exist("/usr/bin/aplay") == 2)
    system(sprintf("/usr/bin/aplay --format=S16_LE --channels=1 --rate=%d \"%s\"", samplerate, file))
	elseif (exist("/usr/bin/paplay") == 2)
	  system(sprintf("/usr/bin/paplay --format=s16le --channels=1 --rate=%d --raw \"%s\"", samplerate, file))
	endif

  unwind_protect_cleanup
  unlink (file);
  end_unwind_protect

endfunction
