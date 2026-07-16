#ifdef __MINGW32__
    #include "SDL2/SDL.h"
#endif

#ifdef __linux__
    #include <SDL2/SDL.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

enum class Binding_Type
{
    Key,
    Mouse,
    Controller_Button,
    Controller_Axis
};

struct Binding
{
    Binding_Type Type = Binding_Type::Key;
    std::string Code;
};

struct Control_Set
{
    std::map<std::string, Binding> Bindings;
};

struct Axis_Range_Function
{
    int Min = 0;
    int Max = 0;
    std::string Function;
};

struct Controller_State
{
    SDL_GameController *Controller = nullptr;
    SDL_JoystickID Instance_ID = -1;
    std::map<SDL_GameControllerButton, bool> Button_Now;
    std::map<SDL_GameControllerButton, bool> Button_Before;
    std::map<SDL_GameControllerAxis, int> Axis_Now;
    std::map<SDL_GameControllerAxis, int> Axis_Before;
};

std::string Resources = "./Resources";
std::string Input_Path = "./Resources/Input/";
std::string Active_Control_Set = "Default";

std::map<std::string, Control_Set> Control_Sets;
std::map<SDL_Scancode, bool> Key_Now;
std::map<SDL_Scancode, bool> Key_Before;
std::map<Uint8, bool> Mouse_Now;
std::map<Uint8, bool> Mouse_Before;

std::map<std::string, std::string> Key_Down_Functions;
std::map<std::string, std::string> Key_Up_Functions;
std::map<std::string, std::string> Mouse_Down_Functions;
std::map<std::string, std::string> Mouse_Up_Functions;
std::map<std::string, std::string> Controller_Button_Down_Functions;
std::map<std::string, std::string> Controller_Button_Up_Functions;
std::map<std::string, std::string> Controller_Axis_Functions;
std::map<int, std::map<std::string, std::string>> Controller_Button_Down_For;
std::map<int, std::map<std::string, std::string>> Controller_Button_Up_For;
std::map<int, std::map<std::string, std::string>> Controller_Axis_For;
std::map<int, std::map<std::string, std::vector<Axis_Range_Function>>> Controller_Axis_Ranges;
std::map<int, Controller_State> Controllers;
std::set<int> Wanted_Controllers;
std::vector<std::string> Triggered_Functions;
void (*Run_Function_Callback)(std::string Function) = nullptr;
void (*Run_Analog_Function_Callback)(std::string Function, int Value) = nullptr;
const int Controller_Deadzone = 8000;

int Mouse_X_Value = 0;
int Mouse_Y_Value = 0;
int Mouse_Delta_X_Value = 0;
int Mouse_Delta_Y_Value = 0;
int Mouse_Wheel_Value = 0;
bool SDL_Ready = false;
bool Windows_Controller_Shutdown_Safe = true;
bool DEBUG = false;
bool Auto_Open_Controllers = false;

extern "C" void Open_Controller(int Controller);

std::string Normalize(std::string Text)
{
    std::replace(Text.begin(), Text.end(), '-', '_');
    return Text;
}

std::string Strip_Prefix(std::string Text, std::string Prefix)
{
    if(Text.rfind(Prefix, 0) == 0)
    {
        return Text.substr(Prefix.length());
    }
    return Text;
}

SDL_Scancode Key_Code(std::string Key)
{
    Key = Normalize(Key);
    Key = Strip_Prefix(Key, "Key_");
    Key = Strip_Prefix(Key, "KEY_");

    if(Key == "Space") Key = "Space";
    else if(Key == "Escape" || Key == "Esc") Key = "Escape";
    else if(Key == "Left") Key = "Left";
    else if(Key == "Right") Key = "Right";
    else if(Key == "Up") Key = "Up";
    else if(Key == "Down") Key = "Down";

    SDL_Scancode Code = SDL_GetScancodeFromName(Key.c_str());
    if(Code == SDL_SCANCODE_UNKNOWN && Key.length() == 1)
    {
        std::string Upper = Key;
        std::transform(Upper.begin(), Upper.end(), Upper.begin(), ::toupper);
        Code = SDL_GetScancodeFromName(Upper.c_str());
    }
    return Code;
}

