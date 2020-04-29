#include <lpvc/lpvc.h>
#include <algorithm>
#include <tuple>


namespace lpvc
{


bool Color::operator==(const Color& other) const noexcept
{
  return r == other.r &&
         g == other.g &&
         b == other.b;
}


bool Color::operator!=(const Color& other) const noexcept
{
  return !operator==(other);
}


std::size_t ColorHash::operator()(const Color& color) const noexcept
{
  return (std::to_integer<std::size_t>(color.r) << 0) |
         (std::to_integer<std::size_t>(color.g) << 8) |
         (std::to_integer<std::size_t>(color.b) << 16);
}


Palette::Palette(std::size_t size) noexcept :
  size_(size)
{
}


std::size_t Palette::size() const noexcept
{
  return size_;
}


void Palette::clear() noexcept
{
  size_ = 0;
}


Palette::Iterator Palette::begin() noexcept
{
  return colors_.data();
}


Palette::Iterator Palette::end() noexcept
{
  return colors_.data() + size_;
}


Palette::ConstIterator Palette::begin() const noexcept
{
  return colors_.data();
}


Palette::ConstIterator Palette::end() const noexcept
{
  return colors_.data() + size_;
}


Color& Palette::operator[](std::size_t index) noexcept
{
  return colors_[index];
}


const Color& Palette::operator[](std::size_t index) const noexcept
{
  return colors_[index];
}


std::size_t Palette::bits() const
{
  if(size_ <= 1)
    return 0;
  else if(size_ <= 2)
    return 1;
  else if(size_ <= 4)
    return 2;
  else if(size_ <= 16)
    return 4;
  else if(size_ <= 256)
    return 8;

  throw std::runtime_error("Invalid palette (too many colors).");
}


Palette Palette::difference(const Palette& other) const
{
  Palette palette;

  palette.size_ = std::set_difference(other.begin(), other.begin() + other.size_, begin(), begin() + size_, palette.begin(), ColorOrdering()) - palette.begin();

  return palette;
}


Palette Palette::merge(const Palette& other) const
{
  Palette palette;

  palette.size_ = std::set_union(other.begin(), other.begin() + other.size_, begin(), begin() + size_, palette.begin(), ColorOrdering()) - palette.begin();

  return palette;
}


static void writeColor(BufferWriter& bufferWriter, const Color& color)
{
  bufferWriter.writeByte(color.r);
  bufferWriter.writeByte(color.g);
  bufferWriter.writeByte(color.b);
}


static Color readColor(BufferReader& bufferReader)
{
  Color color;

  color.r = bufferReader.readByte();
  color.g = bufferReader.readByte();
  color.b = bufferReader.readByte();

  return color;
}


std::size_t KeyFrameBlock::maxSize() noexcept
{
  return 0;
}


void KeyFrameBlock::encode(Encoder& encoder, BufferWriter& bufferWriter)
{
  encoder.reset();
}


void KeyFrameBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  decoder.reset();
  decoder.result_.keyFrame = true;
}


std::size_t PaletteBlock::maxSize() noexcept
{
  std::size_t size = 0;

  size += sizeof(std::uint8_t); // Color count
  size += Palette::maxColorCount * sizeof(Color); // Colors

  return size;
}


void PaletteBlock::encode(Encoder& encoder, BufferWriter& bufferWriter, const Palette& palette)
{
  if(palette.size() == 0)
    throw std::logic_error("Palettes with 0 colors are not allowed.");

  BufferWriter internalBufferWriter(encoder.internalBuffer_.data(), encoder.internalBuffer_.size());

  // Size is decreased by 1 not to overflow uint8 with 256 color palette.
  // It can be interpreted as saving index of the last color in the palette.
  internalBufferWriter.writeUInt8(palette.size() - 1);

  for(const auto& color : palette)
    writeColor(internalBufferWriter, color);

  encoder.compressBuffer(bufferWriter, internalBufferWriter.data(), internalBufferWriter.offset());

  //
  encoder.palette_ = encoder.palette_.merge(palette);

  encoder.colorMap_.clear();
  for(std::size_t colorIdx = 0; colorIdx < encoder.palette_.size(); ++colorIdx)
    encoder.colorMap_.emplace(encoder.palette_[colorIdx], static_cast<unsigned char>(colorIdx));
}


void PaletteBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  decoder.decompressBuffer(bufferReader, decoder.internalBuffer_.data(), decoder.internalBuffer_.size());
  BufferReader internalBufferReader(decoder.internalBuffer_.data(), decoder.internalBuffer_.size());

  Palette palette(static_cast<std::size_t>(internalBufferReader.readUInt8()) + 1); // See PaletteBlock::encode.

  for(Color& color : palette)
    color = readColor(internalBufferReader);

  decoder.palette_ = decoder.palette_.merge(palette);
}


