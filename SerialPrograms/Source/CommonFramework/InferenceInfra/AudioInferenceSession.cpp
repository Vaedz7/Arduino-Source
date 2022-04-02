/*  Async Audio Inference
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QImage>
#include "Common/Cpp/Exceptions.h"
#include "CommonFramework/Inference/StatAccumulator.h"
#include "AudioInferenceSession.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{



struct AudioInferenceSession::Callback{
    AudioInferenceCallback* callback;
    StatAccumulatorI32 stats;

    Callback(AudioInferenceCallback* p_callback)
        : callback(p_callback)
    {}
};



AudioInferenceSession::AudioInferenceSession(
    Logger& logger, CancellableScope& scope,
    AudioFeed& feed,
    std::chrono::milliseconds period
)
    : Cancellable(scope)
    , m_logger(logger)
    , m_feed(feed)
    , m_period(period)
{}
AudioInferenceSession::~AudioInferenceSession(){
    detach();
    AudioInferenceSession::cancel();
}
bool AudioInferenceSession::cancel() noexcept{
    if (Cancellable::cancel()){
        return true;
    }
    {
        std::unique_lock<std::mutex> lg(m_lock);
        m_cv.notify_all();
    }

    const double DIVIDER = std::chrono::milliseconds(1) / std::chrono::microseconds(1);
    const char* UNITS = " ms";
    try{
        for (Callback* callback : m_callback_list){
            callback->stats.log(m_logger, callback->callback->label(), UNITS, DIVIDER);
        }
    }catch (...){}
    return false;
}

void AudioInferenceSession::operator+=(AudioInferenceCallback& callback){
    std::unique_lock<std::mutex> lg(m_lock);

    auto iter = m_callback_map.find(&callback);
    if (iter != m_callback_map.end()){
        return;
    }

    Callback& entry = m_callback_map.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(&callback),
        std::forward_as_tuple(&callback)
    ).first->second;

    try{
        m_callback_list.emplace_back(&entry);
    }catch (...){
        m_callback_map.erase(&callback);
        throw;
    }
}
void AudioInferenceSession::operator-=(AudioInferenceCallback& callback){
    std::unique_lock<std::mutex> lg(m_lock);
    auto iter0 = m_callback_map.find(&callback);
    if (iter0 == m_callback_map.end()){
        return;
    }
    auto iter1 = std::find(m_callback_list.begin(), m_callback_list.end(), &iter0->second);
    m_callback_list.erase(iter1);
    m_callback_map.erase(iter0);
}

AudioInferenceCallback* AudioInferenceSession::run(std::chrono::milliseconds timeout){
    auto now = std::chrono::system_clock::now();
    auto wait_until = now + m_period;
    auto stop_time = timeout == std::chrono::milliseconds(0)
        ? std::chrono::system_clock::time_point::max()
        : wait_until + timeout;
    return run(stop_time);
}
AudioInferenceCallback* AudioInferenceSession::run(std::chrono::system_clock::time_point stop){
    using WallClock = std::chrono::system_clock::time_point;

    auto now = std::chrono::system_clock::now();
    auto next_tick = now + m_period;

    uint64_t lastTimestamp = ~(uint64_t)0;
    // Stores new spectrums from audio feed. The newest spectrum (with largest timestamp) is at
    // the front of the vector.
    std::vector<AudioSpectrum> spectrums;

    while (true){
        throw_if_parent_cancelled();
        if (cancelled()){
            return nullptr;
        }

        if (lastTimestamp == SIZE_MAX){
            spectrums = m_feed.spectrums_latest(1);
        } else{
            // Note: in this file we never consider the case that stamp may overflow.
            // It requires on the order of 1e10 years to overflow if we have about 25ms per stamp.
            spectrums = m_feed.spectrums_since(lastTimestamp + 1);
        }
        if (spectrums.size() > 0){
            // spectrums[0] has the newest spectrum with the largest stamp:
            lastTimestamp = spectrums[0].stamp;
        }

        std::unique_lock<std::mutex> lg(m_lock);
        for (Callback* callback : m_callback_list){
//            std::cout << "Run callback on spectrums " << spectrums.size() << std::endl;
            WallClock time0 = std::chrono::system_clock::now();
            bool done = callback->callback->process_spectrums(spectrums, m_feed);
            WallClock time1 = std::chrono::system_clock::now();
            callback->stats += std::chrono::duration_cast<std::chrono::microseconds>(time1 - time0).count();
            if (done){
                return callback->callback;
            }
        }

        now = std::chrono::system_clock::now();
        if (now >= stop){
            return nullptr;
        }
        auto wait = next_tick - now;
        if (wait <= std::chrono::milliseconds(0)){
            next_tick = now + m_period;
        }else{
            WallClock stop_wait = std::min(next_tick, stop);
            m_cv.wait_until(
                lg, stop_wait,
                [=]{ return cancelled(); }
            );
            next_tick += m_period;
        }
    }
}



AsyncAudioInferenceSession::AsyncAudioInferenceSession(
    ProgramEnvironment& env, Logger& logger, CancellableScope& scope,
    AudioFeed& feed,
    std::chrono::milliseconds period
)
    : AudioInferenceSession(logger, scope, feed, period)
    , m_triggering_callback(nullptr)
    , m_task(env.inference_dispatcher().dispatch([this]{ thread_body(); }))
{}
AsyncAudioInferenceSession::AsyncAudioInferenceSession(
    ProgramEnvironment& env, Logger& logger, CancellableScope& scope,
    AudioFeed& feed,
    std::function<void()> on_finish_callback,
    std::chrono::milliseconds period
)
    : AudioInferenceSession(logger, scope, feed, period)
    , m_on_finish_callback(std::move(on_finish_callback))
    , m_triggering_callback(nullptr)
    , m_task(env.inference_dispatcher().dispatch([this]{ thread_body(); }))
{}
AsyncAudioInferenceSession::~AsyncAudioInferenceSession(){
    AudioInferenceSession::cancel();
}
void AsyncAudioInferenceSession::rethrow_exceptions(){
    if (m_task){
        m_task->rethrow_exceptions();
    }
}
AudioInferenceCallback* AsyncAudioInferenceSession::stop_and_rethrow(){
    AudioInferenceSession::cancel();
    if (m_task){
        m_task->wait_and_rethrow_exceptions();
    }
    return m_triggering_callback;
}
void AsyncAudioInferenceSession::thread_body(){
    try{
        m_triggering_callback = run();
        if (m_on_finish_callback){
            m_on_finish_callback();
        }
    }catch (...){
        if (m_on_finish_callback){
            m_on_finish_callback();
        }
        throw;
    }
}






}
