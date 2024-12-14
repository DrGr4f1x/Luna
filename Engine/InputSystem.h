//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

namespace Luna
{

enum class DigitalInput
{
	// keyboard
	// kKey must start at zero, see m_dxKeyMapping
	kKey_escape = 0,
	kKey_1,
	kKey_2,
	kKey_3,
	kKey_4,
	kKey_5,
	kKey_6,
	kKey_7,
	kKey_8,
	kKey_9,
	kKey_0,
	kKey_minus,
	kKey_equals,
	kKey_back,
	kKey_tab,
	kKey_q,
	kKey_w,
	kKey_e,
	kKey_r,
	kKey_t,
	kKey_y,
	kKey_u,
	kKey_i,
	kKey_o,
	kKey_p,
	kKey_lbracket,
	kKey_rbracket,
	kKey_return,
	kKey_lcontrol,
	kKey_a,
	kKey_s,
	kKey_d,
	kKey_f,
	kKey_g,
	kKey_h,
	kKey_j,
	kKey_k,
	kKey_l,
	kKey_semicolon,
	kKey_apostrophe,
	kKey_grave,
	kKey_lshift,
	kKey_backslash,
	kKey_z,
	kKey_x,
	kKey_c,
	kKey_v,
	kKey_b,
	kKey_n,
	kKey_m,
	kKey_comma,
	kKey_period,
	kKey_slash,
	kKey_rshift,
	kKey_multiply,
	kKey_lalt,
	kKey_space,
	kKey_capital,
	kKey_f1,
	kKey_f2,
	kKey_f3,
	kKey_f4,
	kKey_f5,
	kKey_f6,
	kKey_f7,
	kKey_f8,
	kKey_f9,
	kKey_f10,
	kKey_numlock,
	kKey_scroll,
	kKey_numpad7,
	kKey_numpad8,
	kKey_numpad9,
	kKey_subtract,
	kKey_numpad4,
	kKey_numpad5,
	kKey_numpad6,
	kKey_add,
	kKey_numpad1,
	kKey_numpad2,
	kKey_numpad3,
	kKey_numpad0,
	kKey_decimal,
	kKey_f11,
	kKey_f12,
	kKey_numpadenter,
	kKey_rcontrol,
	kKey_divide,
	kKey_sysrq,
	kKey_ralt,
	kKey_pause,
	kKey_home,
	kKey_up,
	kKey_pgup,
	kKey_left,
	kKey_right,
	kKey_end,
	kKey_down,
	kKey_pgdn,
	kKey_insert,
	kKey_delete,
	kKey_lwin,
	kKey_rwin,
	kKey_apps,

	kNumKeys,

	// gamepad
	kDPadUp = kNumKeys,
	kDPadDown,
	kDPadLeft,
	kDPadRight,
	kStartButton,
	kBackButton,
	kLThumbClick,
	kRThumbClick,
	kLShoulder,
	kRShoulder,
	kAButton,
	kBButton,
	kXButton,
	kYButton,

	// mouse
	kMouse0,
	kMouse1,
	kMouse2,
	kMouse3,
	kMouse4,
	kMouse5,
	kMouse6,
	kMouse7,

	kNumDigitalInputs
};


enum class AnalogInput
{
	// gamepad
	kAnalogLeftTrigger,
	kAnalogRightTrigger,
	kAnalogLeftStickX,
	kAnalogLeftStickY,
	kAnalogRightStickX,
	kAnalogRightStickY,

	// mouse
	kAnalogMouseX,
	kAnalogMouseY,
	kAnalogMouseScroll,

	kNumAnalogInputs
};


class InputSystem
{
public:
	explicit InputSystem(HWND hwnd);
	~InputSystem();

	void Update(float deltaT);

	bool IsAnyPressed() const;

	bool IsPressed(DigitalInput di) const;
	bool IsFirstPressed(DigitalInput di) const;
	bool IsReleased(DigitalInput di) const;
	bool IsFirstReleased(DigitalInput di) const;

	float GetDurationPressed(DigitalInput di) const;

	float GetAnalogInput(AnalogInput ai) const;
	float GetTimeCorrectedAnalogInput(AnalogInput ai) const;

	void SetCaptureMouse(bool capture) { m_captureMouse = capture; }
	bool GetCaptureMouse() const { return m_captureMouse; }

private:
	void Initialize();
	void Shutdown();

	void KbmBuildKeyMapping();
	void KbmZeroInputs();
	void KbmUpdate();

	inline void SetAnalog(AnalogInput input, float value)
	{
		int index = static_cast<int>(input);
		m_analogs[index] = value;
	}

private:
	HWND m_hwnd{};

	bool m_buttons[2][(int)DigitalInput::kNumDigitalInputs];
	float m_holdDuration[(int)DigitalInput::kNumDigitalInputs] = { 0.0f };
	float m_analogs[(int)AnalogInput::kNumAnalogInputs];
	float m_analogsTC[(int)AnalogInput::kNumAnalogInputs];

	DIMOUSESTATE2 m_mouseState{};
	unsigned char m_keybuffer[256];
	unsigned char m_dxKeyMapping[(int)DigitalInput::kNumKeys]; // map DigitalInput enum to DX key codes 

	IDirectInput8A* m_di{ nullptr };
	IDirectInputDevice8A* m_keyboard{ nullptr };
	IDirectInputDevice8A* m_mouse{ nullptr };
	bool m_captureMouse{ false };
};

inline LogCategory LogInput{ "LogInput" };

} // namespace Luna