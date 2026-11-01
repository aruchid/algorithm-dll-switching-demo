#include "algorithm_plugin.h"
#include <cmath>
#include <new>
namespace {
int ALGO_CALL create(void** p) {
    if (!p) return 1;
    *p = nullptr;
#ifdef CREATE_FAILURE
    return 2;
#else
    *p = new (std::nothrow) int(PLUGIN_FACTOR);
    return *p ? 0 : 2;
#endif
}
void ALGO_CALL destroy(void* p) { delete static_cast<int*>(p); }
int ALGO_CALL process(void* p, const float* in, uint32_t n, float* out, uint32_t cap) {
    if (!p || !in || !out || n == 0 || cap < n) return 1;
#ifdef INFER_FAILURE
    return 3;
#else
    const float gain = static_cast<float>(*static_cast<int*>(p));
    for (uint32_t i=0; i<n; ++i)
        if (!std::isfinite(in[i]) || !std::isfinite(in[i]*gain)) return 1;
    for (uint32_t i=0; i<n; ++i) out[i] = in[i]*gain;
    return 0;
#endif
}
}
extern "C" ALGO_EXPORT int ALGO_CALL algorithm_get_api(uint32_t requested, AlgorithmApi* output, uint32_t capacity) {
    if (!output || capacity < sizeof(AlgorithmApi) || requested != ALGO_ABI_V1) return 1;
#ifdef NULL_API
    return 1;
#else
    *output = AlgorithmApi{
#ifdef BAD_VERSION
    99,
#else
    ALGO_ABI_V1,
#endif
    #ifdef SHORT_TABLE
    8,
#else
    sizeof(AlgorithmApi),
#endif
    create, destroy,
#ifdef MISSING_FUNCTION
    nullptr
#else
    process
#endif
    };
    return 0;
#endif
}