Uint8 Mouse_Code(std::string Button)
{
    Button = Normalize(Button);
    Button = Strip_Prefix(Button, "Mouse_");
    Button = Strip_Prefix(Button, "MOUSE_");

    if(Button == "Left" || Button == "Button_Left") return SDL_BUTTON_LEFT;
    if(Button == "Middle" || Button == "Button_Middle") return SDL_BUTTON_MIDDLE;
    if(Button == "Right" || Button == "Button_Right") return SDL_BUTTON_RIGHT;
    if(Button == "X1" || Button == "Button_X1") return SDL_BUTTON_X1;
    if(Button == "X2" || Button == "Button_X2") return SDL_BUTTON_X2;
    return SDL_BUTTON_LEFT;
}

SDL_GameControllerButton Controller_Button_Code(std::string Button)
{
    Button = Normalize(Button);
    Button = Strip_Prefix(Button, "Button_");
    Button = Strip_Prefix(Button, "BUTTON_");

    if(Button == "A") return SDL_CONTROLLER_BUTTON_A;
    if(Button == "B") return SDL_CONTROLLER_BUTTON_B;
    if(Button == "X") return SDL_CONTROLLER_BUTTON_X;
    if(Button == "Y") return SDL_CONTROLLER_BUTTON_Y;
    if(Button == "Back") return SDL_CONTROLLER_BUTTON_BACK;
    if(Button == "Guide") return SDL_CONTROLLER_BUTTON_GUIDE;
    if(Button == "Start") return SDL_CONTROLLER_BUTTON_START;
    if(Button == "Left_Stick") return SDL_CONTROLLER_BUTTON_LEFTSTICK;
    if(Button == "Right_Stick") return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    if(Button == "Left_Shoulder") return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    if(Button == "Right_Shoulder") return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    if(Button == "DPad_Up" || Button == "Dpad_Up") return SDL_CONTROLLER_BUTTON_DPAD_UP;
    if(Button == "DPad_Down" || Button == "Dpad_Down") return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    if(Button == "DPad_Left" || Button == "Dpad_Left") return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    if(Button == "DPad_Right" || Button == "Dpad_Right") return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    return SDL_CONTROLLER_BUTTON_INVALID;
}

SDL_GameControllerAxis Controller_Axis_Code(std::string Axis)
{
    Axis = Normalize(Axis);

    if(Axis == "Left_Stick_X" || Axis == "Left_X") return SDL_CONTROLLER_AXIS_LEFTX;
    if(Axis == "Left_Stick_Y" || Axis == "Left_Y") return SDL_CONTROLLER_AXIS_LEFTY;
    if(Axis == "Right_Stick_X" || Axis == "Right_X") return SDL_CONTROLLER_AXIS_RIGHTX;
    if(Axis == "Right_Stick_Y" || Axis == "Right_Y") return SDL_CONTROLLER_AXIS_RIGHTY;
    if(Axis == "Left_Trigger" || Axis == "Trigger_Left") return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    if(Axis == "Right_Trigger" || Axis == "Trigger_Right") return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    return SDL_CONTROLLER_AXIS_INVALID;
}

