module;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

module Graphics:STBImage;

import :STBImage;

STBImage::STBImage(const std::filesystem::path& filepath)
{
    std::string extension = filepath.extension().string();
    isFloatingPointFormat = extension == ".hdr";
    
    int outWidth, outHeight, outNbComponents;
    if (isFloatingPointFormat)
    {
        data = stbi_loadf(filepath.string().c_str(), &outWidth, &outHeight, &outNbComponents, ExpectedComponentCount);
        componentByteSize = sizeof(float);
    }
    else
    {
        data = stbi_load(filepath.string().c_str(), &outWidth, &outHeight, &outNbComponents, ExpectedComponentCount);
        componentByteSize = sizeof(uint8_t);
    }
    
    Assert(data, "Import data was incorrect");

    width = static_cast<uint32_t>(outWidth);
    height = static_cast<uint32_t>(outHeight);
    nbComponents = static_cast<uint32_t>(outNbComponents);
}

STBImage::~STBImage()
{
    stbi_image_free(data);
}

STBImage::STBImage(STBImage&& other) noexcept
{
    Swap(other);
}

STBImage& STBImage::operator=(STBImage&& other)
{
    Swap(other);
    return *this;
}

void STBImage::Swap(STBImage& other)
{
    data = std::exchange(other.data, nullptr);
    width = std::exchange(other.width, 0);
    height = std::exchange(other.height, 0);
    nbComponents = std::exchange(other.nbComponents, 0);
    componentByteSize = std::exchange(other.componentByteSize, 0);
    isFloatingPointFormat = std::exchange(other.isFloatingPointFormat, false);
}
