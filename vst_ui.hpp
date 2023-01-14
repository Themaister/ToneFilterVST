#pragma once

#include "aeffeditor.h"

#include "vulkan.hpp"
#include "wsi.hpp"
#include "flat_renderer.hpp"
#include <memory>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class FMSynth;

namespace Granite
{
namespace UI
{
class Widget;
class Slider;
}
}

class FMEditor : public AEffEditor, public Vulkan::WSIPlatform, public Granite::EventHandler
{
public:
	FMEditor(FMSynth *plug);
	~FMEditor();

	void post_update();

	bool getRect (ERect **rect) override;
	bool open(void *ptr) override;
	void close() override;
	void idle() override;

	unsigned get_surface_width() override;
	unsigned get_surface_height() override;
	VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice gpu) override;
	std::vector<const char *> get_instance_extensions() override;
	bool alive(Vulkan::WSI &wsi) override;
	void poll_input() override;

	void render();

	void mouse_move(int x, int y);
	void mouse_leave();
	void mouse_button(Granite::MouseButton button, int x, int y, bool pressed);

private:
	FMSynth *plug;
	std::unique_ptr<Vulkan::WSI> wsi;
	ERect rect = {};

	Granite::FlatRenderer renderer;

	HINSTANCE instance = nullptr;
	HWND window = nullptr;
	std::atomic_flag ui_up_to_date;
	bool has_focus = false;

	void on_device_created(const Vulkan::DeviceCreatedEvent &e);
	void on_device_destroyed(const Vulkan::DeviceCreatedEvent &e);
	FMSynth *synth;
	bool update_lock = false;

	std::vector<std::function<void (float)>> refresh_parameters;

	void add_global_slider(Granite::UI::Widget &window, const char *name, unsigned index, int x, int y);
	void add_operator(Granite::UI::Widget &window, unsigned index, int x, int y);
	Granite::UI::Slider &add_slider(Granite::UI::Widget &window, const char *name, unsigned op, unsigned index, int x, int y, int w, int h);
};
