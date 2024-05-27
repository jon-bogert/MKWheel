#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <Windows.h>

#include <hidapi.h>
#include <ViGEm/Client.h>

#define BUFFER_SIZE 10

int g_steerMax = 5000;
int g_deadZone = 2048;
int g_gasTheshold = 127;
int g_brakeTheshold = 55;

struct WheelStatus
{
    uint16_t buttons = 0;
    uint16_t wheelRotation = 32768;
    uint8_t gasPedal = 255;
    uint8_t brakePedal = 255;
    uint8_t clutchPedal = 255;
};

enum WheelButton : uint16_t
{
    WHEEL_BUTTON_NONE =      0x0000,
    WHEEL_BUTTON_DPAD_UP =      0x0001,
    WHEEL_BUTTON_DPAD_RIGHT =   0x0002,
    WHEEL_BUTTON_DPAD_DOWN =    0x0004,
    WHEEL_BUTTON_DPAD_LEFT =    0x0008,
    WHEEL_BUTTON_A =         0x0010,
    WHEEL_BUTTON_B =         0x0020,
    WHEEL_BUTTON_X =         0x0040,
    WHEEL_BUTTON_Y =         0x0080,
    WHEEL_BUTTON_R_TRIG =    0x0100,
    WHEEL_BUTTON_L_TRIG =    0x0200,
    WHEEL_BUTTON_START =     0x0400,
    WHEEL_BUTTON_GUIDE =    0x0800,
    WHEEL_BUTTON_RIGHT_SHOULDER =    0x1000,
    WHEEL_BUTTON_LEFT_SHOULDER =    0x2000,
};

int Remap(int oldLower, int oldUpper, int newLower, int newUpper, int value)
{
    int rangeFactor = (newUpper - newLower) / (oldUpper - oldLower);
    return (value - oldLower) * rangeFactor + newLower;
}

float Clamp(float value, float min, float max)
{ 
    if (value > max)
        return max;
    if (value < min)
        return min;

    return value;
}

uint16_t FixDPad(uint16_t buttons)
{
    uint16_t pad = buttons & 0x000F;
    buttons &= 0xFFF0;

    switch (pad)
    {
    case 0:
        buttons |= WHEEL_BUTTON_DPAD_UP;
        break;
    case 1:
        buttons |= WHEEL_BUTTON_DPAD_UP & WHEEL_BUTTON_DPAD_RIGHT;
        break;
    case 2:
        buttons |= WHEEL_BUTTON_DPAD_RIGHT;
        break;
    case 3:
        buttons |= WHEEL_BUTTON_DPAD_DOWN & WHEEL_BUTTON_DPAD_RIGHT;
        break;
    case 4:
        buttons |= WHEEL_BUTTON_DPAD_DOWN;
        break;
    case 5:
        buttons |= WHEEL_BUTTON_DPAD_DOWN & WHEEL_BUTTON_DPAD_LEFT;
        break;
    case 6:
        buttons |= WHEEL_BUTTON_DPAD_LEFT;
        break;
    case 7:
        buttons |= WHEEL_BUTTON_DPAD_UP & WHEEL_BUTTON_DPAD_LEFT;
        break;
    }

    return buttons;
}

void PopulateStatus(WheelStatus& status, unsigned char* buffer)
{
    memcpy(&status.buttons, buffer + 1, 2);
    memcpy(&status.wheelRotation, buffer + 4, 2);
    status.gasPedal = buffer[6];
    status.brakePedal = buffer[7];
    status.clutchPedal = buffer[8];

    status.buttons = FixDPad(status.buttons);
}

