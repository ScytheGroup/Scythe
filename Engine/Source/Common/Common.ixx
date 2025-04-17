export module Common;

export import :HRChecker;
export import :Debug;
export import :ViewSpan;
export import :ProjectSettings;
export import :Math;
export import :SH;
export import :Transform;
export import :PlatformHelpers;
export import :StaticString;
export import :Array;
export import :Function;
export import :Singleton;
export import :Color;
export import :Handle;
export import :Guid;
export import :TypeFormats;
export import :FileBrowser;
export import :Spinlock;
export import :Delegate;
export import :ArchiveStream;
export import StaticIndexedArray;
export import :RefCounted;

// STD
export import <iostream>;
export import <cstdint>;
export import <memory>;
export import <map>;
export import <variant>;
export import <format>;
export import <future>;
export import <chrono>;
export import <utility>;
export import <format>;
export import <array>;
export import <expected>;
export import <vector>;
export import <numeric>;
export import <unordered_map>;
export import <unordered_set>;
export import <algorithm>;
export import <ranges>;
export import <type_traits>;
export import <concepts>;
export import <filesystem>;
export import <thread>;
export import <source_location>;
export import <numbers>;
export import <queue>;
export import <regex>;
export import <fstream>;
export import <string>;

export using namespace std::literals;

// WinAPI
export import <winstring.h>;
export import <wrl.h>;
export template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// Remove warning for structures with padding (which we need because of how DX11 expects to receive data)
#pragma warning(disable : 4324)

// DX11
export import <d3d11.h>;
export import <dxgi1_6.h>;

// Inputs
export import "Keyboard.h";
export import "Mouse.h";

// Libs
export import <magic_enum/magic_enum.hpp>;

export template<class T>
concept IsImGuiDrawable = requires(T t) {
    { t.ImGuiDraw() } ->  std::convertible_to<bool>;
};

export std::filesystem::path operator""_p(const char* c, size_t length)
{
    return std::filesystem::path{ c };
}