bool Ensure_SDL()
{
    if(SDL_Ready)
    {
        return true;
    }

    if(SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0)
    {
        if(DEBUG) std::cout << "SDL input could not initialize: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_Ready = true;
    return true;
}

Control_Set &Current_Set()
{
    if(Control_Sets.find(Active_Control_Set) == Control_Sets.end())
    {
        Control_Sets[Active_Control_Set] = Control_Set();
    }
    return Control_Sets[Active_Control_Set];
}

void Trigger_Function(std::string Function)
{
    if(Function == "") return;

    Triggered_Functions.push_back(Function);
    if(Run_Function_Callback != nullptr)
    {
        Run_Function_Callback(Function);
    }
    if(DEBUG) std::cout << "Input Function: " << Function << std::endl;
}

void Trigger_Analog_Function(std::string Function, int Value)
{
    if(Function == "") return;

    Triggered_Functions.push_back(Function);
    if(Run_Analog_Function_Callback != nullptr)
    {
        Run_Analog_Function_Callback(Function, Value);
    }
    if(DEBUG) std::cout << "Input Analog Function: " << Function << "(" << Value << ")" << std::endl;
}

void Parse_Controller_Code(std::string Code, int &Controller, std::string &Name)
{
    Controller = 0;
    Name = Code;

    size_t Separator = Code.find(':');
    if(Separator == std::string::npos)
    {
        Separator = Code.find('/');
    }

    if(Separator != std::string::npos)
    {
        Controller = std::atoi(Code.substr(0, Separator).c_str());
        Name = Code.substr(Separator + 1);
    }
}

Controller_State *Get_Controller(int Controller)
{
    auto It = Controllers.find(Controller);
    if(It == Controllers.end() || It->second.Controller == nullptr)
    {
        return nullptr;
    }
    return &It->second;
}

bool Controller_Button_Down_State(int Controller, std::string Button)
{
    Controller_State *State = Get_Controller(Controller);
    if(State == nullptr) return false;

    SDL_GameControllerButton Code = Controller_Button_Code(Button);
    if(Code == SDL_CONTROLLER_BUTTON_INVALID) return false;
    return State->Button_Now[Code];
}

bool Controller_Button_Pressed_State(int Controller, std::string Button)
{
    Controller_State *State = Get_Controller(Controller);
    if(State == nullptr) return false;

    SDL_GameControllerButton Code = Controller_Button_Code(Button);
    if(Code == SDL_CONTROLLER_BUTTON_INVALID) return false;
    return State->Button_Now[Code] && !State->Button_Before[Code];
}

bool Controller_Button_Released_State(int Controller, std::string Button)
{
    Controller_State *State = Get_Controller(Controller);
    if(State == nullptr) return false;

    SDL_GameControllerButton Code = Controller_Button_Code(Button);
    if(Code == SDL_CONTROLLER_BUTTON_INVALID) return false;
    return !State->Button_Now[Code] && State->Button_Before[Code];
}

int Controller_Axis_State(int Controller, std::string Axis)
{
    Controller_State *State = Get_Controller(Controller);
    if(State == nullptr) return 0;

    SDL_GameControllerAxis Code = Controller_Axis_Code(Axis);
    if(Code == SDL_CONTROLLER_AXIS_INVALID) return 0;
    return State->Axis_Now[Code];
}

bool Axis_In_Range(int Value, int Min, int Max)
{
    if(Min > Max)
    {
        std::swap(Min, Max);
    }
    return Value >= Min && Value <= Max;
}

std::vector<SDL_GameControllerButton> Controller_Buttons()
{
    return {
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_GUIDE,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_LEFTSTICK,
        SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_DPAD_UP,
        SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        SDL_CONTROLLER_BUTTON_DPAD_RIGHT
    };
}

std::vector<SDL_GameControllerAxis> Controller_Axes()
{
    return {
        SDL_CONTROLLER_AXIS_LEFTX,
        SDL_CONTROLLER_AXIS_LEFTY,
        SDL_CONTROLLER_AXIS_RIGHTX,
        SDL_CONTROLLER_AXIS_RIGHTY,
        SDL_CONTROLLER_AXIS_TRIGGERLEFT,
        SDL_CONTROLLER_AXIS_TRIGGERRIGHT
    };
}

void Close_Controller_Handle(SDL_GameController *Controller)
{
    if(Controller == nullptr)
    {
        return;
    }

#ifdef __MINGW32__
    Windows_Controller_Shutdown_Safe = false;
    if(DEBUG) std::cout << "Input leaving SDL controller handle open for Windows shutdown safety" << std::endl;
#else
    SDL_GameControllerClose(Controller);
#endif
}

bool Key_Down_State(std::string Key)
{
    SDL_Scancode Code = Key_Code(Key);
    if(Code == SDL_SCANCODE_UNKNOWN) return false;
    return Key_Now[Code];
}

bool Key_Pressed_State(std::string Key)
{
    SDL_Scancode Code = Key_Code(Key);
    if(Code == SDL_SCANCODE_UNKNOWN) return false;
    return Key_Now[Code] && !Key_Before[Code];
}

bool Key_Released_State(std::string Key)
{
    SDL_Scancode Code = Key_Code(Key);
    if(Code == SDL_SCANCODE_UNKNOWN) return false;
    return !Key_Now[Code] && Key_Before[Code];
}

bool Mouse_Down_State(std::string Button)
{
    Uint8 Code = Mouse_Code(Button);
    return Mouse_Now[Code];
}

bool Mouse_Pressed_State(std::string Button)
{
    Uint8 Code = Mouse_Code(Button);
    return Mouse_Now[Code] && !Mouse_Before[Code];
}

bool Mouse_Released_State(std::string Button)
{
    Uint8 Code = Mouse_Code(Button);
    return !Mouse_Now[Code] && Mouse_Before[Code];
}

extern "C" void Constructor()
{
    Ensure_SDL();
    Control_Sets["Default"] = Control_Set();
}

extern "C" void Destructor()
{
    Control_Sets.clear();
    Key_Now.clear();
    Key_Before.clear();
    Mouse_Now.clear();
    Mouse_Before.clear();
    Triggered_Functions.clear();
    Wanted_Controllers.clear();
    Auto_Open_Controllers = false;
    for(auto &Item : Controllers)
    {
        if(Item.second.Controller != nullptr)
        {
            Close_Controller_Handle(Item.second.Controller);
            Item.second.Controller = nullptr;
        }
    }
    Controllers.clear();

    if(SDL_Ready)
    {
#ifdef __MINGW32__
        if(!Windows_Controller_Shutdown_Safe)
        {
            if(DEBUG) std::cout << "Input skipping SDL controller subsystem quit for Windows shutdown safety" << std::endl;
            SDL_Ready = false;
            return;
        }
#endif
        SDL_QuitSubSystem(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER);
        SDL_Ready = false;
    }
}

extern "C" void Set_Paths(std::string Path_of_Resources)
{
    Resources = Path_of_Resources;
    Input_Path = Resources + "/Input/";
}

extern "C" void Add_Control_Set(std::string Name)
{
    Control_Sets[Name] = Control_Set();
}

extern "C" void Set_Control_Set(std::string Name)
{
    if(Control_Sets.find(Name) == Control_Sets.end())
    {
        Add_Control_Set(Name);
    }
    Active_Control_Set = Name;
}

extern "C" void Update(std::string Control_Set_Name)
{
    if(!Ensure_SDL()) return;

    if(Control_Set_Name != "")
    {
        Set_Control_Set(Control_Set_Name);
    }

    Key_Before = Key_Now;
    Mouse_Before = Mouse_Now;
    for(auto &Item : Controllers)
    {
        Item.second.Button_Before = Item.second.Button_Now;
        Item.second.Axis_Before = Item.second.Axis_Now;
    }

    Mouse_Wheel_Value = 0;
    Triggered_Functions.clear();

    SDL_Event Event;
    std::vector<SDL_Event> Unhandled_Events;
    while(SDL_PollEvent(&Event))
    {
        if(Event.type == SDL_MOUSEWHEEL)
        {
            Mouse_Wheel_Value += Event.wheel.y;
        }
        else if(Event.type == SDL_CONTROLLERDEVICEADDED)
        {
            int Controller = Event.cdevice.which;
            if(Controllers.find(Controller) != Controllers.end())
            {
                continue;
            }
            if(Auto_Open_Controllers || Wanted_Controllers.find(Controller) != Wanted_Controllers.end())
            {
                Open_Controller(Controller);
            }
        }
        else if(Event.type == SDL_CONTROLLERDEVICEREMOVED)
        {
            for(auto It = Controllers.begin(); It != Controllers.end();)
            {
                if(It->second.Instance_ID == Event.cdevice.which)
                {
                    Close_Controller_Handle(It->second.Controller);
                    It = Controllers.erase(It);
                }
                else
                {
                    ++It;
                }
            }
        }
        else
        {
            Unhandled_Events.push_back(Event);
        }
    }

    for(const SDL_Event &Unhandled_Event : Unhandled_Events)
    {
        SDL_Event Event_Copy = Unhandled_Event;
        SDL_PushEvent(&Event_Copy);
    }

    int New_Mouse_X = 0;
    int New_Mouse_Y = 0;
    Uint32 Mouse_State = SDL_GetMouseState(&New_Mouse_X, &New_Mouse_Y);
    Mouse_Delta_X_Value = New_Mouse_X - Mouse_X_Value;
    Mouse_Delta_Y_Value = New_Mouse_Y - Mouse_Y_Value;
    Mouse_X_Value = New_Mouse_X;
    Mouse_Y_Value = New_Mouse_Y;

    for(Uint8 Button : {SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE, SDL_BUTTON_RIGHT, SDL_BUTTON_X1, SDL_BUTTON_X2})
    {
        Mouse_Now[Button] = (Mouse_State & SDL_BUTTON(Button)) != 0;
    }

    SDL_PumpEvents();
    const Uint8 *Keyboard = SDL_GetKeyboardState(nullptr);
    std::set<SDL_Scancode> Keys_To_Check;
    for(const auto &Item : Current_Set().Bindings)
    {
        if(Item.second.Type == Binding_Type::Key)
        {
            SDL_Scancode Code = Key_Code(Item.second.Code);
            if(Code != SDL_SCANCODE_UNKNOWN) Keys_To_Check.insert(Code);
        }
    }
    for(const auto &Item : Key_Down_Functions)
    {
        SDL_Scancode Code = Key_Code(Item.first);
        if(Code != SDL_SCANCODE_UNKNOWN) Keys_To_Check.insert(Code);
    }
    for(const auto &Item : Key_Up_Functions)
    {
        SDL_Scancode Code = Key_Code(Item.first);
        if(Code != SDL_SCANCODE_UNKNOWN) Keys_To_Check.insert(Code);
    }

    for(SDL_Scancode Code : Keys_To_Check)
    {
        Key_Now[Code] = Keyboard[Code] != 0;
    }

    for(const auto &Item : Key_Down_Functions)
    {
        if(Key_Pressed_State(Item.first)) Trigger_Function(Item.second);
    }
    for(const auto &Item : Key_Up_Functions)
    {
        if(Key_Released_State(Item.first)) Trigger_Function(Item.second);
    }
    for(const auto &Item : Mouse_Down_Functions)
    {
        if(Mouse_Pressed_State(Item.first)) Trigger_Function(Item.second);
    }
    for(const auto &Item : Mouse_Up_Functions)
    {
        if(Mouse_Released_State(Item.first)) Trigger_Function(Item.second);
    }

    for(auto &Controller_Item : Controllers)
    {
        int Controller_Index = Controller_Item.first;
        Controller_State &State = Controller_Item.second;
        if(State.Controller == nullptr)
        {
            continue;
        }

        for(SDL_GameControllerButton Button : Controller_Buttons())
        {
            State.Button_Now[Button] = SDL_GameControllerGetButton(State.Controller, Button) != 0;
        }

        for(SDL_GameControllerAxis Axis : Controller_Axes())
        {
            State.Axis_Now[Axis] = SDL_GameControllerGetAxis(State.Controller, Axis);
        }

        for(const auto &Item : Controller_Button_Down_Functions)
        {
            if(Controller_Button_Pressed_State(Controller_Index, Item.first)) Trigger_Function(Item.second);
        }
        for(const auto &Item : Controller_Button_Up_Functions)
        {
            if(Controller_Button_Released_State(Controller_Index, Item.first)) Trigger_Function(Item.second);
        }
        for(const auto &Item : Controller_Button_Down_For[Controller_Index])
        {
            if(Controller_Button_Pressed_State(Controller_Index, Item.first)) Trigger_Function(Item.second);
        }
        for(const auto &Item : Controller_Button_Up_For[Controller_Index])
        {
            if(Controller_Button_Released_State(Controller_Index, Item.first)) Trigger_Function(Item.second);
        }

        for(const auto &Item : Controller_Axis_Functions)
        {
            SDL_GameControllerAxis Axis_Code = Controller_Axis_Code(Item.first);
            if(Axis_Code == SDL_CONTROLLER_AXIS_INVALID) continue;

            int Value = State.Axis_Now[Axis_Code];
            int Before = State.Axis_Before[Axis_Code];
            if(std::abs(Value) > Controller_Deadzone || (Value != Before && std::abs(Before) > Controller_Deadzone))
            {
                Trigger_Analog_Function(Item.second, Value);
            }
        }
        for(const auto &Item : Controller_Axis_For[Controller_Index])
        {
            SDL_GameControllerAxis Axis_Code = Controller_Axis_Code(Item.first);
            if(Axis_Code == SDL_CONTROLLER_AXIS_INVALID) continue;

            int Value = State.Axis_Now[Axis_Code];
            int Before = State.Axis_Before[Axis_Code];
            if(std::abs(Value) > Controller_Deadzone || (Value != Before && std::abs(Before) > Controller_Deadzone))
            {
                Trigger_Analog_Function(Item.second, Value);
            }
        }
        for(const auto &Axis_Item : Controller_Axis_Ranges[Controller_Index])
        {
            SDL_GameControllerAxis Axis_Code = Controller_Axis_Code(Axis_Item.first);
            if(Axis_Code == SDL_CONTROLLER_AXIS_INVALID) continue;

            int Value = State.Axis_Now[Axis_Code];
            int Before = State.Axis_Before[Axis_Code];
            for(const Axis_Range_Function &Range : Axis_Item.second)
            {
                if(Axis_In_Range(Value, Range.Min, Range.Max) && !Axis_In_Range(Before, Range.Min, Range.Max))
                {
                    Trigger_Analog_Function(Range.Function, Value);
                }
            }
        }
    }
}

extern "C" void Bind_Key(std::string Action, std::string Key)
{
    Current_Set().Bindings[Action] = {Binding_Type::Key, Key};
}

extern "C" void Bind_Mouse(std::string Action, std::string Button)
{
    Current_Set().Bindings[Action] = {Binding_Type::Mouse, Button};
}

extern "C" void Bind_Button(std::string Action, std::string Button)
{
    Current_Set().Bindings[Action] = {Binding_Type::Controller_Button, Button};
}

extern "C" void Bind_Axis(std::string Action, std::string Axis)
{
    Current_Set().Bindings[Action] = {Binding_Type::Controller_Axis, Axis};
}

extern "C" void Unbind(std::string Action)
{
    Current_Set().Bindings.erase(Action);
}

extern "C" bool Down(std::string Action)
{
    auto It = Current_Set().Bindings.find(Action);
    if(It == Current_Set().Bindings.end()) return false;

    if(It->second.Type == Binding_Type::Key) return Key_Down_State(It->second.Code);
    if(It->second.Type == Binding_Type::Mouse) return Mouse_Down_State(It->second.Code);
    if(It->second.Type == Binding_Type::Controller_Button)
    {
        int Controller;
        std::string Button;
        Parse_Controller_Code(It->second.Code, Controller, Button);
        return Controller_Button_Down_State(Controller, Button);
    }
    return false;
}

extern "C" bool Pressed(std::string Action)
{
    auto It = Current_Set().Bindings.find(Action);
    if(It == Current_Set().Bindings.end()) return false;

    if(It->second.Type == Binding_Type::Key) return Key_Pressed_State(It->second.Code);
    if(It->second.Type == Binding_Type::Mouse) return Mouse_Pressed_State(It->second.Code);
    if(It->second.Type == Binding_Type::Controller_Button)
    {
        int Controller;
        std::string Button;
        Parse_Controller_Code(It->second.Code, Controller, Button);
        return Controller_Button_Pressed_State(Controller, Button);
    }
    return false;
}

extern "C" bool Released(std::string Action)
{
    auto It = Current_Set().Bindings.find(Action);
    if(It == Current_Set().Bindings.end()) return false;

    if(It->second.Type == Binding_Type::Key) return Key_Released_State(It->second.Code);
    if(It->second.Type == Binding_Type::Mouse) return Mouse_Released_State(It->second.Code);
    if(It->second.Type == Binding_Type::Controller_Button)
    {
        int Controller;
        std::string Button;
        Parse_Controller_Code(It->second.Code, Controller, Button);
        return Controller_Button_Released_State(Controller, Button);
    }
    return false;
}

extern "C" float Axis(std::string Action)
{
    if(Action == "Mouse_X") return static_cast<float>(Mouse_X_Value);
    if(Action == "Mouse_Y") return static_cast<float>(Mouse_Y_Value);
    if(Action == "Mouse_Delta_X") return static_cast<float>(Mouse_Delta_X_Value);
    if(Action == "Mouse_Delta_Y") return static_cast<float>(Mouse_Delta_Y_Value);
    if(Action == "Mouse_Wheel") return static_cast<float>(Mouse_Wheel_Value);

    auto It = Current_Set().Bindings.find(Action);
    if(It != Current_Set().Bindings.end() && It->second.Type == Binding_Type::Controller_Axis)
    {
        int Controller;
        std::string Axis_Name;
        Parse_Controller_Code(It->second.Code, Controller, Axis_Name);
        return static_cast<float>(Controller_Axis_State(Controller, Axis_Name));
    }

    return 0.0f;
}

extern "C" int Mouse_X()
{
    return Mouse_X_Value;
}

extern "C" int Mouse_Y()
{
    return Mouse_Y_Value;
}

extern "C" int Mouse_Delta_X()
{
    return Mouse_Delta_X_Value;
}

extern "C" int Mouse_Delta_Y()
{
    return Mouse_Delta_Y_Value;
}

extern "C" int Mouse_Wheel()
{
    return Mouse_Wheel_Value;
}

extern "C" bool Mouse_Down(std::string Button)
{
    return Mouse_Down_State(Button);
}

extern "C" bool Mouse_Pressed(std::string Button)
{
    return Mouse_Pressed_State(Button);
}

extern "C" bool Mouse_Released(std::string Button)
{
    return Mouse_Released_State(Button);
}

extern "C" void Set_Key_Down(std::string Key, std::string Function)
{
    Key_Down_Functions[Key] = Function;
}

extern "C" void Set_Key_Up(std::string Key, std::string Function)
{
    Key_Up_Functions[Key] = Function;
}

extern "C" void Set_Mouse_Down(std::string Button, std::string Function)
{
    Mouse_Down_Functions[Button] = Function;
}

extern "C" void Set_Mouse_Up(std::string Button, std::string Function)
{
    Mouse_Up_Functions[Button] = Function;
}

extern "C" void Set_Controller_Button_Down(std::string Button, std::string Function)
{
    Controller_Button_Down_Functions[Button] = Function;
}

extern "C" void Set_Controller_Button_Up(std::string Button, std::string Function)
{
    Controller_Button_Up_Functions[Button] = Function;
}

extern "C" void Set_Controller_Axis(std::string Axis, std::string Function)
{
    Controller_Axis_Functions[Axis] = Function;
}

extern "C" void Open_Controller(int Controller)
{
    if(!Ensure_SDL()) return;

    if(Controller < 0)
    {
        Auto_Open_Controllers = true;
        int Count = SDL_NumJoysticks();
        for(int Index = 0; Index < Count; ++Index)
        {
            Open_Controller(Index);
        }
        return;
    }

    Wanted_Controllers.insert(Controller);

    if(Controllers.find(Controller) != Controllers.end() && Controllers[Controller].Controller != nullptr)
    {
        return;
    }

    if(Controller >= SDL_NumJoysticks() || !SDL_IsGameController(Controller))
    {
        if(DEBUG) std::cout << "Input controller " << Controller << " is not available as an SDL game controller" << std::endl;
        return;
    }

    SDL_GameController *Game_Controller = SDL_GameControllerOpen(Controller);
    if(Game_Controller == nullptr)
    {
        if(DEBUG) std::cout << "Input controller " << Controller << " could not open: " << SDL_GetError() << std::endl;
        return;
    }

#ifdef __MINGW32__
    Windows_Controller_Shutdown_Safe = false;
#endif

    Controller_State State;
    State.Controller = Game_Controller;

    SDL_Joystick *Joystick = SDL_GameControllerGetJoystick(Game_Controller);
    if(Joystick != nullptr)
    {
        State.Instance_ID = SDL_JoystickInstanceID(Joystick);
    }

    for(SDL_GameControllerButton Button : Controller_Buttons())
    {
        State.Button_Now[Button] = SDL_GameControllerGetButton(Game_Controller, Button) != 0;
        State.Button_Before[Button] = State.Button_Now[Button];
    }

    for(SDL_GameControllerAxis Axis : Controller_Axes())
    {
        State.Axis_Now[Axis] = SDL_GameControllerGetAxis(Game_Controller, Axis);
        State.Axis_Before[Axis] = State.Axis_Now[Axis];
    }

    Controllers[Controller] = State;
    if(DEBUG) std::cout << "Input opened controller " << Controller << ": " << SDL_GameControllerName(Game_Controller) << std::endl;
}

extern "C" void Close_Controller(int Controller)
{
    if(Controller < 0)
    {
        Auto_Open_Controllers = false;
        Wanted_Controllers.clear();
        for(auto &Item : Controllers)
        {
            if(Item.second.Controller != nullptr)
            {
                Close_Controller_Handle(Item.second.Controller);
                Item.second.Controller = nullptr;
            }
        }
        Controllers.clear();
        return;
    }

    Wanted_Controllers.erase(Controller);

    auto It = Controllers.find(Controller);
    if(It == Controllers.end())
    {
        return;
    }

    if(It->second.Controller != nullptr)
    {
        Close_Controller_Handle(It->second.Controller);
    }
    Controllers.erase(It);
}

extern "C" bool Controller_Down(int Controller, std::string Button)
{
    return Controller_Button_Down_State(Controller, Button);
}

extern "C" bool Controller_Pressed(int Controller, std::string Button)
{
    return Controller_Button_Pressed_State(Controller, Button);
}

extern "C" bool Controller_Released(int Controller, std::string Button)
{
    return Controller_Button_Released_State(Controller, Button);
}

extern "C" int Controller_Axis(int Controller, std::string Axis)
{
    return Controller_Axis_State(Controller, Axis);
}

extern "C" void Set_Controller_Button_Down_For(int Controller, std::string Button, std::string Function)
{
    Controller_Button_Down_For[Controller][Button] = Function;
}

extern "C" void Set_Controller_Button_Up_For(int Controller, std::string Button, std::string Function)
{
    Controller_Button_Up_For[Controller][Button] = Function;
}

extern "C" void Set_Controller_Axis_For(int Controller, std::string Axis, std::string Function)
{
    Controller_Axis_For[Controller][Axis] = Function;
}

extern "C" void Set_Controller_Axis_Range(int Controller, std::string Axis, int Min, int Max, std::string Function)
{
    Controller_Axis_Ranges[Controller][Axis].push_back({Min, Max, Function});
}

extern "C" void Set_Function_Runner(void (*Run_Function)(std::string), void (*Run_Analog_Function)(std::string, int))
{
    Run_Function_Callback = Run_Function;
    Run_Analog_Function_Callback = Run_Analog_Function;
}

extern "C" void Debug(bool Enable)
{
    DEBUG = Enable;
}