int main()
{
    try
    {
        std::ifstream settings("settings.txt");
        std::string line;
        std::string key;
        std::string val;
        std::stringstream stream;
        while (std::getline(settings, line))
        {
            stream.str(line);
            std::getline(stream, key, '=');
            std::getline(stream, val);
            if (key == "steerMax")
                g_steerMax = std::stoi(val);
            else if (key == "deadZone")
                g_deadZone = std::stoi(val);
            else if (key == "gasThreshold")
                g_gasTheshold = std::stoi(val);-
            else if (key == "brakeThreshold")
                g_brakeTheshold = std::stoi(val);
        }

    }
    catch (std::exception e)
    {
        std::cout << "Could not open settings file" << std::endl;
    }



    //INITIALIZE HIDAPI
	hid_init();
    hid_device* handle = hid_open(0x046D, 0xC262, NULL);
    if (handle == NULL)
    {
        std::cout << "Unable to connect to device" << std::endl;
        return 1;
    }

    //INITIALIZE ViGEm
    PVIGEM_CLIENT client = vigem_alloc();
    VIGEM_ERROR error = vigem_connect(client);
    if (error != VIGEM_ERROR_NONE)
    {
        return 1;
    }

    PVIGEM_TARGET xbox360 = vigem_target_x360_alloc();
    error = vigem_target_add(client, xbox360);
    if (error != VIGEM_ERROR_NONE)
    {
        return 1;
    }


    //UPDATE LOOP
    unsigned char buffer[BUFFER_SIZE];
    uint16_t last = 0;

    uint16_t lowerSteerBound = 32768 - g_steerMax;
    uint16_t upperSteerBound = 32768 + g_steerMax;

    while (true)
    {
        int res = hid_read(handle, buffer, sizeof(buffer));

        if (res < 0)
        {
            std::cout << "Unable to read()" << std::endl;
            continue;
        }

        WheelStatus status{};
        PopulateStatus(status, buffer);



        XUSB_REPORT report;
        XUSB_REPORT_INIT(&report);

        if (status.buttons & WHEEL_BUTTON_A)
            report.wButtons |= XUSB_GAMEPAD_A;
        if (status.buttons & WHEEL_BUTTON_B)
            report.wButtons |= XUSB_GAMEPAD_B;
        if (status.buttons & WHEEL_BUTTON_X)
            report.wButtons |= XUSB_GAMEPAD_X;
        if (status.buttons & WHEEL_BUTTON_Y)
            report.wButtons |= XUSB_GAMEPAD_Y;
        if (status.buttons & WHEEL_BUTTON_START)
            report.wButtons |= XUSB_GAMEPAD_START;
        if (status.buttons & WHEEL_BUTTON_GUIDE)
            report.wButtons |= XUSB_GAMEPAD_GUIDE;
        if (status.buttons & WHEEL_BUTTON_DPAD_UP)
            report.wButtons |= XUSB_GAMEPAD_DPAD_UP;
        if (status.buttons & WHEEL_BUTTON_DPAD_DOWN)
            report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
        if (status.buttons & WHEEL_BUTTON_DPAD_LEFT)
            report.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
        if (status.buttons & WHEEL_BUTTON_DPAD_RIGHT)
            report.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;

        report.bLeftTrigger = (status.buttons & WHEEL_BUTTON_L_TRIG) ? 255 : 0;
        report.bRightTrigger = (status.buttons & WHEEL_BUTTON_R_TRIG) ? 255 : 0;

        if (status.gasPedal < 255 - g_gasTheshold)
            report.wButtons |= XUSB_GAMEPAD_B;
        if (status.brakePedal < 255 - g_brakeTheshold)
            report.wButtons |= XUSB_GAMEPAD_A;

        float halfDiff = (upperSteerBound - lowerSteerBound) * 0.5f;
        float axis = ((status.wheelRotation - lowerSteerBound) - halfDiff) / halfDiff;
        axis = Clamp(axis, -1.f, 1.f);

        report.sThumbLX = axis * SHRT_MAX;
        if (report.sThumbLX > -g_deadZone && report.sThumbLX < g_deadZone)
            report.sThumbLX = 0;

        vigem_target_x360_update(client, xbox360, report);
    }

    vigem_target_remove(client, xbox360);
    vigem_target_free(xbox360);
    vigem_free(client);

    hid_close(handle);
    hid_exit();

	return 0;
}