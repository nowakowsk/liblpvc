#ifndef LIBLPVC_DETAIL_VARIANT_UTILS_H
#define LIBLPVC_DETAIL_VARIANT_UTILS_H

#include <variant>
#include <stdexcept>


namespace lpvc
{


// ===========================================================================
//  make_variant
// ===========================================================================

namespace detail
{

template<typename Variant>
struct MakeVariantImpl;

template<typename ...Ts>
struct MakeVariantImpl<std::variant<Ts...>>
{
  template<std::size_t Index, typename, typename ...Rest, typename ...Args>
  static decltype(auto) make_variant_impl(std::size_t index, Args&& ...args)
  {
    if(Index == index)
      return std::variant<Ts...>(std::in_place_index_t<Index>(), std::forward<Args>(args)...);

    if constexpr(sizeof...(Rest) != 0)
      return make_variant_impl<Index + 1, Rest...>(index, std::forward<Args>(args)...);
    else
      throw std::runtime_error("Invalid variant index");
  }

  template<typename ...Args>
  static decltype(auto) make_variant(std::size_t index, Args&& ...args)
  {
    return make_variant_impl<0, Ts...>(index, std::forward<Args>(args)...);
  }
};

} // namespace detail


template<typename Variant, typename ...Args>
decltype(auto) make_variant(std::size_t index, Args&& ...args)
{
  return detail::MakeVariantImpl<Variant>::template make_variant(index, std::forward<Args>(args)...);
}


// ===========================================================================
//  variant_type_index
// ===========================================================================

namespace detail
{

template<std::size_t, typename Type, typename Variant>
struct VariantTypeIndexImpl;

template<std::size_t Index, typename Type, typename VariantType, typename ...VariantTypes>
struct VariantTypeIndexImpl<Index, Type, std::variant<VariantType, VariantTypes...>>
{
  static constexpr std::size_t variant_type_index() noexcept
  {
    if constexpr(std::is_same_v<Type, VariantType>)
      return Index;
    else
      return VariantTypeIndexImpl<Index + 1, Type, std::variant<VariantTypes...>>::variant_type_index();
  }
};

} // namespace detail


template<typename Type, typename Variant>
constexpr std::size_t variant_type_index() noexcept
{
  return detail::VariantTypeIndexImpl<0, Type, Variant>::variant_type_index();
}


} // namespace lpvc


#endif // LIBLPVC_DETAIL_VARIANT_UTILS_H
