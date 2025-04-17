export module ResourceBrowser;
import Common;
import <set>;

namespace fs = std::filesystem;

static constexpr StaticString ResourceDirName = "Resource"_ss;

export
{
    struct ResourceBrowser
    {
        static fs::path FormatPath(const fs::path& path);
        static Array<fs::path> GetFilesWithExtension(std::set<std::string> extensions);

        static auto GetResourceDir() {return ProjectSettings::Get().projectRoot / ResourceDirName.data(); }
#ifdef DEBUG
        static auto GetEngineResourceDir() {return ProjectSettings::Get().projectRoot / ResourceDirName.data(); }
#endif
    };
}