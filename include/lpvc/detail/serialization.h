#ifndef LIBLPVC_DETAIL_SERIALIZATION_H
#define LIBLPVC_DETAIL_SERIALIZATION_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>


namespace lpvc
{


// ===========================================================================
//  BufferWriter
// ===========================================================================

class BufferWriter final
{
public:
  BufferWriter(std::byte* buffer, std::size_t size) noexcept :
    buffer_(buffer),
    size_(size)
  {
  }

  std::size_t size() const noexcept
  {
    return size_;
  }

  std::size_t offset() const noexcept
  {
    return offset_;
  }

  void advance(std::size_t offset)
  {
    if(offset_ + offset > size_)
      throw std::out_of_range("Buffer overflow");

    offset_ += offset;
  }

  std::byte* data() noexcept
  {
    return buffer_;
  }

  const std::byte* data() const noexcept
  {
    return buffer_;
  }

  template<typename T>
  auto& writeInt8(T value)
  {
    return write<std::uint8_t>(value);
  }

  template<typename T>
  auto& writeInt16(T value)
  {
    return write<std::uint16_t>(value);
  }

  template<typename T>
  auto& writeInt32(T value)
  {
    return write<std::uint32_t>(value);
  }

  template<typename T>
  auto& writeInt64(T value)
  {
    return write<std::uint64_t>(value);
  }

  template<typename T>
  auto& writeUInt8(T value)
  {
    return write<std::uint8_t>(value);
  }

  template<typename T>
  auto& writeUInt16(T value)
  {
    return write<std::uint16_t>(value);
  }

  template<typename T>
  auto& writeUInt32(T value)
  {
    return write<std::uint32_t>(value);
  }

  template<typename T>
  auto& writeUInt64(T value)
  {
    return write<std::uint64_t>(value);
  }

  auto& writeByte(std::byte value)
  {
    return write<std::uint8_t>(std::to_integer<uint8_t>(value));
  }
  
private:
  template<typename C, typename T>
  C& write(T value)
  {
    static_assert(
      std::numeric_limits<T>::is_integer &&
      std::numeric_limits<C>::is_integer
    );

    if(offset_ + sizeof(C) > size_)
      throw std::out_of_range("Buffer overflow");

    auto castValue = static_cast<C>(value);
    auto& destination = *reinterpret_cast<C*>(buffer_ + offset_);

    destination = castValue;
    offset_ += sizeof(C);

    return destination;
  }

  std::byte* buffer_ = nullptr;
  std::size_t size_ = 0;
  std::size_t offset_ = 0;
};


// ===========================================================================
//  BufferReader
// ===========================================================================

class BufferReader final
{
public:
  BufferReader(const std::byte* buffer, std::size_t size) noexcept :
    buffer_(buffer),
    size_(size)
  {
  }

  std::size_t size() const noexcept
  {
    return size_;
  }

  std::size_t offset() const noexcept
  {
    return offset_;
  }

  void advance(std::size_t offset)
  {
    if(offset_ + offset > size_)
      throw std::out_of_range("Buffer overflow");

    offset_ += offset;
  }

  const std::byte* data() const noexcept
  {
    return buffer_;
  }

  uint8_t readInt8()
  {
    return read<std::uint8_t>();
  }

  uint16_t readInt16()
  {
    return read<std::uint16_t>();
  }

  uint32_t readInt32()
  {
    return read<std::uint32_t>();
  }

  uint64_t readInt64()
  {
    return read<std::uint64_t>();
  }

  uint8_t readUInt8()
  {
    return read<std::uint8_t>();
  }

  uint16_t readUInt16()
  {
    return read<std::uint16_t>();
  }

  uint32_t readUInt32()
  {
    return read<std::uint32_t>();
  }

  uint64_t readUInt64()
  {
    return read<std::uint64_t>();
  }

  std::byte readByte()
  {
    return read<std::byte>();
  }

private:
  template<typename T>
  T read()
  {
    if(offset_ + sizeof(T) > size_)
      throw std::out_of_range("Buffer overflow");

    auto valuePtr = reinterpret_cast<const T*>(buffer_ + offset_);

    offset_ += sizeof(T);

    return *valuePtr;
  }

  const std::byte* buffer_ = nullptr;
  std::size_t size_ = 0;
  std::size_t offset_ = 0;
};


} // namespace lpvc


#endif // LIBLPVC_DETAIL_SERIALIZATION_H
