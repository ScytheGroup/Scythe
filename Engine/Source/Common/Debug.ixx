export module Common:Debug;

import <iostream>;
import <source_location>;
import <sstream>;
import <winstring.h>;
import <print>;

export template<class ...T>
void DebugPrint(std::format_string<T...> formatString, T&&... args)
{
    OutputDebugStringA(std::format(formatString, std::forward<T>(args)...).append("\n").c_str());
    // There is a rare crash here inside println when on windows console subsystem!
    if (!IsDebuggerPresent())
    {
        std::string formattedString = std::format(formatString, std::forward<T>(args)...);
        std::cout << formattedString << std::endl;
    }
}

export template<class ...T>
int DebugPrintWindow(std::format_string<T...> formatString, T&&... args)
{
    const auto message = std::format(formatString, std::forward<T>(args)...).append("\n");
    OutputDebugStringA(message.c_str());
    return MessageBoxA(nullptr, message.c_str(), NULL, MB_ICONINFORMATION | MB_OKCANCEL);
}

export template <class... T>
void DebugPrintError(std::format_string<T...> formatString, T&&... args)
{
    const auto message = std::format(formatString, std::forward<T>(args)...).append("\n");
    OutputDebugStringA(message.c_str());
    std::cout << message << std::endl;
    
    // open the windows error window
    auto result = MessageBoxA(nullptr, message.c_str(), NULL, MB_ICONERROR | MB_ABORTRETRYIGNORE);
    if (result == IDRETRY && IsDebuggerPresent())
    {
        DebugBreak();
    }
    
    std::exit(1);
}

export inline std::string WStringToString(const std::wstring& wstr)
{
    std::string s;
    s.resize(wstr.size());
    int usedDefaultChar = 0;
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), static_cast<int>(wstr.size()), s.data(), static_cast<int>(s.size()), "#", &usedDefaultChar);
    return s;
}

export inline std::wstring StringToWString(const std::string& str)
{
    std::wstring s;
    s.resize(str.size());
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), s.data(), static_cast<int>(s.size()));
    return s;
}

export template <class... T>
void Assert(bool condition, std::format_string<T...> formatString = "", T&&... args, std::source_location loc = std::source_location::current())
{
    if (!condition)
    {
        const auto message = std::format(formatString, std::forward<T>(args)...);
        const std::string assert = std::format("{}:{} Assertion failed : ", loc.file_name(), loc.line()).c_str();
        OutputDebugStringA(assert.c_str());
        std::cout << (assert) << std::endl;
        std::string assertionMessage = assert + message;

        // open the windows error window
        auto result = MessageBoxA(nullptr, assertionMessage.c_str(), NULL, MB_ICONERROR | MB_ABORTRETRYIGNORE);
        if (result == IDRETRY && IsDebuggerPresent())
        {
            DebugBreak();
        }
    
        std::exit(1);
    }
}