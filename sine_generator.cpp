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
    char riff_header[4] = {'R', 'I', 'F', 'F'};
    uint32_t wav_size;
    char wave_header[4] = {'W', 'A', 'V', 'E'};
    char fmt_header[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels = 1;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t sample_alignment = 2;
    uint16_t bit_depth = 16;
    char data_header[4] = {'d', 'a', 't', 'a'};
    uint32_t data_bytes;
};

// Function to convert binary string to vector of 8-bit values
std::vector<uint8_t> binary_to_bytes(const std::string& binary) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < binary.length(); i += 8) {
        std::string byte_str = binary.substr(i, 8);
        uint8_t byte_value = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 2));
        bytes.push_back(byte_value);
    }
    return bytes;
}

// Function to generate a sine wave WAV file from a binary message
void sine_gen(const std::string& msg, double bps, double level_dbfs, double sample_rate_khz) {
    double sample_rate = sample_rate_khz * 1000.0;
    double amplitude = pow(10, level_dbfs / 20.0);
    
    // Frequencies for each bit position (0 or 1)
    const double freq_table[8][2] = {
        {300, 500},   // Bit 1 (LSB)
        {700, 900},   // Bit 2
        {1100, 1300}, // Bit 3
        {1500, 1700}, // Bit 4
        {1900, 2100}, // Bit 5
        {2300, 2500}, // Bit 6
        {2700, 2900}, // Bit 7
        {3100, 3300}  // Bit 8 (MSB)
    };
    
    std::vector<uint8_t> bytes = binary_to_bytes(msg);
    int num_bytes = bytes.size();
    double bit_duration = 1.0 / bps;
    double total_duration = bit_duration * num_bytes;
    uint32_t num_samples = static_cast<uint32_t>(total_duration * sample_rate);
    
    WavHeader header;
    header.sample_rate = static_cast<uint32_t>(sample_rate);
    header.byte_rate = header.sample_rate * header.num_channels * (header.bit_depth / 8);
    header.data_bytes = num_samples * (header.bit_depth / 8);
    header.wav_size = 36 + header.data_bytes;
    
    std::vector<int16_t> samples(num_samples);
    
    for (uint32_t i = 0; i < num_samples; i++) {
        double t = static_cast<double>(i) / sample_rate;
        int byte_index = static_cast<int>(t / bit_duration);
        
        if (byte_index >= num_bytes) byte_index = num_bytes - 1;
        uint8_t current_byte = bytes[byte_index];
        
        double sample = 0.0;
        for (int bit = 0; bit < 8; ++bit) {
            bool bit_value = (current_byte >> bit) & 1;
            double freq = freq_table[bit][bit_value];
            sample += sin(2.0 * M_PI * freq * t);
        }
        
        sample = (sample / 8.0) * amplitude * 32767.0;
        samples[i] = static_cast<int16_t>(sample);
    }
    
    std::string filename = "Audios/sine_" + msg + "_" + std::to_string(num_bytes) + ".wav";
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(samples.data()), samples.size() * sizeof(int16_t));
    
    std::cout << "Generated WAV file: " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <binary_message> <bits_per_second> <level_dbfs> <sample_rate_khz>" << std::endl;
        std::cout << "Example: " << argv[0] << " 0100000101000010 1 -3 44.1" << std::endl;
        return 1;
    }
    
    std::string msg = argv[1];
    double bps = std::stod(argv[2]);
    double level_dbfs = std::stod(argv[3]);
    double sample_rate_khz = std::stod(argv[4]);
    
    sine_gen(msg, bps, level_dbfs, sample_rate_khz);
    return 0;
}