std::size_t PaletteResetBlock::maxSize() noexcept
{
  return 0;
}


void PaletteResetBlock::encode(Encoder& encoder, BufferWriter& bufferWriter)
{
  encoder.resetPalette();
}


void PaletteResetBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  decoder.resetPalette();
}


std::size_t IndexedBitmapBlock::maxSize(const BitmapInfo& bitmapInfo) noexcept
{
  std::size_t size = 0;

  size += sizeof(std::uint8_t); // Bit count
  size += bitmapInfo.width * bitmapInfo.height * sizeof(std::uint8_t); // Indexed bitmap (8-bit)

  return size;
}


void IndexedBitmapBlock::encode(Encoder& encoder, BufferWriter& bufferWriter)
{
  BufferWriter internalBufferWriter(encoder.internalBuffer_.data(), encoder.internalBuffer_.size());

  auto paletteBits = encoder.palette_.bits();
  unsigned char compressedBitmap = 0;
  std::size_t compressedOffset = 0;

  internalBufferWriter.writeUInt8(paletteBits);
  for(const auto& color : encoder.frameBitmap_)
  {
    auto paletteIndex = encoder.colorMap_.at(color);

    compressedBitmap |= (paletteIndex << compressedOffset);
    compressedOffset += paletteBits;

    if(compressedOffset == 8)
    {
      internalBufferWriter.writeUInt8(compressedBitmap);
      compressedBitmap = 0;
      compressedOffset = 0;
    }
  }

  if(compressedOffset != 0)
    internalBufferWriter.writeUInt8(compressedBitmap);

  encoder.compressBuffer(bufferWriter, internalBufferWriter.data(), internalBufferWriter.offset());
}


void IndexedBitmapBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  decoder.decompressBuffer(bufferReader, decoder.internalBuffer_.data(), decoder.internalBuffer_.size());
  BufferReader internalBufferReader(decoder.internalBuffer_.data(), decoder.internalBuffer_.size());

  unsigned char compressedBitmap = 0;
  std::size_t compressedOffset = 8;
  auto paletteBits = internalBufferReader.readUInt8();

  for(auto& color : decoder.frameBitmap_)
  {
    if(compressedOffset == 8)
    { 
      compressedBitmap = internalBufferReader.readUInt8();
      compressedOffset = 0;
    }

    unsigned char paletteIndex = compressedBitmap;
    paletteIndex <<= 8 - paletteBits - compressedOffset;
    paletteIndex >>= (8 - paletteBits);

    color = decoder.palette_[paletteIndex];
    compressedOffset += paletteBits;
  }
}


std::size_t RawBitmapBlock::maxSize(const BitmapInfo& bitmapInfo) noexcept
{
  return bitmapInfo.width * bitmapInfo.height * sizeof(Color);
}


void RawBitmapBlock::encode(Encoder& encoder, BufferWriter& bufferWriter)
{
  encoder.compressBuffer(bufferWriter, reinterpret_cast<const std::byte*>(encoder.frameBitmap_.data()), encoder.frameBitmap_.size() * sizeof(Color));
}


void RawBitmapBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  decoder.decompressBuffer(bufferReader, reinterpret_cast<std::byte*>(decoder.frameBitmap_.data()), decoder.frameBitmap_.size() * sizeof(Color));
}


std::size_t SolidColorBitmapBlock::maxSize() noexcept
{
  return sizeof(Color);
}


void SolidColorBitmapBlock::encode(Encoder& encoder, BufferWriter& bufferWriter, const Color& color)
{
  writeColor(bufferWriter, color);
  std::fill(encoder.frameBitmap_.begin(), encoder.frameBitmap_.end(), color);
}


void SolidColorBitmapBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  std::fill(decoder.frameBitmap_.begin(), decoder.frameBitmap_.end(), readColor(bufferReader));
}


std::size_t NullBitmapBlock::maxSize() noexcept
{
  return 0;
}


void NullBitmapBlock::encode(Encoder& encoder, BufferWriter& bufferWriter)
{
}


void NullBitmapBlock::decode(Decoder& decoder, BufferReader& bufferReader)
{
  decoder.frameBitmap_ = decoder.previousFrameBitmap_;
}


static std::size_t safeInternalOutpuBufferSize(const BitmapInfo& bitmapInfo) noexcept
{
  return std::max(PaletteBlock::maxSize(), IndexedBitmapBlock::maxSize(bitmapInfo));
}


