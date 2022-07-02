#include <future>
#include <thread>

#include "common/data_format.hpp"
#include "common/phonebook.hpp"
#include "common/relative_clock.hpp"
#include "common/switchboard.hpp"
#include "common/threadloop.hpp"

#include <audio.h>

using namespace ILLIXR;

class audio_xcoding : public threadloop
{
public:
    audio_xcoding(phonebook *pb_, bool encoding)
        : threadloop{encoding ? "audio_encoding" : "audio_decoding", pb_}
        , _m_sb{pb->lookup_impl<switchboard>()}
        , _m_clock{pb->lookup_impl<RelativeClock>()}
        , _m_pose{_m_sb->get_reader<pose_type>("slow_pose")}
        , xcoder{"", encoding ? ILLIXR_AUDIO::ABAudio::ProcessType::ENCODE : ILLIXR_AUDIO::ABAudio::ProcessType::DECODE}
        , last_time{_m_clock->now()}
        , encoding_{encoding}
    {
        xcoder.loadSource();
    }

    virtual skip_option _p_should_skip() override {
        last_time += audio_period;
        std::this_thread::sleep_for(last_time - _m_clock->now());
        return skip_option::run;
    }

    virtual void _p_one_iteration() override {
        if (!encoding_) {
            [[maybe_unused]] auto most_recent_pose = _m_pose.get_ro_nullable();
        }
        xcoder.processBlock();
    }

private:
    const std::shared_ptr<switchboard> _m_sb;
    const std::shared_ptr<RelativeClock> _m_clock;
    switchboard::reader<pose_type> _m_pose;
    ILLIXR_AUDIO::ABAudio xcoder;
    time_point last_time;
    static constexpr duration audio_period{freq2period(static_cast<double>(SAMPLERATE) / static_cast<double>(BLOCK_SIZE))};
    bool encoding_;
};

class audio_pipeline : public plugin {
public:
    audio_pipeline(std::string name_, phonebook *pb_)
        : plugin{name_, pb_}
        , audio_encoding{pb_, true }
        , audio_decoding{pb_, false}
    { }

    virtual void start() override {
        audio_encoding.start();
        audio_decoding.start();
        plugin::start();
    }

    virtual void stop() override {
        audio_encoding.stop();
        audio_decoding.stop();
        plugin::stop();
    }

private:
    audio_xcoding audio_encoding;
    audio_xcoding audio_decoding;
};

PLUGIN_MAIN(audio_pipeline)
