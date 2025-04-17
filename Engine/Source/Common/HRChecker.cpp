module;
#include <wrl.h>
module Common:HRChecker;

import :HRChecker;
import :PlatformHelpers;
import :Debug;
import <source_location>;
import <algorithm>;

HrGrabber::HrGrabber(HRESULT hr, std::source_location loc) noexcept
    : hr(hr)
    , loc(std::move(loc))
{}

void operator>>(const HrGrabber& grabber, CheckerToken)
{
    if (FAILED(grabber.hr))
    {
        DebugPrint("Graphics Error: {} {}({})", HRToError(grabber.hr), grabber.loc.file_name(), grabber.loc.line());
        DebugBreak();
        std::exit(1);
    }
}

void operator<<(CheckerToken checker, const HrGrabber& grabber)
{
    grabber >> checker;
}
