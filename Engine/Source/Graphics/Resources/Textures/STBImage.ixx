export module Graphics:STBImage;

import Common;

export class STBImage
{
    void* data{};
    uint32_t width{}, height{}, nbComponents{}, componentByteSize{};
    bool isFloatingPointFormat;
public:
    static constexpr uint32_t ExpectedComponentCount = 4; // for RGBA
    
    STBImage(const std::filesystem::path& filepath);
    ~STBImage();

    STBImage(const STBImage&) = delete;
    STBImage& operator=(const STBImage&) = delete;

    STBImage(STBImage&&) noexcept;
    STBImage& operator=(STBImage&&);

    uint32_t GetByteSize() const noexcept { return height * GetRowByteSize(); }
    uint32_t GetWidth() const noexcept { return width; }
    uint32_t GetHeight() const noexcept { return height; }
    uint32_t GetPixelByteSize() const noexcept { return ExpectedComponentCount * componentByteSize; }
    uint32_t GetRowByteSize() const noexcept { return width * GetPixelByteSize(); }
    uint32_t GetComponentByteSize() const noexcept { return componentByteSize; }
    bool IsFloatingPointFormat() const noexcept { return isFloatingPointFormat; };
    void* Data() const noexcept { return data; }

protected:
    void Swap(STBImage& other);
};

