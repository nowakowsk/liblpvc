#ifndef LIBLPVC_LPVC_H
#define LIBLPVC_LPVC_H

#include <lpvc/detail/serialization.h>
#include <lpvc/detail/variant_utils.h>
#include <lpvc/detail/zstd_wrapper.h>
#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>


namespace lpvc
{


// ===========================================================================
//  Forward declarations
// ===========================================================================

class Encoder;
class Decoder;


// ===========================================================================
//  Version
// ===========================================================================

unsigned int version() noexcept;
const char* versionString() noexcept;


// ===========================================================================
//  Color
// ===========================================================================

struct Color final
{
  std::byte r;
  std::byte g;
  std::byte b;

  bool operator==(const Color& other) const noexcept;
  bool operator!=(const Color& other) const noexcept;
};

static_assert(sizeof(Color) == 3);


struct ColorHash final
{
  std::size_t operator()(const Color& color) const noexcept;
};


// ===========================================================================
//  BitmapInfo
// ===========================================================================

struct BitmapInfo final
{
  std::size_t width = 0;
  std::size_t height = 0;
};


// ===========================================================================
//  Palette
// ===========================================================================

class Palette final
{
public:
  static constexpr std::size_t maxColorCount = 256;

  using Iterator = Color*;
  using ConstIterator = const Color*;

  Palette() noexcept = default;
  Palette(std::size_t size) noexcept;

  template<typename ColorIterator>
  Palette(ColorIterator begin, ColorIterator end);

  std::size_t size() const noexcept;
  void clear() noexcept;

  Iterator begin() noexcept;
  Iterator end() noexcept;

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;

  Color& operator[](std::size_t index) noexcept;
  const Color& operator[](std::size_t index) const noexcept;
  
  std::size_t bits() const;

  Palette difference(const Palette& other) const;
  Palette merge(const Palette& other) const;

private:
  std::array<Color, maxColorCount> colors_ {};
  std::size_t size_ = 0;
};


// ===========================================================================
//  FrameBlock
// ===========================================================================

struct KeyFrameBlock;
struct PaletteBlock;
struct PaletteResetBlock;
struct IndexedBitmapBlock;
struct RawBitmapBlock;
struct SolidColorBitmapBlock;
struct NullBitmapBlock;

// NOTE: Order of block types within FrameBlock does matter!
// Do not remove block types. When adding a new block type, add it at the end
// of the list.

using FrameBlock = std::variant<
  KeyFrameBlock,
  PaletteBlock,
  PaletteResetBlock,
  IndexedBitmapBlock,
  RawBitmapBlock,
  SolidColorBitmapBlock,
  NullBitmapBlock
>;


// ===========================================================================
//  KeyFrameBlock
// ===========================================================================

struct KeyFrameBlock final
{
  static std::size_t maxSize() noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  PaletteBlock
// ===========================================================================

struct PaletteBlock final
{
  static std::size_t maxSize() noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter, const Palette& palette);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  PaletteResetBlock
// ===========================================================================

struct PaletteResetBlock final
{
  static std::size_t maxSize() noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  IndexedBitmapBlock
// ===========================================================================

struct IndexedBitmapBlock final
{
  static std::size_t maxSize(const BitmapInfo& bitmapInfo) noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  RawBitmapBlock
// ===========================================================================

struct RawBitmapBlock final
{
  static std::size_t maxSize(const BitmapInfo& bitmapInfo) noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  SolidColorBitmapBlock
// ===========================================================================

struct SolidColorBitmapBlock final
{
  static std::size_t maxSize() noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter, const Color& color);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  NullBitmapBlock
// ===========================================================================

struct NullBitmapBlock final
{
  static std::size_t maxSize() noexcept;

  void encode(Encoder& encoder, BufferWriter& bufferWriter);
  void decode(Decoder& decoder, BufferReader& bufferReader);
};


// ===========================================================================
//  Encoder
// ===========================================================================

// TODO: Put EncoderSettings inside Encoder body (currently impossible because
// of a bug in GCC).

struct EncoderSettings final
{
  bool usePalette = true;
  int zstdCompressionLevel = 18;
  int zstdWorkerCount = 1;
};


class Encoder final
{
public:
  struct EncodeResult
  {
    std::size_t bytesWritten = 0;
    bool keyFrame = false;
  };

  Encoder(const BitmapInfo& bitmapInfo, const EncoderSettings& settings = {});

  std::size_t safeOutputBufferSize() const noexcept;

  template<typename BitmapIterator>
  EncodeResult encode(BitmapIterator bitmapIterator, std::byte* outputBuffer, bool keyFrame);

private:
  template<typename Block, typename ...Args>
  void writeBlock(BufferWriter& bufferWriter, Args&& ...args);

  template<typename BitmapIterator>
  void copyFrameBitmap(BitmapIterator bitmapIterator);

  template<typename BitmapIterator>
  std::optional<Palette> makePalette(BitmapIterator bitmapIterator);

  void updatePalette(BufferWriter& bufferWriter, const Palette& newPalette);
  void compressBuffer(BufferWriter& bufferWriter, const std::byte* inputBuffer, std::size_t inputBufferSize);

  void resetPalette();
  void reset();

  EncoderSettings settings_;
  BitmapInfo bitmapInfo_;
  std::vector<Color> frameBitmap_;
  std::vector<Color> previousFrameBitmap_;
  std::vector<std::byte> internalBuffer_;
  Palette palette_;
  std::unordered_map<Color, unsigned char, ColorHash> colorMap_;
  bool firstFrame_ = true;
  ZSTDCCtx zstdCompressor_;

  friend struct KeyFrameBlock;
  friend struct PaletteBlock;
  friend struct PaletteResetBlock;
  friend struct IndexedBitmapBlock;
  friend struct RawBitmapBlock;
  friend struct SolidColorBitmapBlock;
  friend struct NullBitmapBlock;
};


// ===========================================================================
//  Decoder
// ===========================================================================

class Decoder final
{
public:
  struct DecodeResult
  {
    bool keyFrame = false;
  };

  Decoder(const BitmapInfo& bitmapInfo);

  template<typename BitmapIterator>
  DecodeResult decode(const std::byte* inputBuffer, std::size_t inputBufferSize, BitmapIterator bitmapIterator);

private:
  void decompressBuffer(BufferReader& bufferReader, std::byte* outputBuffer, std::size_t outputBufferSize);

  void resetPalette();
  void reset();

  BitmapInfo bitmapInfo_;
  std::vector<Color> frameBitmap_;
  std::vector<Color> previousFrameBitmap_;
  std::vector<std::byte> internalBuffer_;
  Palette palette_;
  ZSTDDCtx zstdDecompressor_;
  DecodeResult result_;

  friend struct KeyFrameBlock;
  friend struct PaletteBlock;
  friend struct PaletteResetBlock;
  friend struct IndexedBitmapBlock;
  friend struct RawBitmapBlock;
  friend struct SolidColorBitmapBlock;
  friend struct NullBitmapBlock;
};


} // namespace lpvc


#include <lpvc/detail/lpvc_impl.h>


#endif // LIBLPVC_LPVC_H