Encoder::Encoder(const BitmapInfo& bitmapInfo, const EncoderSettings& settings) :
  settings_(settings),
  bitmapInfo_(bitmapInfo),
  frameBitmap_(bitmapInfo_.width * bitmapInfo_.height),
  internalBuffer_(safeInternalOutpuBufferSize(bitmapInfo)),
  colorMap_(Palette::maxColorCount)
{
  zstdCompressor_.reset(ZSTD_createCCtx());
  ZSTD_CCtx_setParameter(zstdCompressor_.get(), ZSTD_c_compressionLevel, settings_.zstdCompressionLevel);
  ZSTD_CCtx_setParameter(zstdCompressor_.get(), ZSTD_c_nbWorkers, settings_.zstdWorkerCount);
}


std::size_t Encoder::safeOutputBufferSize() const noexcept
{
  auto fullBlockSize = [](std::size_t blockSize)
  {
    return sizeof(std::uint8_t) + // Block type id
           blockSize;             // Block data
  };

  auto compressedBlockSize = [](std::size_t blockSize)
  {
    return sizeof(std::uint32_t) +        // Compressed block size
           ZSTD_compressBound(blockSize); // Block data 
  };

  const auto indexedBitmapWithPaletteSize = fullBlockSize(compressedBlockSize(PaletteResetBlock::maxSize())) +
                                            fullBlockSize(compressedBlockSize(PaletteBlock::maxSize())) +
                                            fullBlockSize(compressedBlockSize(IndexedBitmapBlock::maxSize(bitmapInfo_)));

  const auto rawBitmapSize = fullBlockSize(compressedBlockSize(RawBitmapBlock::maxSize(bitmapInfo_)));

  const auto solidColorBitmapSize = fullBlockSize(SolidColorBitmapBlock::maxSize());

  return fullBlockSize(KeyFrameBlock::maxSize()) +
         std::max({ indexedBitmapWithPaletteSize, rawBitmapSize, solidColorBitmapSize });
}


void Encoder::updatePalette(BufferWriter& bufferWriter, const Palette& newPalette)
{
  auto newColors = palette_.difference(newPalette);

  if(newColors.size() > 0)
  {
    auto newPaletteMaxColorCount = (std::size_t(1) << newPalette.bits());

    if(palette_.size() + newColors.size() > newPaletteMaxColorCount)
    {
      if(palette_.size() != 0)
        writeBlock<PaletteResetBlock>(bufferWriter);

      writeBlock<PaletteBlock>(bufferWriter, newPalette);
    }
    else
    {
      writeBlock<PaletteBlock>(bufferWriter, newColors);
    }
  }
}


void Encoder::compressBuffer(BufferWriter& bufferWriter, const std::byte* inputBuffer, std::size_t inputBufferSize)
{
  auto& compressedSize = bufferWriter.writeUInt32(0); // Placeholder for compressed data size. Set after compression is finished.

  ZSTD_inBuffer zstdInput = { inputBuffer, inputBufferSize, 0 };
  ZSTD_outBuffer zstdOutput = { bufferWriter.data() + bufferWriter.offset(), bufferWriter.size() - bufferWriter.offset(), 0 };

  while(zstdInput.pos != zstdInput.size)
    ZSTD_compressStream2(zstdCompressor_.get(), &zstdOutput , &zstdInput, ZSTD_e_flush);

  compressedSize = static_cast<std::uint32_t>(zstdOutput.pos);
  bufferWriter.advance(compressedSize);
}


void Encoder::resetPalette()
{
  palette_.clear();
  colorMap_.clear();
}


void Encoder::reset()
{
  resetPalette();
  previousFrameBitmap_.clear();
  ZSTD_CCtx_reset(zstdCompressor_.get(), ZSTD_reset_session_only);
}


Decoder::Decoder(const BitmapInfo& bitmapInfo) :
  bitmapInfo_(bitmapInfo),
  frameBitmap_(bitmapInfo_.width * bitmapInfo_.height),
  internalBuffer_(safeInternalOutpuBufferSize(bitmapInfo))
{
  zstdDecompressor_.reset(ZSTD_createDCtx());
}


void Decoder::decompressBuffer(BufferReader& bufferReader, std::byte* outputBuffer, std::size_t outputBufferSize)
{
  auto compressedSize = bufferReader.readUInt32();

  ZSTD_inBuffer zstdInput = { bufferReader.data() + bufferReader.offset(), compressedSize, 0 };
  ZSTD_outBuffer zstdOutput = { outputBuffer, outputBufferSize, 0 };

  while(zstdInput.pos < zstdInput.size)
    ZSTD_decompressStream(zstdDecompressor_.get(), &zstdOutput , &zstdInput);

  bufferReader.advance(compressedSize);
}


void Decoder::resetPalette()
{
  palette_.clear();
}


void Decoder::reset()
{
  resetPalette();
  ZSTD_DCtx_reset(zstdDecompressor_.get(), ZSTD_reset_session_only);
}


} // namespace lpvc
