/*
Title: C++ Binary Encoded Sine Wave .wav Generator with Customizable bits/s, Audio Levels, and Sample Rate
Name: sine_generator.cpp
Author: Ishan Leung
Language: C++23

Usage:
g++ -o sine_generator sine_generator.cpp
./sine_generator <binary_message> <bits_per_second> <level_dbfs> <sample_rate_khz>

Example Usage:
g++ -o sine_generator sine_generator.cpp
./sine_generator 01000001 1 -3 44.1
*/

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <cstdint>

// WAV file header structure
struct WavHeader {
    // RIFF chunk
    char riff_header[4] = {'R', 'I', 'F', 'F'};
    uint32_t wav_size;
    char wave_header[4] = {'W', 'A', 'V', 'E'};
    
    // fmt chunk
    char fmt_header[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1; // PCM
    uint16_t num_channels = 1; // Mono
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t sample_alignment = 2;
    uint16_t bit_depth = 16;
    
    // data chunk
    char data_header[4] = {'d', 'a', 't', 'a'};
    uint32_t data_bytes;
};

// Function to convert binary string to vector of bits
std::vector<bool> binary_to_bits(const std::string& binary) {
    std::vector<bool> bits;
    for (char c : binary) {
        if (c == '0') {
            bits.push_back(false);
        } else if (c == '1') {
            bits.push_back(true);
        } else {
            std::cerr << "Warning: Ignoring non-binary character '" << c << "'" << std::endl;
        }
    }
    return bits;
}

// Function to generate a sine wave WAV file from a binary message
void sine_gen(const std::string& msg, double bps, double level_dbfs, double sample_rate_khz) {
    // Convert parameters to working values
    double sample_rate = sample_rate_khz * 1000.0;
    double amplitude = pow(10, level_dbfs / 20.0); // Convert dBFS to amplitude
    
    // Frequencies for 0 and 1 bits
    const double freq_0 = 1000.0; // Hz for bit 0
    const double freq_1 = 2000.0; // Hz for bit 1
    
    // Convert message to bits
    std::vector<bool> bits = binary_to_bits(msg);
    int num_bits = bits.size();
    
    // Calculate duration and number of samples
    double bit_duration = 1.0 / bps; // Duration of one bit in seconds
    double total_duration = bit_duration * num_bits; // Total duration in seconds
    uint32_t num_samples = static_cast<uint32_t>(total_duration * sample_rate);
    
    // Prepare WAV header
    WavHeader header;
    header.sample_rate = static_cast<uint32_t>(sample_rate);
    header.byte_rate = header.sample_rate * header.num_channels * (header.bit_depth / 8);
    header.data_bytes = num_samples * (header.bit_depth / 8);
    header.wav_size = 36 + header.data_bytes;
    
    // Generate samples
    std::vector<int16_t> samples(num_samples);
    
    for (uint32_t i = 0; i < num_samples; i++) {
        double t = static_cast<double>(i) / sample_rate; // Time in seconds
        int bit_index = static_cast<int>(t / bit_duration);
        
        // Ensure bit_index is within range
        if (bit_index >= num_bits) bit_index = num_bits - 1;
        
        // Select frequency based on current bit
        double frequency = bits[bit_index] ? freq_1 : freq_0;
        
        // Generate sine wave sample
        double sample = amplitude * sin(2.0 * M_PI * frequency * t);
        
        // Convert to 16-bit PCM
        samples[i] = static_cast<int16_t>(sample * 32767.0);
    }
    
    // Create output filename
    std::string filename = "Audios/sine_" + msg + "_" + std::to_string(num_bits) + ".wav";
    
    // Write to file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write samples
    file.write(reinterpret_cast<const char*>(samples.data()), samples.size() * sizeof(int16_t));
    
    std::cout << "Generated WAV file: " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <binary_message> <bits_per_second> <level_dbfs> <sample_rate_khz>" << std::endl;
        std::cout << "Example: " << argv[0] << " 01000001 1 -3 44.1" << std::endl;
        return 1;
    }
    
    std::string msg = argv[1];
    double bps = std::stod(argv[2]);
    double level_dbfs = std::stod(argv[3]);
    double sample_rate_khz = std::stod(argv[4]);
    
    sine_gen(msg, bps, level_dbfs, sample_rate_khz);
    
    return 0;
}