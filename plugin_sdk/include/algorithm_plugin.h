#ifndef ALGORITHM_PLUGIN_H
#define ALGORITHM_PLUGIN_H
#include <stdint.h>
#ifdef _WIN32
#ifdef ALGO_BUILD_PLUGIN
#define ALGO_EXPORT __declspec(dllexport)
#else
#define ALGO_EXPORT
#endif
#define ALGO_CALL __cdecl
#else
#define ALGO_EXPORT __attribute__((visibility("default")))
#define ALGO_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define ALGO_ABI_V1 1u
/* Same architecture/calling convention required. No STL, ownership transfer,
 * or exceptions may cross this boundary. Input/output owned by host.
 * Status: 0 success; nonzero failure. All calls are synchronous. */
typedef struct AlgorithmApi {
    uint32_t abi_version;
    uint32_t struct_size;
    int (ALGO_CALL *create)(void** instance);
    void (ALGO_CALL *destroy)(void* instance);
    int (ALGO_CALL *process)(void* instance, const float* input,
                             uint32_t count, float* output, uint32_t capacity);
} AlgorithmApi;
/* Host owns the function table as well. Query returns nonzero on rejection. */
typedef int (ALGO_CALL *AlgorithmGetApi)(uint32_t requested_abi, AlgorithmApi* output, uint32_t capacity);
ALGO_EXPORT int ALGO_CALL algorithm_get_api(uint32_t requested_abi, AlgorithmApi* output, uint32_t capacity);
#ifdef __cplusplus
}
#endif
#endif
