#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <atomic>

#include "engine.hpp"


namespace rack {

bool gPaused = false;
std::vector<Module*> gModules;
std::vector<Wire*> gWires;
bool gPowerMeter = false;

static bool running = false;
static float sampleRate = 44100.f;
static float sampleTime = 1.f / sampleRate;
static float sampleRateRequested = sampleRate;

static Module *resetModule = NULL;
static Module *randomizeModule = NULL;

static std::mutex mutex;
static std::thread thread;
static VIPMutex vipMutex;

// Parameter interpolation
static Module *smoothModule = NULL;
static int smoothParamId;
static float smoothValue;


float Light::getBrightness() {
	// LEDs are diodes, so don't allow reverse current.
	// For some reason, instead of the RMS, the sqrt of RMS looks better
	return powf(fmaxf(0.f, value), 0.25f);
}

void Light::setBrightnessSmooth(float brightness, float frames) {
	float v = (brightness > 0.f) ? brightness * brightness : 0.f;
	if (v < value) {
		// Fade out light with lambda = framerate
		value += (v - value) * sampleTime * frames * 60.f;
	}
	else {
		// Immediately illuminate light
		value = v;
	}
}


void Wire::step() {
	float value = outputModule->outputs[outputId].value;
	inputModule->inputs[inputId].value = value;
}

static void moduleStep(Module*);
static std::atomic_int moduleIndex;
static std::atomic_int completedProcessors;


class AudioProcessor {
public:
	AudioProcessor():
	blocksize(1),
	initialblock(1),
	sleeping(false),
	stepping(false),
	running(true),
	thread([this]{ audioThreadFunction(); })
	{ }

	void Stop() {
		running = false;
		thread.join();
	}

	void Step(int blocksize, int initialblock) {
		this->blocksize = blocksize;
		this->initialblock = initialblock;
		stepping = true;
	}

	bool IsStepping() {
		return stepping;
	}

	void Sleep() {
		sleepMutex.lock();
		sleeping = true;
	}

	void Wake() {
		if (sleeping) {
			sleepMutex.unlock();
			sleeping = false;
		}
	}

