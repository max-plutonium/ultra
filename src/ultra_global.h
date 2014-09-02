#ifndef ULTRA_GLOBAL_H
#define ULTRA_GLOBAL_H

#include <cstdint> // std::size_t
#include <bits/move.h> // std::addressof

#ifdef ULTRA_SHARED
#  define ULTRA_EXPORT Q_DECL_EXPORT
#else
#  define ULTRA_EXPORT Q_DECL_IMPORT
#endif

#ifdef QT_DEBUG
#   define ULTRA_PRIVATE Q_DECL_IMPORT
#else
#   define ULTRA_PRIVATE Q_DECL_HIDDEN
#endif

namespace ultra {

template <std::size_t... Indices>
  struct tuple_indices {
      typedef tuple_indices<Indices..., sizeof... (Indices)> next;
  };

template <std::size_t N>
  struct tuple_indices_builder {
      typedef typename tuple_indices_builder<N - 1>::type::next type;
  };

template <>
  struct tuple_indices_builder<0> {
      typedef tuple_indices<> type;
  };

template <typename Tp>
  static inline constexpr Tp *_get_ptr(Tp *tp) { return tp; }
template <typename Wrap>
  static inline constexpr typename Wrap::element_type *_get_ptr(Wrap &w)
  { return std::addressof(*w); }
template <typename Wrap>
  static inline constexpr typename Wrap::pointer _get_ptr(Wrap const &w)
  { return std::addressof(*w); }

#define unique_impl(impl)  class impl; std::unique_ptr<impl> _##impl
#define shared_impl(impl)  class impl; std::shared_ptr<impl> _##impl

#define decl_impl(impl) private: \
    inline impl *_get_##impl() \
    { return reinterpret_cast<impl *>(_get_ptr(_##impl)); } \
    inline const impl *_get_##impl() const \
    { return reinterpret_cast<const impl *>(_get_ptr(_##impl)); } \
    friend class impl

#define decl_face(face) \
    inline face *_get_##face() { return static_cast<face *>(_get_ptr(_##face)); } \
    inline const face *_get_##face() const \
    { return static_cast<const face *>(_get_ptr(_##face)); } \
    friend class face

#define def(ptr) auto *const ptr = _get_#ptr()

} // namespace ultra

#endif // ULTRA_GLOBAL_H
