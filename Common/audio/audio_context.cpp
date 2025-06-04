#include <iostream>
#include <array>
#include <AL/alc.h>
#include <AL/alext.h>
#include "audio_context.hpp"
#include "audiosource_pool.hpp"
using namespace std;

static constexpr auto GetDebugSourceName(ALenum source) noexcept -> std::string_view
{
	switch (source)
	{
	case AL_DEBUG_SOURCE_API_EXT: return "API"sv;
	case AL_DEBUG_SOURCE_AUDIO_SYSTEM_EXT: return "Audio System"sv;
	case AL_DEBUG_SOURCE_THIRD_PARTY_EXT: return "Third Party"sv;
	case AL_DEBUG_SOURCE_APPLICATION_EXT: return "Application"sv;
	case AL_DEBUG_SOURCE_OTHER_EXT: return "Other"sv;
	}
	return "<invalid source>"sv;
}

static constexpr auto GetDebugTypeName(ALenum type) noexcept -> std::string_view
{
	switch (type)
	{
	case AL_DEBUG_TYPE_ERROR_EXT: return "Error"sv;
	case AL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_EXT: return "Deprecated Behavior"sv;
	case AL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_EXT: return "Undefined Behavior"sv;
	case AL_DEBUG_TYPE_PORTABILITY_EXT: return "Portability"sv;
	case AL_DEBUG_TYPE_PERFORMANCE_EXT: return "Performance"sv;
	case AL_DEBUG_TYPE_MARKER_EXT: return "Marker"sv;
	case AL_DEBUG_TYPE_PUSH_GROUP_EXT: return "Push Group"sv;
	case AL_DEBUG_TYPE_POP_GROUP_EXT: return "Pop Group"sv;
	case AL_DEBUG_TYPE_OTHER_EXT: return "Other"sv;
	}
	return "<invalid type>"sv;
}

static constexpr auto GetDebugSeverityName(ALenum severity) noexcept -> std::string_view
{
	switch (severity)
	{
	case AL_DEBUG_SEVERITY_HIGH_EXT: return "High"sv;
	case AL_DEBUG_SEVERITY_MEDIUM_EXT: return "Medium"sv;
	case AL_DEBUG_SEVERITY_LOW_EXT: return "Low"sv;
	case AL_DEBUG_SEVERITY_NOTIFICATION_EXT: return "Notification"sv;
	}
	return "<invalid severity>"sv;
}

static ALCdevice* device;
static ALCcontext* context;

#ifdef _DEBUG
static LPALDEBUGMESSAGECALLBACKEXT alDebugMessageCallbackEXT;
static LPALDEBUGMESSAGEINSERTEXT alDebugMessageInsertEXT;
static LPALDEBUGMESSAGECONTROLEXT alDebugMessageControlEXT;
static LPALPUSHDEBUGGROUPEXT alPushDebugGroupEXT;
static LPALPOPDEBUGGROUPEXT alPopDebugGroupEXT;
static LPALGETDEBUGMESSAGELOGEXT alGetDebugMessageLogEXT;
static LPALOBJECTLABELEXT alObjectLabelEXT;
static LPALGETOBJECTLABELEXT alGetObjectLabelEXT;
static LPALGETPOINTEREXT alGetPointerEXT;
static LPALGETPOINTERVEXT alGetPointervEXT;
#endif

void Audio::initContext(ObjectManager& manager) {
	device = alcOpenDevice(nullptr);

	if (!device)
	{
		std::cout << "Failed to open audio device!\n";
		exit(1);
	}
	ALuint flags = 0;
#ifdef _DEBUG
	bool debugExt = alcIsExtensionPresent(device, "ALC_EXT_debug");
	if (debugExt)
	{
		flags = ALC_CONTEXT_DEBUG_BIT_EXT;
	}
	else {
		cout << "ALC_EXT_debug supported on device\n";
	}
#endif

	std::array<ALCint, 3> attributes{ ALC_CONTEXT_FLAGS_EXT, flags, 0 };
	context = alcCreateContext(device, attributes.data());
	if (!context) {
		std::cout << "Failed to create OpenAL context!\n";
		exit(1);
	}
	if (!alcMakeContextCurrent(context)) {
		std::cout << "Failed to set current OpenAL context!\n";
		exit(1);
	}
#ifdef _DEBUG
	if (debugExt) {
#define LOAD_PROC(N) N = reinterpret_cast<decltype(N)>(alcGetProcAddress(device, #N))
		LOAD_PROC(alDebugMessageCallbackEXT);
		LOAD_PROC(alDebugMessageInsertEXT);
		LOAD_PROC(alDebugMessageControlEXT);
		LOAD_PROC(alPushDebugGroupEXT);
		LOAD_PROC(alPopDebugGroupEXT);
		LOAD_PROC(alGetDebugMessageLogEXT);
		LOAD_PROC(alObjectLabelEXT);
		LOAD_PROC(alGetObjectLabelEXT);
		LOAD_PROC(alGetPointerEXT);
		LOAD_PROC(alGetPointervEXT);
#undef LOAD_PROC
		alDebugMessageControlEXT(AL_DONT_CARE_EXT, AL_DONT_CARE_EXT, AL_DEBUG_SEVERITY_LOW_EXT, 0,
			nullptr, AL_TRUE);
		alEnable(AL_DEBUG_OUTPUT_EXT);

		static constexpr auto debug_callback = [](ALenum source, ALenum type, ALuint id,
			ALenum severity, ALsizei length, const ALchar* message, void* userParam [[maybe_unused]] )
			noexcept -> void
			{
				/* The message length provided to the callback does not include the
				 * null terminator.
				 */
				const auto msgstr = std::string_view{ message, static_cast<ALuint>(length) };
				cout << ("Got message from callback:\n"
					"  Source: {}\n"
					"  Type: {}\n"
					"  ID: {}\n"
					"  Severity: {}\n"
					"  Message: \"{}\"", GetDebugSourceName(source), GetDebugTypeName(type), id,
					GetDebugSeverityName(severity), msgstr);
			};
		alDebugMessageCallbackEXT(debug_callback, nullptr);
	}
#endif
	initializeAudioSourcePool(manager);
}

void Audio::destroyContext() {
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}