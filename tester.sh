#!/bin/bash

# This script creates few testing executions of the program "befft". Before every call to this program, short message is send to terminal containing information about specific action that this particular test simulates.

progname="befft"

# Clean out the directory
rm -f ${program}.log
make clean
# Build the program
make


# Play input sound and modified output sound from file given as the first parameter, second argument can disallow (by setting it to something non-empty) playing the original sound file
function playSound() {
	if [ "x${2}" == "x" ]; then
		mplayer "${1}" >/dev/null
	fi
	mplayer "${1}.out.wav" 1>/dev/null
}

# This function takes WAV input file as the first argument, knobs settings as a second argument, denominator of Octave fraction in the third argument, then it executes band equalization on it
function bandWav() {
	echo "Processing WAV file ${1}:"
	./"${progname}" -d 40 -f "${1}" -w -o "${1}.out.wav" -k "${2}" -r "${3}" >> "${progname}.log"
}

# This function takes input file with raw data as the first argument, knobs settings as a second argument, denominator of Octave fraction in the third argument, then it executes band equalization on it
function bandRaw() {
	echo "Processing raw file ${1}:"
	./"${progname}" -d 40 -f "${1}" -o "${1}.out.wav" -k "${2}" -r "${3}" >> "${progname}.log"
}


# Process tests

# Generation of sound
echo -e "TEST #1: Generate sound (note A) with Flat function"
infile="./tests/silence.wav"
bandWav "${infile}" "54f+24,54f+24,54f+14" 12
playSound "${infile}" 0

echo -e "\nTEST #2: Generate sound (note A) with Peak function"
bandWav "${infile}" "54p+24,54p+24,54p+14" 12
playSound "${infile}" 0

echo -e "\nTEST #3: Generate sound (note A) with Next function"
bandWav "${infile}" "54n+24,54n+24,54n+14" 12
playSound "${infile}" 0

# Modification of real sound files
echo -e "\nTEST #4: Change the ringing tone a little bit"
infile="./tests/ringing.wav"
bandWav "${infile}" "4-6f-24,7-8p+9,9n-8,10f-10"
playSound "${infile}"

echo -e "\nTEST #5: Reduction of 266Hz -> 530Hz in female voice"
infile="./tests/singing-female.wav"
bandWav "${infile}" "22n-24,23-28f-24,24n+24" 6
playSound "${infile}"

echo -e "\nTEST #6: Reduction of bass, strenghtening of tremble"
infile="./tests/tone.ogg"
bandWav "${infile}" "1-5p-24,1-5f-24,6-10f+12"
playSound "${infile}"

echo -e "\nTEST #7: Strenghtening of bass, reduction of tremble"
infile="./tests/tone.ogg"
bandWav "${infile}" "1-5f+12,6-10p-24"
playSound "${infile}"

echo -e "\nTEST #8: Noise reduction"
infile="./tests/rain.wav"
bandWav "${infile}" "47-67f-14,68-121f-24" 12
playSound "${infile}"
