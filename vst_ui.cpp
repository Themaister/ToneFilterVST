#define VK_USE_PLATFORM_WIN32_KHR

#include <ui_manager.hpp>
#include "vst_ui.hpp"
#include "vst_plugin.hpp"
#include "horizontal_packing.hpp"
#include "vertical_packing.hpp"
#include "window.hpp"
#include "slider.hpp"
#include "label.hpp"
#include "image_widget.hpp"
#include "os.hpp"
#include <windowsx.h>
#include <string>
#include <toggle_button.hpp>
#include <click_button.hpp>

using namespace Vulkan;
using namespace Granite;
using namespace std;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	auto *editor = reinterpret_cast<FMEditor *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	switch (umsg)
	{
	case WM_PAINT:
		editor->render();
		break;

	case WM_MOUSEMOVE:
		editor->mouse_move(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		return 0;

	case WM_MOUSELEAVE:
		editor->mouse_leave();
		return 0;

	case WM_LBUTTONDOWN:
		editor->mouse_button(MouseButton::Left, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), true);
		return 0;

	case WM_LBUTTONUP:
		editor->mouse_button(MouseButton::Left, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), false);
		return 0;

	case WM_RBUTTONDOWN:
		editor->mouse_button(MouseButton::Right, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), true);
		return 0;

	case WM_RBUTTONUP:
		editor->mouse_button(MouseButton::Right, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), false);
		return 0;

	case WM_MBUTTONDOWN:
		editor->mouse_button(MouseButton::Middle, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), true);
		return 0;

	case WM_MBUTTONUP:
		editor->mouse_button(MouseButton::Middle, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), false);
		return 0;
	}
	return DefWindowProc(hwnd, umsg, wparam, lparam);
}

FMEditor::FMEditor(FMSynth *plug)
	: AEffEditor(plug), synth(plug)
{
	Filesystem::get().register_protocol("assets", make_unique<OSFilesystem>(ASSETS));
	instance = GetModuleHandle(nullptr);
	rect.right = 1075;
	rect.bottom = 620;

	WNDCLASSEXA cls = {};
	cls.cbSize = sizeof(cls);
	cls.style = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc = WindowProc;
	cls.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
	cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
	cls.hInstance = instance;
	cls.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	cls.lpszClassName = "FMEditor";
	cls.hIconSm = LoadIconA(nullptr, IDI_WINLOGO);

	RegisterClassExA(&cls);

	ui_up_to_date.clear();

	EVENT_MANAGER_REGISTER_LATCH(FMEditor, on_device_created, on_device_destroyed, Vulkan::DeviceCreatedEvent);
}

static const vec4 bg_color(0.02f, 0.01f, 0.01f, 1.0f);
static const vec4 bg_color_op(0.08f, 0.02f, 0.06f, 1.0f);
static const vec4 main_color(0.0f, 0.3f, 0.4f, 1.0f);

UI::Slider &FMEditor::add_slider(Granite::UI::Widget &window, const char *name, unsigned op, unsigned index,
                                 int x, int y, int w, int h)
{
	auto slider = Util::make_abstract_handle<UI::Widget, UI::Slider>();
	window.add_child(slider);

	auto &sli = static_cast<UI::Slider &>(*slider);
	sli.show_tooltip(true);
	sli.show_label(true);
	sli.show_value(false);
	sli.set_floating(true);
	sli.set_margin(2.0f);
	sli.set_range(0.0f, 1.0f);
	sli.set_value(1.0f);
	sli.set_label_slider_gap(3.0f);
	sli.set_text(name);
	sli.set_color(main_color);
	sli.set_background_color(bg_color);
	sli.set_floating_position(vec2(x, y));
	sli.set_size(vec2(w, h));

	if (w < h)
		sli.set_orientation(UI::Slider::Orientation::Vertical);
	else
		sli.set_orientation(UI::Slider::Orientation::Horizontal);

	sli.on_value_changed([this, op, index](float value) {
		if (!update_lock)
			synth->set_automated_parameter(op, index, value);
	});

	refresh_parameters[FMSYNTH_GLOBAL_PARAM_END + FMSYNTH_PARAM_END * op + index] = [this, &sli](float value) {
		update_lock = true;
		sli.set_value(value);
		update_lock = false;
	};

	return sli;
}

