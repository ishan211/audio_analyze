/*
Title: C++ Binary Encoded .wav Frequency Analyzer Using FFT with ASCII Output
Name: freq_analyzer.cpp
Author: Ishan Leung
Language: C++23

Usage:
g++ -o freq_analyzer freq_analyzer.cpp -lsndfile -lfftw3
./freq_analyzer

Notes:
Set path of .wav file in line 72: const char* filename = "Audios/file.wav";
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <sndfile.h>
#include <complex>

#define SAMPLE_RATE 44100  // Assuming 44.1 kHz sample rate
#define CHUNK_SIZE SAMPLE_RATE  // 1 second of audio
#define MIN_SAMPLES 44000  // Minimum samples threshold for processing

using Complex = std::complex<double>;
using CArray = std::vector<Complex>;

std::vector<int> msg; // Define msg vector

// Recursive Cooley-Tukey FFT
void fft(CArray& data) {
    int N = data.size();
    if (N <= 1) return;

    // Divide
    CArray even(N / 2), odd(N / 2);
    for (int i = 0; i < N / 2; i++) {
        even[i] = data[i * 2];
        odd[i] = data[i * 2 + 1];
    }

    // Conquer (Recursive call)
    fft(even);
    fft(odd);

    // Combine
    for (int k = 0; k < N / 2; k++) {
        Complex t = std::polar(1.0, -2 * M_PI * k / N) * odd[k];
        data[k] = even[k] + t;
        data[k + N / 2] = even[k] - t;
    }
}

// Function to find the dominant frequency
double getDominantFrequency(const CArray& fftResult, int N, double sampleRate) {
    int peakIndex = 0;
    double maxMagnitude = 0.0;

    for (int i = 1; i < N / 2; ++i) {  // Only look at the positive frequencies
        double magnitude = std::abs(fftResult[i]);
        if (magnitude > maxMagnitude) {
            maxMagnitude = magnitude;
            peakIndex = i;
        }
    }

    return (peakIndex * sampleRate) / N;  // Convert index to frequency
}

int main() {
    const char* filename = "Audios/sine_01000001_8.wav";
    SNDFILE* file;
    SF_INFO sfinfo;

    file = sf_open(filename, SFM_READ, &sfinfo);
    if (!file) {
        std::cerr << "Failed to open file!" << std::endl;
        return 1;
    }

    int numChannels = sfinfo.channels;
    int sampleRate = sfinfo.samplerate;
    std::vector<double> buffer(CHUNK_SIZE * numChannels);
    CArray fftInput(CHUNK_SIZE);

    int readSamples;
    while ((readSamples = sf_readf_double(file, buffer.data(), CHUNK_SIZE)) > 0) {
        if (readSamples < MIN_SAMPLES) {
            continue;  // Skip processing if samples read are less than 44000
        }

        std::cout << "Read Samples: " << readSamples << std::endl;

        // If the last chunk is smaller than CHUNK_SIZE, zero-pad it
        std::fill(buffer.begin() + readSamples, buffer.end(), 0);

        // If stereo, mix down to mono by averaging
        if (numChannels > 1) {
            for (int i = 0; i < readSamples; ++i) {
                buffer[i] = 0;
                for (int ch = 0; ch < numChannels; ++ch) {
                    buffer[i] += buffer[i * numChannels + ch];
                }
                buffer[i] /= numChannels;
            }
        }

        // Convert real input to complex format
        for (int i = 0; i < CHUNK_SIZE; i++) {
            fftInput[i] = Complex(buffer[i], 0.0);
        }

        // Execute FFT
        fft(fftInput);

        // Get dominant frequency
        double dominantFreq = getDominantFrequency(fftInput, CHUNK_SIZE, sampleRate);
        std::cout << "Dominant Frequency: " << dominantFreq << " Hz" << std::endl;

        // Store frequencies in msg if within range
        if (dominantFreq >= 900 && dominantFreq <= 1000) {
            msg.push_back(static_cast<int>(0));  // Ensure msg stored as integer
        } if (dominantFreq >= 1900 && dominantFreq <= 2000) {
            msg.push_back(static_cast<int>(1)); // Ensure msg stored as integer
        }
    }

    sf_close(file);

    // Print full msg vector
    std::cout << "Message: " << std::endl;
    for (int freq : msg) {
        std::cout << freq << "";
    }
    std::cout << std::endl;
    // Convert vector to string
    std::stringstream ss;
    for (size_t i = 0; i < msg.size(); ++i) {
        ss << msg[i];
    }
    std::string str_msg = ss.str();
    
    // Turn string into int
    int int_msg = stoi(str_msg);
    
    // Turn int into ascii char
    char char_msg = static_cast<char>(int_msg);
    
    std::cout << char_msg << std::endl;

    return 0;
}