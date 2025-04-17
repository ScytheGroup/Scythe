export module Graphics:Texture;

import Common;

import :Device;
import :STBImage;
import :Interfaces;
import :LazyLoadedResource;
import Tools;

export template<class T> struct AssetRef;
export class EntityComponentSystem;

export class Texture : public ILazyLoadedResource, public IReadableResource, public IReadWriteResource
{
public:
    enum Type : uint8_t
    {
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBE
    };

    enum BindFlags : uint8_t
    {
        TEXTURE_RTV = 1 << 0,
        TEXTURE_SRV = 1 << 1,
        TEXTURE_UAV = 1 << 2,
    };

    enum Usage : uint8_t
    {
        DEFAULT = 0,    // Read and write on GPU
        // IMMUTABLE = 1,  // Read only on GPU, NOT USED
        // The GPU already does its job as well as it can so it is wasted effort to get this working, especially since having mipmapped immutable textures requires:
        //  - Creating another temp texture
        //  - Generating mip maps in the temp texture
        //  - Copying this to a staging texture
        //  - Downloading the texture data on CPU
        //  - Passing the downloaded data to our Immutable Texture as initialisation data
        // All this for the SAME performance.
        // You can probably understand why I gave up adding this halfway through.
        DYNAMIC = 2,    // Read only on GPU and write only on CPU
        STAGING = 3    // Transfer data from GPU to CPU
    };
    
    struct Description
    {
        std::string name{};
        uint32_t width = 0, height = 0, depth = 0;
        uint32_t arraySize = 1;
        DXGI_FORMAT format;
        Type type;
        Usage usage = DEFAULT;
        uint8_t bindFlags = 0;
        bool generateMips = false;
        UINT mips = 1;
        
        static Description CreateTextureCube(const std::string& name, uint32_t size, DXGI_FORMAT format, bool generateMips);
        static Description CreateTexture2D(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format, uint8_t bindFlags, bool generateMips);
        static Description CreateTexture3D(const std::string& name, uint32_t width, uint32_t height, uint32_t depth, DXGI_FORMAT format, uint8_t bindFlags, bool generateMips);
    };

    // This could be the only class to be serialized for later use
    struct TextureData
    {
        std::vector<char> data;

        Description description;
    
        TextureData() = default;
        TextureData(TextureData&&) = default;
        TextureData& operator=(TextureData&&) = default;
        TextureData(const TextureData&) = default;
        TextureData& operator=(const TextureData&) = default;

        TextureData(const std::string& name, const STBImage& image, Texture::Type type, Texture::Usage usage, uint8_t bindFlags, bool generateMips);
        TextureData(const std::filesystem::path& filename, Texture::Type type, Texture::Usage usage, uint8_t bindFlags, bool generateMips);

        template<class T>
        void FillWith(const T& value, int count);
    };
    
    // Creates empty texture with explicit params
    Texture(const Description& description, Device& device);
    Texture(const Description& description);

    // Creates texture with data
    Texture(const TextureData& textureData, Device& device);
    Texture(const TextureData& textureData);

    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;
    ~Texture() override {}

    const ComPtr<ID3D11RenderTargetView>& GetRTV() const { return rtv; }
    const ComPtr<ID3D11ShaderResourceView>& GetSRV() const override { return srv; }
    const ComPtr<ID3D11UnorderedAccessView>& GetUAV() const { return uav; }
    std::string_view GetName() const noexcept { return data.description.name; }
    ID3D11Resource* GetRawTexture() const noexcept { return rawTexture.Get(); }
    Type GetType() const noexcept { return data.description.type; }
    uint8_t GetBindFlags() const noexcept { return data.description.bindFlags; }
    uint8_t GetMipsCount() const noexcept { return data.description.mips; }
    Usage GetUsage() const noexcept { return data.description.usage; }
    uint32_t GetWidth() const noexcept { return data.description.width; }
    uint32_t GetHeight() const noexcept { return data.description.height; }
    uint32_t GetDepth() const noexcept { return data.description.depth; }
    DXGI_FORMAT GetFormat() const noexcept { return data.description.format; }
    const Description& GetDescription() const noexcept { return data.description; }
    const TextureData& GetData() const noexcept { return data; }

    void GenerateMips(Device& device);
    void Resize(uint32_t inWidth, uint32_t inHeight, Device& device);

    bool ImGuiDraw();

    void LoadResource(Device& device) override;

    Texture CreateCopy(Device& device, bool forceGenerateMips = false) const;
    Texture CreateStagingCopy(Device& device) const;
    
private:
    // Create from existing texture2d resource (specific for backbuffer)
    Texture(const std::string& name, const ComPtr<ID3D11Texture2D>& sourceTexture, DXGI_FORMAT format, uint8_t bindFlags, Device& device);
    Texture(const Texture& other) = default;
    Texture& operator=(const Texture& other) = default;
    void InitializeTexture(const void* textureData, Device& device);
    virtual void GenerateViews(Device& device);

    mutable TextureData data;

    ComPtr<ID3D11Resource> rawTexture{};

    ComPtr<ID3D11RenderTargetView> rtv{};
    ComPtr<ID3D11ShaderResourceView> srv{};
    ComPtr<ID3D11UnorderedAccessView> uav{};

    friend class Graphics;

};

template <class T>
void Texture::TextureData::FillWith(const T& value, int count)
{
    data.resize(count * sizeof(T));
    std::fill_n(reinterpret_cast<T*>(data.data()), count, value);
}

export inline Texture::TextureData CreateUniformColorTextureData(Color color, int width, int height, Texture::Type type, Texture::Usage usage, uint8_t bindFlags);

export struct TextureLoaderUtils
{
    std::filesystem::path GetProjectRelativeStoreFolder() const noexcept { return "Resources/Imported/Textures"; };
    std::string_view GetAssetDisplayName() const noexcept { return "Texture"; };
    
    void LoadPresets(Device& device, ResourceLoader<Texture>::CacheType& cache);
    std::unique_ptr<Texture> LoadAsset(ArchiveStream& readStream);
    void SaveAsset(const Texture& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs);
    bool HandlesExtension(const std::string& extension);
    bool CanLoadAsync() const noexcept { return true; }
};