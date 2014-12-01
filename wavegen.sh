#!/bin/bash

# Script for generating specific note with specific length of specific number of samples with usage of GNU Octave

fout="sound_in.mat"
sampout="sample.in"

octave <<EOF
graphics_toolkit gnuplot;
figure("visible", "off");
# duration (sec)
dur = 5;
# samples
sampl = 10000
# frequency (Hz)
f = 440;

# generate X values
t = linspace(0, dur, dur*sampl);
# generate Y values
y = sin(f * 2*pi*t);

# use data nad play them as sound
playsound(y, sampl);

plot(y);
saveas(1, "octave.png");
save ${fout} y
EOF

rm -f "$sampout"
cp "$fout" "$sampout"