void FMEditor::add_global_slider(Granite::UI::Widget &window, const char *name, unsigned index, int x, int y)
{
	auto vbox = Util::make_abstract_handle<UI::Widget, UI::VerticalPacking>();
	auto &v = static_cast<UI::VerticalPacking &>(*vbox);
	auto hbox = Util::make_abstract_handle<UI::Widget, UI::HorizontalPacking>();
	v.set_floating(true);
	v.set_floating_position(vec2(x, y));
	v.set_margin(5.0f);
	window.add_child(vbox);

	auto name_label = Util::make_abstract_handle<UI::Widget, UI::Label>();
	auto &n = static_cast<UI::Label &>(*name_label);
	n.set_text(name);
	v.add_child(name_label);
	v.add_child(hbox);

	auto &h = static_cast<UI::HorizontalPacking &>(*hbox);
	h.set_margin(5.0f);

	auto slider = Util::make_abstract_handle<UI::Widget, UI::Slider>();
	h.add_child(slider);

	auto label = Util::make_abstract_handle<UI::Widget, UI::Label>();
	auto &l = static_cast<UI::Label &>(*label);
	v.add_child(label);

	l.set_minimum_geometry(vec2(25.0f));
	l.set_target_geometry(vec2(25.0f));
	l.set_margin(5.0f);
	l.set_color(main_color);
	l.set_background_color(bg_color);

	auto &sli = static_cast<UI::Slider &>(*slider);
	sli.set_margin(2.0f);
	sli.set_background_color(bg_color);
	sli.show_value(false);
	sli.show_label(false);
	sli.set_range(0.0f, 1.0f);
	sli.set_value(0.5f);
	sli.set_label_slider_gap(3.0f);
	sli.set_text(name);
	sli.set_size(vec2(80.0f, 20.0f));
	sli.set_color(main_color);
	sli.on_value_changed([this, index, &l](float value) {
		if (!update_lock)
			synth->set_automated_global_parameter(index, value);

		auto real_value = to_string(fmsynth_convert_from_normalized_global_parameter(synth->get_synth(),
		                                                                             index, value));
		l.set_text(real_value);
	});

	refresh_parameters[index] = [this, &sli](float value) {
		update_lock = true;
		sli.set_value(value);
		update_lock = false;
	};
}