	void audioThreadFunction() {

		while (running) {
			// run this thread as a busy loop

			while (running && !stepping) { // Busy wait for stepping to set
				if (sleepMutex.try_lock_for(std::chrono::milliseconds(200))) {
					sleepMutex.unlock();
				}
			} 

			if (!running) return;


			int modulecount = gModules.size();

			int blockend = initialblock;
			int first = blockend - blocksize;
			while (first < modulecount) {
				for (int next = first; next < blockend && next < modulecount; next += 1) {
					moduleStep(gModules[modulecount-next-1]);
				}

				blockend = moduleIndex += blocksize;
				first = blockend - blocksize;
			}

			stepping = false;
			completedProcessors++;
		}

	}

private:
	volatile bool stepping;
	volatile bool running;
	std::thread thread;
	std::timed_mutex sleepMutex;
	bool sleeping;
	int blocksize;
	int initialblock;
};

static std::vector<AudioProcessor*> audioProcessors;


void engineInit() {
}

void engineDestroy() {

	engineSetAudioThreads(0);

	// Make sure there are no wires or modules in the rack on destruction. This suggests that a module failed to remove itself before the RackWidget was destroyed.
	assert(gWires.empty());
	assert(gModules.empty());
}

static void moduleStep(Module* module) {
	std::chrono::high_resolution_clock::time_point startTime;
	if (gPowerMeter) {
		startTime = std::chrono::high_resolution_clock::now();

		module->step();

		auto stopTime = std::chrono::high_resolution_clock::now();
		float cpuTime = std::chrono::duration<float>(stopTime - startTime).count() * sampleRate;
		module->cpuTime += (cpuTime - module->cpuTime) * sampleTime / 0.5f;
	}
	else {
		module->step();
	}

	// Step ports
	for (Input &input : module->inputs) {
		if (input.active) {
			float value = input.value / 5.f;
			input.plugLights[0].setBrightnessSmooth(value);
			input.plugLights[1].setBrightnessSmooth(-value);
		}
	}
	for (Output &output : module->outputs) {
		if (output.active) {
			float value = output.value / 5.f;
			output.plugLights[0].setBrightnessSmooth(value);
			output.plugLights[1].setBrightnessSmooth(-value);
		}
	}
}

std::mutex mainEngineSleepMutex;

void engineStep() {
	// Sample rate
	if (sampleRateRequested != sampleRate) {
		sampleRate = sampleRateRequested;
		sampleTime = 1.f / sampleRate;
		for (Module *module : gModules) {
			module->onSampleRateChange();
		}
	}

	// Events
	if (resetModule) {
		resetModule->onReset();
		resetModule = NULL;
	}
	if (randomizeModule) {
		randomizeModule->onRandomize();
		randomizeModule = NULL;
	}

	// Param smoothing
	{
		Module *localSmoothModule = smoothModule;
		int localSmoothParamId = smoothParamId;
		float localSmoothValue = smoothValue;
		if (localSmoothModule) {
			float value = localSmoothModule->params[localSmoothParamId].value;
			const float lambda = 60.0; // decay rate is 1 graphics frame
			float delta = localSmoothValue - value;
			float newValue = value + delta * lambda * sampleTime;
			if (value == newValue) {
				// Snap to actual smooth value if the value doesn't change enough (due to the granularity of floats)
				localSmoothModule->params[localSmoothParamId].value = localSmoothValue;
				smoothModule = NULL;
			}
			else {
				localSmoothModule->params[localSmoothParamId].value = newValue;
			}
		}
	}

	completedProcessors = 0;

	// Step modules
	int blocksize = max(1, gModules.size() / ((audioProcessors.size()+1) * 3));
	moduleIndex = blocksize * (audioProcessors.size()+1);
	int modulecount = gModules.size();
	int blockend = blocksize;
	for (auto audioProcessor : audioProcessors) {
		audioProcessor->Step(blocksize, blockend);
		blockend += blocksize;
	}

	int first = blockend - blocksize;
	while (first < modulecount) {
		for (int next = first; next < blockend && next < modulecount; next += 1) {
			moduleStep(gModules[modulecount-next-1]);
		}

		blockend = moduleIndex += blocksize;
		first = blockend - blocksize;
	}

	auto waitingFor = audioProcessors.size();
	while (completedProcessors < waitingFor) {
		mainEngineSleepMutex.lock();
		mainEngineSleepMutex.unlock();
	}

	// Step cables by moving their output values to inputs
	for (Wire *wire : gWires) {
		wire->step();
	}
}

static void engineRun() {
	// Set CPU to flush-to-zero (FTZ) and denormals-are-zero (DAZ) mode
	// https://software.intel.com/en-us/node/682949
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

	// Every time the engine waits and locks a mutex, it steps this many frames
	const int mutexSteps = 64;
	// Time in seconds that the engine is rushing ahead of the estimated clock time
	double ahead = 0.0;
	auto lastTime = std::chrono::high_resolution_clock::now();

	while (running) {
		vipMutex.wait();

		if (!gPaused) {
			std::lock_guard<std::mutex> lock(mutex);
			for (int i = 0; i < mutexSteps; i++) {
				engineStep();
			}
		}

		double stepTime = mutexSteps * sampleTime;
		ahead += stepTime;
		auto currTime = std::chrono::high_resolution_clock::now();
		const double aheadFactor = 2.0;
		ahead -= aheadFactor * std::chrono::duration<double>(currTime - lastTime).count();
		lastTime = currTime;
		ahead = fmaxf(ahead, 0.0);

		// Avoid pegging the CPU at 100% when there are no "blocking" modules like AudioInterface, but still step audio at a reasonable rate
		// The number of steps to wait before possibly sleeping
		const double aheadMax = 1.0; // seconds
		if (ahead > aheadMax) {
			engineSleep();
			std::this_thread::sleep_for(std::chrono::duration<double>(stepTime));
			engineWake();
		}
	}
}

void engineStart() {
	running = true;
	thread = std::thread(engineRun);
}

void engineStop() {
	running = false;
	thread.join();
}

bool engineAddAudioThread() {
	info("Add audio processing thread");

	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);

	if (audioProcessors.size() >= std::thread::hardware_concurrency()-1) return false;

