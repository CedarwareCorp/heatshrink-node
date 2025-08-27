extern "C" {
#include "heatshrink_encoder.h"
}
#include <napi.h>
#include <cstdlib>
#include <vector>

Napi::Value Encode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    if (!info[1].IsNumber() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Window and lookahead sizes must be numbers")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Buffer<uint8_t> in_buffer = info[0].As<Napi::Buffer<uint8_t>>();

    uint32_t window = (uint8_t) info[1].As<Napi::Number>().Uint32Value();
    uint32_t lookahead = (uint8_t) info[2].As<Napi::Number>().Uint32Value();

    if(window < 4 || window > 15){
        Napi::Error::New(env, "Window size must be between 4 and 15").ThrowAsJavaScriptException();
        return env.Null();
    }

    if(lookahead < 3 || lookahead > window - 1){
        Napi::Error::New(env, "Lookahead size must be between 3 and window - 1").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::vector<uint8_t> out_buffer;
    uint8_t output_temp[2048];

    heatshrink_encoder *hse = heatshrink_encoder_alloc(window, lookahead);

    for(int i = 0; i < in_buffer.Length();){
        int bytes = 2048;
        if(i + 2048 > in_buffer.Length()){
            bytes = in_buffer.Length() - i;
        }

        size_t actual_bytes;
        HSE_sink_res result = heatshrink_encoder_sink(hse, &in_buffer.Data()[i], bytes, &actual_bytes);

        if(result != HSER_SINK_OK){
            Napi::Error::New(env, "Sink failed bytes: " + std::to_string(bytes) + 
                " state: " + std::to_string(hse->state) +
                " finishing: " + (hse->flags != 0 ? "true" : "false") +
                " error " + std::to_string(result)
            ).ThrowAsJavaScriptException();
            return env.Null();
        }

        i += actual_bytes;

        HSE_poll_res poll_res;
        HSE_finish_res finish_res;
        do{
            poll_res = heatshrink_encoder_poll(hse, output_temp, 2048, &actual_bytes);

            if(actual_bytes > 0){
                out_buffer.insert(out_buffer.end(), &output_temp[0], &output_temp[actual_bytes]);
            }

            if(i == in_buffer.Length()){
                finish_res = heatshrink_encoder_finish(hse);
            }else{
                finish_res = HSER_FINISH_DONE;
            }

            if(poll_res < 0){
                Napi::Error::New(env, "Poll failed: " + std::to_string(poll_res)).ThrowAsJavaScriptException();
                return env.Null();
            }
            if(finish_res < 0){
                Napi::Error::New(env, "Finish failed: " + std::to_string(finish_res)).ThrowAsJavaScriptException();
                return env.Null();
            }
        }while(poll_res == HSER_POLL_MORE || finish_res == HSER_FINISH_MORE);
    }

    heatshrink_encoder_free(hse);

    return Napi::Buffer<uint8_t>::Copy(env, out_buffer.data(), out_buffer.size());
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "encode"), Napi::Function::New(env, Encode));
  return exports;
}

NODE_API_MODULE(addon, Init)