void FMEditor::add_operator(Granite::UI::Widget &window, unsigned index, int x, int y)
{
	auto vbox = Util::make_abstract_handle<UI::Widget, UI::VerticalPacking>();
	window.add_child(vbox);
	vbox->set_floating(true);
	vbox->set_background_color(bg_color_op);
	vbox->set_floating_position(vec2(x, y));
	vbox->set_minimum_geometry(vec2(256.0f));
	vbox->set_target_geometry(vec2(256.0f));
	vbox->set_margin(5.0f);

	auto label = Util::make_abstract_handle<UI::Widget, UI::Label>();
	auto &l = static_cast<UI::Label &>(*label);
	l.set_target_geometry(vec2(256.0f - 10.0f, 240.0f - 10.0f));
	l.set_minimum_geometry(vec2(256.0f - 10.0f, 240.0f - 10.0f));
	l.set_font_alignment(Font::Alignment::TopRight);
	l.set_color(main_color);
	l.set_text(string("OP #") + to_string(index));
	l.set_floating(true);
	vbox->add_child(label);

	add_slider(*vbox, "Amp", index, FMSYNTH_PARAM_AMP, 0, 0, 10, 50);
	add_slider(*vbox, "LA", index, FMSYNTH_PARAM_LFO_AMP_SENSITIVITY, 0, 80, 10, 30);
	add_slider(*vbox, "LF", index, FMSYNTH_PARAM_LFO_FREQ_MOD_DEPTH, 25, 80, 10, 30);

	add_slider(*vbox, "KL", index, FMSYNTH_PARAM_KEYBOARD_SCALING_LOW_FACTOR, 55, 80, 10, 30);
	add_slider(*vbox, "KM", index, FMSYNTH_PARAM_KEYBOARD_SCALING_MID_POINT, 75, 80, 10, 30);
	add_slider(*vbox, "KH", index, FMSYNTH_PARAM_KEYBOARD_SCALING_HIGH_FACTOR, 95, 80, 10, 30);

	add_slider(*vbox, "T0", index, FMSYNTH_PARAM_ENVELOPE_TARGET0, 0, 140, 15, 55);
	add_slider(*vbox, "T1", index, FMSYNTH_PARAM_ENVELOPE_TARGET1, 25, 140, 15, 55);
	add_slider(*vbox, "T2", index, FMSYNTH_PARAM_ENVELOPE_TARGET2, 50, 140, 15, 55);

	add_slider(*vbox, "D0", index, FMSYNTH_PARAM_DELAY0, 90, 140, 15, 55);
	add_slider(*vbox, "D1", index, FMSYNTH_PARAM_DELAY1, 115, 140, 15, 55);
	add_slider(*vbox, "D2", index, FMSYNTH_PARAM_DELAY2, 140, 140, 15, 55);
	add_slider(*vbox, "DR", index, FMSYNTH_PARAM_RELEASE_TIME, 165, 140, 15, 55);

	add_slider(*vbox, "FOff", index, FMSYNTH_PARAM_FREQ_OFFSET, 35, 50, 50, 10);

	add_slider(*vbox, "VS", index, FMSYNTH_PARAM_VELOCITY_SENSITIVITY, 120, 80, 50, 10);
	add_slider(*vbox, "MS", index, FMSYNTH_PARAM_MOD_WHEEL_SENSITIVITY, 120, 100, 50, 10);
	add_slider(*vbox, "Pan", index, FMSYNTH_PARAM_PAN, 120, 120, 50, 10);

	auto mod_box = Util::make_abstract_handle<UI::Widget, UI::VerticalPacking>();
	auto &mbox = static_cast<UI::VerticalPacking &>(*mod_box);
	vbox->add_child(mod_box);
	mbox.set_floating(true);
	mbox.set_floating_position(vec2(210.0f, 30.0f));
	mbox.set_margin(5.0f);
	mbox.set_background_color(bg_color);

	for (unsigned i = 0; i < 9; i++)
	{
		auto button = Util::make_abstract_handle<UI::Widget, UI::ToggleButton>();
		mbox.add_child(button);
		auto &btn = static_cast<UI::ToggleButton &>(*button);
		btn.set_toggled_font_color(vec4(0.8f, 0.5f, 0.3f, 1.0f));
		btn.set_untoggled_font_color(main_color);
		btn.set_background_color(bg_color);
		btn.set_margin(2.0f);

		if (i == 8)
		{
			btn.set_text("C");
			btn.on_toggle([this, index, &btn](bool state)
			              {
				              if (!update_lock)
					              synth->set_automated_parameter(index, FMSYNTH_PARAM_CARRIERS, state ? 1.0f : 0.0f);
			              });

			refresh_parameters[FMSYNTH_GLOBAL_PARAM_END + FMSYNTH_PARAM_END * index + FMSYNTH_PARAM_CARRIERS] =
				[this, &btn](float value)
				{
					update_lock = true;
					btn.set_toggled(value >= 0.5f);
					update_lock = false;
				};
		}
		else
		{
			btn.set_text(string("#") + to_string(i));
			btn.on_toggle([this, index, i, &btn](bool state)
			              {
				              if (!update_lock)
					              synth->set_automated_parameter(index, FMSYNTH_PARAM_MOD_TO_CARRIERS0 + i, state ? 1.0f : 0.0f);
			              });

			refresh_parameters[FMSYNTH_GLOBAL_PARAM_END + FMSYNTH_PARAM_END * index + FMSYNTH_PARAM_MOD_TO_CARRIERS0 + i] =
				[this, &btn](float value)
				{
					update_lock = true;
					btn.set_toggled(value >= 0.5f);
					update_lock = false;
				};
		}
	}

	label = Util::make_abstract_handle<UI::Widget, UI::Label>();
	auto &freq = static_cast<UI::Label &>(*label);
	vbox->add_child(label);
	freq.set_floating(true);
	freq.set_floating_position(vec2(0.0f, 40.0f));
	freq.set_font_alignment(Font::Alignment::Center);
	freq.set_target_geometry(vec2(30.0f, 30.0f));
	freq.set_floating_position(vec2(30.0f, 0.0f));
	freq.set_margin(5.0f);
	freq.set_background_color(bg_color);
	freq.set_color(main_color);

	refresh_parameters[FMSYNTH_GLOBAL_PARAM_END + FMSYNTH_PARAM_END * index + FMSYNTH_PARAM_FREQ_MOD] =
		[this, &freq](float value)
		{
			float rounded = round(fmsynth_convert_from_normalized_parameter(synth->get_synth(),
			                                                                FMSYNTH_PARAM_FREQ_MOD, value) * 100.0f) / 100.0f;
			char buffer[16];
			sprintf(buffer, "%5.2f", rounded);
			freq.set_text(buffer);
		};

	auto button_vbox = Util::make_abstract_handle<UI::Widget, UI::VerticalPacking>();
	button_vbox->set_floating(true);
	button_vbox->set_floating_position(vec2(80.0f, 0.0f));
	vbox->add_child(button_vbox);

	for (unsigned y = 0; y < 2; y++)
	{
		auto hbox = Util::make_abstract_handle<UI::Widget, UI::HorizontalPacking>();
		hbox->set_margin(2.0f);
		button_vbox->add_child(hbox);

		for (unsigned x = 0; x < 3; x++)
		{
			auto button = Util::make_abstract_handle<UI::Widget, UI::ClickButton>();
			hbox->add_child(button);
			auto &btn = static_cast<UI::ClickButton &>(*button);
			btn.set_background_color(bg_color);
			btn.set_font_color(main_color);
			btn.set_margin(3.0f);

			const char *increment = nullptr;
			switch (x)
			{
			default:
			case 0:
				increment = y ? "-1.00" : "+1.00";
				break;
			case 1:
				increment = y ? "-0.10" : "+0.10";
				break;
			case 2:
				increment = y ? "-0.01" : "+0.01";
				break;
			}

			btn.set_text(increment);

			btn.on_click([this, &freq, index, x, y]() {
				float offset;
				switch (x)
				{
				default:
				case 0:
					offset = 1.0f;
					break;
				case 1:
					offset = 0.1f;
					break;
				case 2:
					offset = 0.01f;
					break;
				}
				if (y)
					offset = -offset;
				float rounding = round(abs(1.0f / offset));

				float value = synth->getParameter(FMSYNTH_GLOBAL_PARAM_END + FMSYNTH_PARAM_END * index + FMSYNTH_PARAM_FREQ_MOD);
				value = fmsynth_convert_from_normalized_parameter(synth->get_synth(), FMSYNTH_PARAM_FREQ_MOD, value);
				value += offset;

				float rounded = round(value * rounding) / rounding;
				float normalized = fmsynth_convert_to_normalized_parameter(synth->get_synth(),
				                                                           FMSYNTH_PARAM_FREQ_MOD,
				                                                           rounded);

				normalized = clamp(normalized, 0.0f, 1.0f);

				synth->set_automated_parameter(index, FMSYNTH_PARAM_FREQ_MOD, normalized);
				char buffer[64];
				sprintf(buffer, "%05.2f", rounded);
				freq.set_text(buffer);
			});
		}
	}
}