	audioProcessors.push_back(new AudioProcessor());

	return true;
}

bool engineRemoveAudioThread() {
	info("Remove audio processing thread");

	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);

	if (audioProcessors.size() > 0) {
		AudioProcessor* audioProcessor = audioProcessors.back();
		audioProcessors.pop_back();
		audioProcessor->Stop();
		delete audioProcessor;

		return true;
	}

	return false;
}

int engineGetAudioThreads() {
	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);

	return audioProcessors.size() + 1;
}

void engineSetAudioThreads(int threads) {
	info("Set audio threads %d", threads);

	while (threads > engineGetAudioThreads()) {
		if (!engineAddAudioThread()) break;
	}

	while (threads < engineGetAudioThreads()) {
		if (!engineRemoveAudioThread()) break;
	}

}

void engineSleep() {
	mainEngineSleepMutex.lock();
	for (auto audioProcessor : audioProcessors) {
		audioProcessor->Sleep();
	}
}

void engineWake() {
	for (auto audioProcessor : audioProcessors) {
		audioProcessor->Wake();
	}
	mainEngineSleepMutex.unlock();
}

void engineAddModule(Module *module) {
	assert(module);
	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);
	// Check that the module is not already added
	auto it = std::find(gModules.begin(), gModules.end(), module);
	assert(it == gModules.end());
	gModules.push_back(module);
}

void engineRemoveModule(Module *module) {
	assert(module);
	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);
	// If a param is being smoothed on this module, stop smoothing it immediately
	if (module == smoothModule) {
		smoothModule = NULL;
	}
	// Check that all wires are disconnected
	for (Wire *wire : gWires) {
		assert(wire->outputModule != module);
		assert(wire->inputModule != module);
	}
	// Check that the module actually exists
	auto it = std::find(gModules.begin(), gModules.end(), module);
	assert(it != gModules.end());
	// Remove it
	gModules.erase(it);
}

void engineResetModule(Module *module) {
	resetModule = module;
}

void engineRandomizeModule(Module *module) {
	randomizeModule = module;
}

static void updateActive() {
	// Set everything to inactive
	for (Module *module : gModules) {
		for (Input &input : module->inputs) {
			input.active = false;
		}
		for (Output &output : module->outputs) {
			output.active = false;
		}
	}
	// Set inputs/outputs to active
	for (Wire *wire : gWires) {
		wire->outputModule->outputs[wire->outputId].active = true;
		wire->inputModule->inputs[wire->inputId].active = true;
	}
}

void engineAddWire(Wire *wire) {
	assert(wire);
	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);
	// Check wire properties
	assert(wire->outputModule);
	assert(wire->inputModule);
	// Check that the wire is not already added, and that the input is not already used by another cable
	for (Wire *wire2 : gWires) {
		assert(wire2 != wire);
		assert(!(wire2->inputModule == wire->inputModule && wire2->inputId == wire->inputId));
	}
	// Add the wire
	gWires.push_back(wire);
	updateActive();
}

void engineRemoveWire(Wire *wire) {
	assert(wire);
	VIPLock vipLock(vipMutex);
	std::lock_guard<std::mutex> lock(mutex);
	// Check that the wire is already added
	auto it = std::find(gWires.begin(), gWires.end(), wire);
	assert(it != gWires.end());
	// Set input to 0V
	wire->inputModule->inputs[wire->inputId].value = 0.0;
	// Remove the wire
	gWires.erase(it);
	updateActive();
}

void engineSetParam(Module *module, int paramId, float value) {
	module->params[paramId].value = value;
}

void engineSetParamSmooth(Module *module, int paramId, float value) {
	// If another param is being smoothed, jump value
	if (smoothModule && !(smoothModule == module && smoothParamId == paramId)) {
		smoothModule->params[smoothParamId].value = smoothValue;
	}
	smoothParamId = paramId;
	smoothValue = value;
	smoothModule = module;
}

void engineSetSampleRate(float newSampleRate) {
	sampleRateRequested = newSampleRate;
}

float engineGetSampleRate() {
	return sampleRate;
}

float engineGetSampleTime() {
	return sampleTime;
}

} // namespace rack
