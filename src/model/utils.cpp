#include <utils.h>

float clamp(const float& min, const float& max, const float& n)
{
	return std::max(min, std::min(max, n));
}

float getRandomNumber(const float min, const float max)
{
	std::random_device device;
	std::mt19937 generate(device());
	std::uniform_real_distribution<float> distribution(min, max);

	return distribution(generate);
}

void outputProgress(float progress)
{
	std::cout << int(progress * 100.0) << " %\r";
	std::cout.flush();
}

void outputTimeUse(std::string name, std::chrono::system_clock::duration duration)
{
	std::cout << name << " Time Use: ";
	std::cout << std::chrono::duration_cast<std::chrono::hours>(duration).count() << " hours ";
	std::cout << std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60 << " minutes ";
	std::cout << std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60 << " seconds ";
	std::cout << std::endl;
}

void outputFrameRate(const int fps, const int fr)
{
	std::cout << "========== FPS: " << fps << " , Frame Time: " << fr << " ms ==========\r";
	std::cout.flush();
}
