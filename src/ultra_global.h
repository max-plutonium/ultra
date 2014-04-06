#ifndef ULTRA_GLOBAL_H
#define ULTRA_GLOBAL_H

#include <QtCore/qglobal.h>

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

#ifdef Q_CC_GNU
#   define ULTRA_CONSTRUCTOR __attribute__((constructor))
#   define ULTRA_DESTRUCTOR __attribute__((destructor))
#   define ULTRA_LIKELY(V) __builtin_expect(!!(V), 1)
#   define ULTRA_UNLIKELY(V) __builtin_expect(!!(V), 0)
#else
#   define ULTRA_CONSTRUCTOR
#   define ULTRA_DESTRUCTOR
#   define ULTRA_LIKELY(V)
#   define ULTRA_UNLIKELY(V)
#endif

#include <bits/move.h> // std::addressof

template <typename Tp>
static constexpr Tp *_get_ptr(Tp *tp) { return tp; }
template <typename Wrap>
static constexpr typename Wrap::element_type *_get_ptr(Wrap &w)
{ return std::addressof(*w); }
template <typename Wrap>
static constexpr typename Wrap::pointer _get_ptr(Wrap const &w)
{ return std::addressof(*w); }

#define implclass(C)    class C##_impl
#define faceclass(C)    class C
#define unique_impl(C)  C##_impl *const _implptr
#define shared_impl(C)  std::shared_ptr<C##_impl> _implptr
#define face_ptr(C)     C *_faceptr = nullptr

#define decl_impl(C) private: \
    inline C##_impl *_get_impl() \
    { return reinterpret_cast<C##_impl*>(_get_ptr(_implptr)); } \
    inline const C##_impl *_get_impl() const \
    { return reinterpret_cast<const C##_impl*>(_get_ptr(_implptr)); } \
    friend class C##_impl

#define decl_impl_name(C, P) private: \
    inline C##_impl *_get_impl() \
    { return reinterpret_cast<C##_impl*>(_get_ptr(P)); } \
    inline const C##_impl *_get_impl() const \
    { return reinterpret_cast<const C##_impl*>(_get_ptr(P)); } \
    friend class C##_impl

#define decl_face(C) \
    inline C *_get_face() { return static_cast<C*>(_get_ptr(_faceptr)); } \
    inline const C *_get_face() const \
    { return static_cast<const C*>(_get_ptr(_faceptr)); } \
    friend class C

#define pimpl auto *const impl = _get_impl()
#define pface auto *const face = _get_face()

#endif // ULTRA_GLOBAL_H
