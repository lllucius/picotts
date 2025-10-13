#!/bin/bash
#
# demo_test2wave.sh - Demonstration script for test2wave example
#
# This script shows how to use the test2wave example program
# to generate speech with voice quality improvements.
#

set -e

# Check if test2wave exists
if [ ! -f "test2wave" ] && [ ! -f ".libs/test2wave" ]; then
    echo "Error: test2wave not found. Please build it first:"
    echo "  cd pico"
    echo "  ./autogen.sh"
    echo "  ./configure"
    echo "  make"
    exit 1
fi

# Determine the correct binary path
if [ -f ".libs/test2wave" ]; then
    export LD_LIBRARY_PATH=.libs
    TEST2WAVE=".libs/test2wave"
elif [ -f "test2wave" ]; then
    TEST2WAVE="./test2wave"
else
    echo "Error: Could not find test2wave binary"
    exit 1
fi

echo "============================================"
echo "PicoTTS test2wave Demonstration"
echo "============================================"
echo ""

# Example 1: Simple greeting
echo "1. Simple greeting..."
$TEST2WAVE demo1_greeting.wav "Hello! Welcome to PicoTTS."
echo "   Created: demo1_greeting.wav"
echo ""

# Example 2: Longer sentence
echo "2. Longer sentence..."
$TEST2WAVE demo2_sentence.wav "The quick brown fox jumps over the lazy dog. This is a classic pangram used for testing."
echo "   Created: demo2_sentence.wav"
echo ""

# Example 3: Technical description
echo "3. Technical description..."
$TEST2WAVE demo3_technical.wav "This speech is synthesized using the Pico text to speech engine with voice quality enhancements. A low shelf filter improves clarity and tonal balance."
echo "   Created: demo3_technical.wav"
echo ""

# Example 4: Numbers and punctuation
echo "4. Numbers and punctuation..."
$TEST2WAVE demo4_numbers.wav "Testing numbers: one, two, three, 4, 5, 6. Punctuation works too! Does it? Yes, it does."
echo "   Created: demo4_numbers.wav"
echo ""

echo "============================================"
echo "Demo complete!"
echo "============================================"
echo ""
echo "Generated WAV files:"
ls -lh demo*.wav
echo ""
echo "You can play these files using:"
echo "  aplay demo1_greeting.wav"
echo "  play demo2_sentence.wav"
echo "or any audio player that supports WAV format."
echo ""
echo "All files are 16kHz, 16-bit mono PCM with voice quality"
echo "enhancements applied."
