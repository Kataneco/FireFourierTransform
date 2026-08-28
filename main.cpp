#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "src/desu.h"

#include <portaudio.h>
#include <fftw3.h>

#include <cmath>

//const int SAMPLE_RATE = 44100;
//const int BUFFER_SIZE = SAMPLE_RATE/25;
const int NUM_CHANNELS = 2;

float* fft_input;
fftwf_complex* fft_output;
fftwf_plan fft_plan;

std::vector<glm::vec4> spectrum(BUFFER_SIZE);
std::vector<glm::vec4> length(BUFFER_SIZE);

template<typename T>
T sqr(T v) { return v*v; }

int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    float* samples = (float*)inputBuffer;

    //Right channel
    float right_sum = 0;
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        fft_input[i] = samples[i*2];
        right_sum += std::sqrt(1+ sqr(samples[i*2+2]-samples[i*2]) );
        length[i].x = right_sum;
    }

    fftwf_execute(fft_plan);
    
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        spectrum[i].x = fft_output[i][0];
        spectrum[i].y = fft_output[i][1];

        if(i > BUFFER_SIZE/2) {
            spectrum[i].x = samples[i*2];
        }
    }

    //Left channel
    float left_sum = 0;
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        fft_input[i] = samples[i*2+1];
        left_sum += std::sqrt(1+ sqr(samples[i*2+3]-samples[i*2+1]) );
        length[i].y = left_sum;
    }

    fftwf_execute(fft_plan);
    
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        spectrum[i].z = fft_output[i][0];
        spectrum[i].w = fft_output[i][1];

        if(i > BUFFER_SIZE/2) {
            spectrum[i].z = samples[i*2+1];
        }
    }

    Desu* desu = reinterpret_cast<Desu*>(userData);
    vmaCopyMemoryToAllocation(desu->allocator, spectrum.data(), desu->spectrumStageAllocation, 0, sizeof(glm::vec4)*spectrum.size());
    vmaCopyMemoryToAllocation(desu->allocator, length.data(), desu->spectrumStageAllocation, sizeof(glm::vec4)*spectrum.size(), sizeof(glm::vec4)*length.size());

    return paContinue;
}

int main(int argc, char* argv[]) {
    std::cout << "Yeah, bitch!" << std::endl;

    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) return 1;

    Desu desu;
    desu.init();

    fftwf_init_threads();
    fftwf_plan_with_nthreads(16);
    fft_input = (float*)fftwf_malloc(sizeof(float) * BUFFER_SIZE);
    fft_output = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * BUFFER_SIZE);
    fft_plan = fftwf_plan_dft_r2c_1d(BUFFER_SIZE, fft_input, fft_output,  FFTW_MEASURE);

    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream, NUM_CHANNELS, 0, paFloat32, SAMPLE_RATE, BUFFER_SIZE, audioCallback, &desu);
    if (err != paNoError) return 1;
    err = Pa_StartStream(stream);
    if (err != paNoError) return 1;

    desu.start();

    err = Pa_StopStream(stream);
    if (err != paNoError) return 1;
    err = Pa_CloseStream(stream);
    if (err != paNoError) return 1;
    Pa_Terminate();

    // Clean up FFT resources
    fftwf_destroy_plan(fft_plan);
    fftwf_free(fft_input);
    fftwf_free(fft_output);

    desu.destroy();
}