void FMEditor::on_device_created(const Vulkan::DeviceCreatedEvent &)
{
	auto &ui = UI::UIManager::get();
	ui.reset_children();

	refresh_parameters.clear();
	refresh_parameters.resize(FMSynthParameters);

	auto window = Util::make_abstract_handle<UI::Widget, UI::Window>();
	auto &win = static_cast<UI::Window &>(*window);

	ui.add_child(window);
	win.set_floating(false);
	win.set_minimum_geometry(vec2(rect.right, rect.bottom));
	win.set_target_geometry(vec2(rect.right, rect.bottom));
	win.set_fullscreen(true);
	win.show_title_bar(false);
	win.set_background_color(vec4(0.0f));

	add_global_slider(win, "Volume", FMSYNTH_GLOBAL_PARAM_VOLUME, 5, 5);
	add_global_slider(win, "LFO frequency", FMSYNTH_GLOBAL_PARAM_LFO_FREQ, 120, 5);

	static const unsigned op_width = 256 + 8;
	static const unsigned op_height = 240 + 8;
	add_operator(win, 0, 10 + 0 * op_width, 120);
	add_operator(win, 1, 10 + 1 * op_width, 120);
	add_operator(win, 2, 10 + 2 * op_width, 120);
	add_operator(win, 3, 10 + 3 * op_width, 120);
	add_operator(win, 4, 10 + 0 * op_width, 120 + op_height);
	add_operator(win, 5, 10 + 1 * op_width, 120 + op_height);
	add_operator(win, 6, 10 + 2 * op_width, 120 + op_height);
	add_operator(win, 7, 10 + 3 * op_width, 120 + op_height);

	auto texture = Util::make_abstract_handle<UI::Widget, UI::Image>("assets://textures/oktopus.png");
	auto &tex = static_cast<UI::Image &>(*texture);
	tex.set_filter(Vulkan::StockSampler::NearestClamp);
	tex.set_floating(true);
	tex.set_floating_position(vec2(400.0f, 10.0f));
	tex.set_target_geometry(vec2(128.0f, 32.0f) * 3.0f);
	tex.set_minimum_geometry(vec2(128.0f, 32.0f) * 3.0f);
	window->add_child(texture);
}

