extern "C" {
#include "heatshrink_encoder.h"
}
#include <napi.h>
#include <cstdlib>
#include <vector>

Napi::Value Encode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Buffer<uint8_t> in_buffer = info[0].As<Napi::Buffer<uint8_t>>();

    std::vector<uint8_t> out_buffer;
    uint8_t output_temp[2048];

    heatshrink_encoder *hse = heatshrink_encoder_alloc(14, 13);

    for(int i = 0; i < in_buffer.Length();){
        int bytes = 2048;
        if(i + 2048 > in_buffer.Length()){
            bytes = in_buffer.Length() - i;
        }

        size_t actual_bytes;
        HSE_sink_res result = heatshrink_encoder_sink(hse, &in_buffer.Data()[i], bytes, &actual_bytes);

        i += actual_bytes;

        if(i == in_buffer.Length()){
            break;
        }

        HSE_poll_res poll_res;
        do{
            HSE_poll_res poll_res = heatshrink_encoder_poll(hse, output_temp, 2048, &actual_bytes);

            if(actual_bytes > 0){
                out_buffer.insert(out_buffer.end(), &output_temp[0], &output_temp[actual_bytes]);
            }

            if(i == in_buffer.Length()){
                heatshrink_encoder_finish(hse);
            }
        }while(poll_res == HSER_POLL_MORE);
    }

    while(heatshrink_encoder_finish(hse) == HSER_FINISH_MORE){
        size_t actual_bytes;
        heatshrink_encoder_poll(hse, output_temp, 2048, &actual_bytes);

        if(actual_bytes > 0){
            out_buffer.insert(out_buffer.end(), &output_temp[0], &output_temp[actual_bytes]);
        }
    }

    heatshrink_encoder_free(hse);

    return Napi::Buffer<uint8_t>::Copy(env, out_buffer.data(), out_buffer.size());
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "encode"), Napi::Function::New(env, Encode));
  return exports;
}

NODE_API_MODULE(addon, Init)
