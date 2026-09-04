#ifndef PHOSPHOR_CONSTANTS_H
#define PHOSPHOR_CONSTANTS_H

#define GPU_ALIGN __attribute__((aligned(16)))

#define EPS 1e-4f
#define INF 1e9f
#define PI 3.141592654f

#define NO_TEXTURE 0xFFFFFFFFu
#define LUMINOUS_EFFICACY 683.0f // cd * sr / W
#define DEFAULT_TRANSMISSION 0.0f
#define DEFAULT_IOR 1.5f
#define AIR_IOR 1.0f
#define MAX_PHOTON_BOUNCES 50
#define MAX_RAY_BOUNCES 5
#define MIN_CELL_SIZE 0.1f

#define BVH_STACK_SIZE 24
#define KD_STACK_SIZE 24
#define MAX_PHOTON_SAMPLES 256

#define MAX_PHOTONS_PER_BATCH (1u << 15)
#define DEFAULT_CAMERA_ASPECT (16.0f / 9.0f)

#define BLACK (float4)(0.0f, 0.0f, 0.0f, 0.0f)
#define WHITE (float4)(1.0f, 1.0f, 1.0f, 0.0f)

#define MIN_ROUGHNESS 0.001f
#define GGX_SAMPLING_ATTEMPTS 32u
#define MIN_RAY_RR_DEPTH 2
#define MIN_PHOTON_RR_DEPTH 2

#endif // PHOSPHOR_CONSTANTS_H