void FMEditor::on_device_destroyed(const Vulkan::DeviceCreatedEvent &)
{
}

void FMEditor::poll_input()
{
}

void FMEditor::post_update()
{
	ui_up_to_date.clear();
}

FMEditor::~FMEditor()
{
	UnregisterClassA("FMEditor", instance);
}

bool FMEditor::getRect(ERect **rect)
{
	*rect = &this->rect;
	return true;
}

void FMEditor::close()
{
	AEffEditor::close();
	wsi->deinit_surface_and_swapchain();

	if (window)
	{
		CloseWindow(window);
		window = nullptr;
	}
}

bool FMEditor::open(void *ptr)
{
	AEffEditor::open(ptr);

	window = CreateWindowEx(0, "FMEditor", "Window",
	                        WS_CHILD | WS_VISIBLE,
	                        0, 0, rect.right, rect.bottom,
	                        static_cast<HWND>(systemWindow),
	                        nullptr, instance, nullptr);

	SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	if (!wsi)
	{
		Context::init_loader(nullptr);

		wsi.reset(new WSI);
		wsi->set_platform(this);
		if (!wsi->init())
		{
			wsi.reset();
			return false;
		}
	}
	else
	{
		auto surface = create_surface(wsi->get_context().get_instance(), wsi->get_context().get_gpu());
		wsi->init_surface_and_swapchain(surface);
	}

	return true;
}

void FMEditor::mouse_leave()
{
	has_focus = false;
	get_input_tracker().mouse_leave();
}

void FMEditor::mouse_move(int x, int y)
{
	if (!has_focus)
	{
		get_input_tracker().mouse_enter(x, y);
		has_focus = true;
	}

	get_input_tracker().mouse_move_event(x, y);
	post_update();
}

void FMEditor::mouse_button(Granite::MouseButton button, int x, int y, bool pressed)
{
	get_input_tracker().mouse_button_event(button, x, y, pressed);
	post_update();
}

void FMEditor::render()
{
	// Don't bother dispatching the global state.
	for (auto &refresh : refresh_parameters)
		if (refresh)
			refresh(synth->getParameter(VstInt32(&refresh - refresh_parameters.data())));

	Granite::EventManager::get_global().dispatch();
	Filesystem::get().poll_notifications();

	wsi->begin_frame();
	Device &device = wsi->get_device();
	auto rp = device.get_swapchain_render_pass(SwapchainRenderPass::Depth);
	rp.clear_color[0].float32[0] = bg_color.r;
	rp.clear_color[0].float32[1] = bg_color.g;
	rp.clear_color[0].float32[2] = bg_color.b;
	rp.clear_color[0].float32[3] = bg_color.a;
	auto cmd = device.request_command_buffer();
	cmd->begin_render_pass(rp);
	UI::UIManager::get().render(*cmd);
	cmd->end_render_pass();
	device.submit(cmd);
	wsi->end_frame();
}

void FMEditor::idle()
{
	AEffEditor::idle();
	wsi->get_platform().alive(*wsi);
}

VkSurfaceKHR FMEditor::create_surface(VkInstance instance, VkPhysicalDevice)
{
	PFN_vkCreateWin32SurfaceKHR create;
	VULKAN_SYMBOL_WRAPPER_LOAD_INSTANCE_SYMBOL(instance, "vkCreateWin32SurfaceKHR", create);

	VkWin32SurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	info.hwnd = window;
	info.hinstance = this->instance;

	VkSurfaceKHR surface;
	if (create(instance, &info, nullptr, &surface) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return surface;
}

unsigned FMEditor::get_surface_width()
{
	return rect.right;
}

unsigned FMEditor::get_surface_height()
{
	return rect.bottom;
}

bool FMEditor::alive(Vulkan::WSI &)
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return false;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (!ui_up_to_date.test_and_set())
	{
		RedrawWindow(window, nullptr, nullptr, RDW_INTERNALPAINT);
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				return false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return true;
}

std::vector<const char *> FMEditor::get_instance_extensions()
{
	return { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
}
