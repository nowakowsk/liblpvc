#ifndef LIBLPVC_DETAIL_LPVC_IMPL_H
#define LIBLPVC_DETAIL_LPVC_IMPL_H

#include <algorithm>
#include <set>


namespace lpvc
{


struct ColorOrdering final
{
  bool operator()(const Color& lhs, const Color& rhs) const noexcept
  {
    return std::tie(lhs.r, lhs.g, lhs.b) < std::tie(rhs.r, rhs.g, rhs.b);
  }
};


template<typename ColorIterator>
Palette::Palette(ColorIterator begin, ColorIterator end) :
  size_(std::distance(begin, end))
{
  if(size_ > maxColorCount)
    throw std::invalid_argument("Too many colors to form a palette.");

  std::copy(begin, end, colors_.begin());
}


template<typename BitmapIterator>
Encoder::EncodeResult Encoder::encode(BitmapIterator bitmapIterator, std::byte* outputBuffer, bool keyFrame)
{
  BufferWriter bufferWriter(outputBuffer, safeOutputBufferSize());

  if(firstFrame_)
  {
    firstFrame_ = false;
    keyFrame = true;
  }

  if(keyFrame)
    writeBlock<KeyFrameBlock>(bufferWriter);

  if(!previousFrameBitmap_.empty() &&
     std::equal(previousFrameBitmap_.begin(), previousFrameBitmap_.end(), bitmapIterator))
  {
    writeBlock<NullBitmapBlock>(bufferWriter);
  }
  else
  {
    if(settings_.usePalette)
    {
      auto newPalette = makePalette(bitmapIterator);

      if(newPalette)
      {
        if(newPalette->size() == 1)
        {
          writeBlock<SolidColorBitmapBlock>(bufferWriter, (*newPalette)[0]);
        }
        else
        {
          updatePalette(bufferWriter, *newPalette);
          copyFrameBitmap(bitmapIterator);
          writeBlock<IndexedBitmapBlock>(bufferWriter);
        }
      }
      else
      {
        copyFrameBitmap(bitmapIterator);
        writeBlock<RawBitmapBlock>(bufferWriter);
      }
    }
    else
    {
      copyFrameBitmap(bitmapIterator);
      writeBlock<RawBitmapBlock>(bufferWriter);
    }

    previousFrameBitmap_ = frameBitmap_;
  }

  return { bufferWriter.offset(), keyFrame };
}


template<typename Block, typename ...Args>
void Encoder::writeBlock(BufferWriter& bufferWriter, Args&& ...args)
{
  bufferWriter.writeUInt8(variant_type_index<Block, FrameBlock>());
  Block().encode(*this, bufferWriter, std::forward<Args>(args)...);
}


template<typename BitmapIterator>
void Encoder::copyFrameBitmap(BitmapIterator bitmapIterator)
{
  std::copy_n(bitmapIterator, bitmapInfo_.width * bitmapInfo_.height, frameBitmap_.begin());
}


template<typename BitmapIterator>
std::optional<Palette> Encoder::makePalette(BitmapIterator bitmapIterator)
{
  // TODO: Instead of using heap-allocated std::set consider smarter implementation of Palette (fixed-size dynamic array?)
  std::set<Color, ColorOrdering> colorSet;

  for(std::size_t colorIdx = 0; colorIdx != bitmapInfo_.width * bitmapInfo_.height; ++bitmapIterator, ++colorIdx)
  {
    colorSet.insert(*bitmapIterator);

    if(colorSet.size() > Palette::maxColorCount)
      return std::nullopt;
  }

  return Palette(colorSet.begin(), colorSet.end());
}


template<typename BitmapIterator>
Decoder::DecodeResult Decoder::decode(const std::byte* inputBuffer, std::size_t inputBufferSize, BitmapIterator bitmapIterator)
{
  BufferReader bufferReader(inputBuffer, inputBufferSize);

  result_ = {};

  while(bufferReader.offset() != bufferReader.size())
  {
    auto frameBlockId = bufferReader.readUInt8();
    auto block = make_variant<FrameBlock>(frameBlockId);

    std::visit(
      [&, this](auto& block)
      {
        block.decode(*this, bufferReader);
      },
      block
    );
  }

  std::copy(frameBitmap_.begin(), frameBitmap_.end(), bitmapIterator);
  previousFrameBitmap_ = frameBitmap_;

  return result_;
}


} // namespace lpvc


#endif // LIBLPVC_DETAIL_LPVC_IMPL_H
