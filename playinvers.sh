#!/bin/bash

fin="invers_1.out"
fin_name=$(basename -s .out "$fin")
sampl="$1"

octave <<EOF
load ${fin}
playsound(${fin_name}, ${sampl})
EOF
