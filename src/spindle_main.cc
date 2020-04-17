#include <audio.h>
#include <common/threadloop.hh>
#include <common/phonebook.hh>
#include <common/switchboard.hh>

using namespace ILLIXR;

const size_t TIME_SAFETY_FACTOR = 100;

template< class Rep, class Period >
static void t_reliable_sleep(const std::chrono::duration<Rep, Period>& sleep_duration) {
	auto start = std::chrono::high_resolution_clock::now();
	while (std::chrono::high_resolution_clock::now() - start < sleep_duration) {
		std::this_thread::sleep_for(sleep_duration / TIME_SAFETY_FACTOR);
	}
}

class audio_comp : threadloop {
public:
	audio_comp(phonebook* pb)
		: sb{pb->lookup_impl<switchboard>()}
		, pose{sb->subscribe_latest<pose_type>("slow_pose")}
		, now{std::chrono::system_clock::now()}
		, orig_pos{pose->get_latest_ro()->position}
		, orig_rot{pose->get_latest_ro()->orientation}
	{ }

protected:
	virtual void _p_one_iteration() {
		ABAudio::ProcessType procType {ABAudio::ProcessType::FULL};
		const int numBlocks = 1024;
		ABAudio audio {"output.wav", procType};
		audio.loadSource();
		for (int i = 0; i < numBlocks; ++i) {
			auto rot = pose->get_latest_ro()->rotation * orig_rot;
			auto euler_rot = rot.toRotationMatrix().eulerAngles(0, 1, 2);
			audio.updateRotation(Orientation{
				euler_rot[0], euler_rot[1], euler_rot[2]});
			audio.processBlock();
			now += std::chrono::milliseconds {43};
			t_reliable_sleep(now - std::chrono::system_clock::now());
		}
	}

private:
	switchboard * const sb;
	std::unique_ptr<writer<pose_type>> pose;
	std::chrono::time_point<std::chrono::system_clock> now;
	Eigen::Vector3f orig_pos;
	Eigen::Quaternionf orig_rot;
};

PLUGIN_MAIN(audio_comp)
