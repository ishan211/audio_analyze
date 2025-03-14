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
#include <complex>
#include <sndfile.h>
#include <bitset>
#include <algorithm>

#define SAMPLE_RATE 44100  // 44.1 kHz sample rate
#define CHUNK_SIZE SAMPLE_RATE  // 1 second of audio
#define MIN_SAMPLES 44000  // Minimum samples threshold for processing

using Complex = std::complex<double>;
using CArray = std::vector<Complex>;

// Frequency mapping for bit positions
const std::vector<std::pair<double, double>> bitFrequencyPairs = {
    {300, 500},   // Bit 1 (LSB)
    {700, 900},   // Bit 2
    {1100, 1300}, // Bit 3
    {1500, 1700}, // Bit 4
    {1900, 2100}, // Bit 5
    {2300, 2500}, // Bit 6
    {2700, 2900}, // Bit 7
    {3100, 3300}  // Bit 8 (MSB)
};

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

// Function to find the 8 most dominant frequencies
std::vector<double> getTop8Frequencies(const CArray& fftResult, int N, double sampleRate) {
    std::vector<std::pair<double, int>> magnitudes;

    for (int i = 1; i < N / 2; ++i) {  // Ignore DC component (i=0)
        double magnitude = std::abs(fftResult[i]);
        magnitudes.emplace_back(magnitude, i);
    }

    std::sort(magnitudes.begin(), magnitudes.end(), std::greater<>());  // Sort by magnitude

    std::vector<double> topFrequencies;
    for (int i = 0; i < 8 && i < magnitudes.size(); ++i) {
        int peakIndex = magnitudes[i].second;
        double frequency = (peakIndex * sampleRate) / N;
        topFrequencies.push_back(frequency);
    }

    return topFrequencies;
}

// Function to map detected frequencies to a binary byte with improved output format
int frequenciesToByte(const std::vector<double>& detectedFrequencies) {
    int byteValue = 0;
    std::vector<int> bits(8, 0);
    std::vector<double> closestFreqs(8, 0.0);
    
    // First pass: analyze each bit separately with improved output
    for (int i = 0; i < 8; ++i) {
        bool bit0Detected = false;
        bool bit1Detected = false;
        double closestFreq = 0.0;
        double minDiff = 1000.0;  // Initialize with a large value
        
        // Find the closest frequency match for this bit position
        for (double freq : detectedFrequencies) {
            double diff0 = std::abs(freq - bitFrequencyPairs[i].first);
            double diff1 = std::abs(freq - bitFrequencyPairs[i].second);
            
            if (diff0 < 50 && diff0 < minDiff) {
                bit0Detected = true;
                bit1Detected = false;
                minDiff = diff0;
                closestFreq = freq;
            }
            
            if (diff1 < 50 && diff1 < minDiff) {
                bit1Detected = true;
                bit0Detected = false;
                minDiff = diff1;
                closestFreq = freq;
            }
        }
        
        // Store the bit value and closest frequency
        bits[i] = bit1Detected ? 1 : 0;
        closestFreqs[i] = closestFreq;
    }
    
    // Display individual bit analysis
    for (int i = 0; i < 8; ++i) {
        std::cout << "Bit " << (i + 1) << ": ";
        if (closestFreqs[i] > 0) {
            std::cout << closestFreqs[i] << " Hz, ";
        } else {
            std::cout << "No frequency detected, ";
        }
        std::cout << bits[i] << std::endl;
    }
    
    // Construct the byte value (MSB first for correct ASCII)
    for (int i = 0; i < 8; ++i) {
        if (bits[i]) {
            // The LSB is bit 0, MSB is bit 7
            byteValue |= (1 << (7 - i));
        }
    }
    
    std::string bitString;
    for (int i = 0; i < 8; ++i) {
        bitString += (bits[i] ? '1' : '0');
    }
    
    std::cout << "Decoded Byte: " << bitString;
    
    // Only print ASCII if it's a printable character
    if (isprint(byteValue)) {
        std::cout << " (" << char(byteValue) << ")";
    }
    std::cout << std::endl;
    
    return byteValue;
}

int main() {
    const char* filename = "test_ABC123.wav";
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
    std::vector<char> asciiMessage;

    while ((readSamples = sf_readf_double(file, buffer.data(), CHUNK_SIZE)) > 0) {
        if (readSamples < MIN_SAMPLES) continue;  // Skip small chunks

        std::cout << "\nSamples Read: " << readSamples << std::endl;

        std::fill(buffer.begin() + readSamples, buffer.end(), 0);  // Zero-pad small chunks

        // Convert stereo to mono if needed
        if (numChannels > 1) {
            for (int i = 0; i < readSamples; ++i) {
                buffer[i] = 0;
                for (int ch = 0; ch < numChannels; ++ch) {
                    buffer[i] += buffer[i * numChannels + ch];
                }
                buffer[i] /= numChannels;
            }
        }

        // Convert real input to complex format for FFT
        for (int i = 0; i < CHUNK_SIZE; i++) {
            fftInput[i] = Complex(buffer[i], 0.0);
        }

        fft(fftInput);  // Perform FFT

        std::vector<double> detectedFrequencies = getTop8Frequencies(fftInput, CHUNK_SIZE, sampleRate);

        // Print detected frequencies for debugging
        std::cout << "Detected Frequencies: ";
        for (double freq : detectedFrequencies) {
            std::cout << freq << " Hz, ";
        }
        std::cout << std::endl;

        int byteValue = frequenciesToByte(detectedFrequencies);
        asciiMessage.push_back(static_cast<char>(byteValue));
    }

    sf_close(file);

    std::cout << "\nDecoded Message: ";
    for (char c : asciiMessage) {
        if (isprint(c)) {
            std::cout << c;
        } else {
            std::cout << "?"; // Replace non-printable characters
        }
    }
    std::cout << std::endl;

    return 0;
}