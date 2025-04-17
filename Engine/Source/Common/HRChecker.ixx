export module Common:HRChecker;
import <intsafe.h>;
import <source_location>;

// Thanks to ChiliTomatoNoodle for this great code we retooled to work with modules!
// https://github.com/planetchili/d3d12-shallow

struct CheckerToken
{};

// Use this to check for HRESULT errors in DX11 functions
export inline CheckerToken chk;

export struct HrGrabber
{
    HrGrabber(HRESULT hr, std::source_location = std::source_location::current()) noexcept;
    HRESULT hr;
    std::source_location loc;
};

export void operator>>(const HrGrabber&, CheckerToken);
export void operator<<(CheckerToken, const HrGrabber&);
