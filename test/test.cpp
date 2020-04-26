#define CATCH_CONFIG_MAIN

#include <lpvc/lpvc.h>
#include <catch2/catch.hpp>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>


static constexpr lpvc::Color makeColor(int r, int g, int b) noexcept
{
  return { static_cast<std::byte>(r), static_cast<std::byte>(g), static_cast<std::byte>(b) };
}


static void fillBitmap(std::vector<lpvc::Color>& bitmap, std::size_t colorCount)
{
  if(colorCount == 0 ||
     bitmap.size() < colorCount)
  {
    throw std::invalid_argument("Color count out of bounds.");
  }

  std::fill(bitmap.begin(), bitmap.end(), makeColor(0, 0, 0));

  std::size_t idx = 0;

  for(int r = 0; r < 256; ++r)
  {
    for(int g = 0; g < 256; ++g)
    {
      for(int b = 0; b < 256; ++b)
      {
        if(idx == bitmap.size())
          return;

        bitmap[idx++] = makeColor(r, g, b);

        if(idx == colorCount)
          return;
      }
    }
  }
}


TEST_CASE("Encoder and decoder results comparison", "")
{
  auto encoderSettings = GENERATE(
    lpvc::EncoderSettings { true, 1, 1 },
    lpvc::EncoderSettings { false, 1, 1 }
  );

  auto bitmapInfo = lpvc::BitmapInfo{17, 17};
  auto bitmapPixelCount = bitmapInfo.width * bitmapInfo.height;
  auto encoder = lpvc::Encoder(bitmapInfo, encoderSettings);
  auto decoder = lpvc::Decoder(bitmapInfo);
  auto encoderBuffer = std::vector<std::byte>(encoder.safeOutputBufferSize());
  auto inputBitmap = std::vector<lpvc::Color>(bitmapPixelCount);
  auto outputBitmap = std::vector<lpvc::Color>(bitmapPixelCount);

  auto inputAndOutputEqual = [&](std::size_t pixelCount, std::size_t colorCount, bool keyFrame)
  {
    fillBitmap(inputBitmap, colorCount);
    auto encodeResult = encoder.encode(inputBitmap.begin(), encoderBuffer.data(), keyFrame);
    auto decodeResult = decoder.decode(encoderBuffer.data(), encodeResult.bytesWritten, outputBitmap.data());

    assert(inputBitmap == outputBitmap);

    return inputBitmap == outputBitmap;
  };

  SECTION("Palette wave")
  {
    auto keyFrame = GENERATE(
      +[](std::size_t) { return true; },
      +[](std::size_t) { return false; },
      +[](std::size_t colorIdx) { return colorIdx % 2 ? true : false; },
      +[](std::size_t colorIdx) { return colorIdx % 2 ? false : true; }
    );

    for(std::size_t waveCycle = 0; waveCycle < 2; ++waveCycle)
    {
      for(std::size_t colorCount = 1; colorCount <= bitmapPixelCount; ++colorCount)
        REQUIRE(inputAndOutputEqual(bitmapPixelCount, colorCount, keyFrame(colorCount)));

      for(std::size_t colorCount = bitmapPixelCount; colorCount != 0; --colorCount)
        REQUIRE(inputAndOutputEqual(bitmapPixelCount, colorCount, keyFrame(colorCount)));
    }
  }
}
