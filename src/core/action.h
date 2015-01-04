#ifndef ACTION_H
#define ACTION_H

#include "../ultra_global.h"
#include <tuple>
#include <memory>

namespace ultra { namespace core {

namespace details {

template <std::size_t... Indices> struct index_sequence {
    using Next = index_sequence<Indices..., sizeof... (Indices)>;
};

template <std::size_t Num> struct index_sequence_bulder {
    using Type = typename index_sequence_bulder<Num - 1>::Type::Next;
};

template <> struct index_sequence_bulder<0> {
    using Type = index_sequence<>;
};

template <std::size_t Head, std::size_t... Tail> struct last_index {
    enum { Value = last_index<Tail...>::Value };
};

template <std::size_t Head> struct last_index<Head> {
    enum { Value = Head };
};

template <std::size_t... Indices> struct index_range { };

template <std::size_t Start, std::size_t... Indices>
struct index_range<Start, Indices...> {
    using Next = index_range<Start, Indices..., last_index<Indices...>::Value + 1>;
};

template <std::size_t Start> struct index_range<Start> {
    using Next = index_range<Start, Start + 1>;
};

template <std::size_t Start, std::size_t Num> struct index_range_bulder {
    using Type = typename index_range_bulder<Start, Num - 1>::Type::Next;
};

template <std::size_t Start> struct index_range_bulder<Start, 1> {
    using Type = index_range<Start>;
};

template <std::size_t Start> struct index_range_bulder<Start, 0> {
    using Type = index_range<>;
};

template <std::size_t Num>
  using make_index_sequence = typename index_sequence_bulder<Num>::Type;

template <typename... Args>
  using make_index_sequence_for = make_index_sequence<sizeof... (Args)>;

template <std::size_t Start, std::size_t Num>
  using make_index_range = typename index_range_bulder<Start, Num>::Type;

template <typename Tp>
  struct return_value_setter {
      Tp *ptr;
      explicit return_value_setter(Tp *tp) : ptr(tp) { }
  };

template<typename Tp, typename Up>
  void operator ,(Tp &&value, const return_value_setter<Up> &setter) {
      if (setter.ptr)
          *reinterpret_cast<Up*>(setter.ptr) = std::forward<Tp>(value);
  }

template<typename Tp>
  void operator ,(Tp, const return_value_setter<void> &) { }

} // namespace details


template <typename...> class curried_function;

template <typename Function, typename... BoundArgs>
class curried_function<Function, BoundArgs...>
{
    Function f;
    std::tuple<typename std::decay<BoundArgs>::type...> t;

  template <typename Tp>
    inline static constexpr Tp &get_helper(Tp &tp) { return tp; }

  template <typename Tp>
    inline static constexpr Tp &get_helper(std::reference_wrapper<Tp> &tp) { return tp.get(); }

  template <typename Result, typename... Args, std::size_t... BoundIndices>
    void apply(Result *res, details::index_sequence<BoundIndices...>, Args &&...args) const {
        (f(get_helper(std::get<BoundIndices>(t))..., std::forward<Args>(args)...)),
                details::return_value_setter<Result>(res);
    }

public:
  template <typename Functor, typename... Args>
    explicit curried_function(Functor &&af, Args &&...args)
        : f(std::forward<Functor>(af)), t(std::forward<Args>(args)...) { }

  template <typename Result, typename... Args>
    void operator()(Result *res, Args &&...args) const {
        apply(res, details::make_index_sequence_for<BoundArgs...>(), std::forward<Args>(args)...);
    }
};

template <typename...> class action_base;

template <typename Res, typename... Arguments>
class action_base<Res (Arguments...)>
{
protected:
    class holder_base {
        holder_base(const holder_base &) = delete;
        holder_base &operator=(const holder_base &) = delete;
        using Invoker = void (*)(holder_base *, Res *, Arguments...);
        using Destroyer = void (*)(holder_base *);
        Invoker invoker;
        Destroyer destroyer;

    public:
        holder_base(Invoker i, Destroyer d) : invoker(i), destroyer(d) { }
        inline void invoke(Res *res, Arguments ...args) { return invoker(this, res, args...); }
        inline void destroy() { destroyer(this); }
    };

    struct holder_destroyer {
        inline void operator()(holder_base *holder) {
            holder->destroy();
        }
    };

    std::shared_ptr<holder_base> holder;

  template <typename Function>
    class function_holder : public holder_base {
        Function f;

      template <typename Tp>
        inline static constexpr Tp &get_helper(Tp &tp) { return tp; }

      template <typename Tp>
        inline static constexpr Tp &get_helper(std::reference_wrapper<Tp> &tp) { return tp.get(); }

        static void invoke(holder_base *thiz, Res *res, Arguments ...args) {
            (static_cast<function_holder *>(thiz)->f(get_helper(args)...)), details::return_value_setter<Res>(res);
        }

        static void destroy(holder_base *thiz) {
            delete static_cast<function_holder *>(thiz);
        }

    public:
        explicit function_holder(Function &&af)
            : holder_base(&invoke, &destroy), f(af) { }
    };

  template <typename Function, typename... BoundArgs>
    class function_holder<curried_function<Function, BoundArgs...>> : public holder_base {
        curried_function<Function, BoundArgs...> f;

        static void invoke(holder_base *thiz, Res *res, Arguments ...args) {
            static_cast<function_holder *>(thiz)->f(res, args...);
        }

        static void destroy(holder_base *thiz) {
            delete static_cast<function_holder *>(thiz);
        }

    public:
        explicit function_holder(curried_function<Function, BoundArgs...> &&af)
            : holder_base(&invoke, &destroy), f(std::move(af)) { }
    };

public:
    bool is_valid() const { return bool(holder); }

    action_base() = default;
    action_base(const action_base &o) : holder(o.holder) { }
    action_base &operator =(const action_base &o) {
        holder = o.holder; return *this;
    }
    action_base(action_base &&o) : holder(std::move(o.holder)) { }
    action_base &operator =(action_base &&o) {
        holder = std::move(o.holder); return *this;
    }

  template <typename Function, typename HeldType = typename std::decay<Function>::type>
    explicit action_base(Function &&fun)
        : holder(new function_holder<Function>(std::forward<Function>(fun)), holder_destroyer())
    { }

  template <typename Function, typename... BoundArgs,
            typename HeldType = curried_function<Function, BoundArgs...>>
    explicit action_base(Function &&fun, BoundArgs &&...args)
        : holder(new function_holder<HeldType>(HeldType(std::forward<Function>(fun),
            std::forward<BoundArgs>(args)...)), holder_destroyer())
    { }

}; // class action_base

template <typename...> class action;

template <typename Res, typename... Arguments>
class action<Res (Arguments...)> : public action_base<Res (Arguments...)>
{
  template <typename... BoundArgs, std::size_t... ResultArgsIndices>
    action<Res (typename std::tuple_element<ResultArgsIndices, std::tuple<Arguments...>>::type...)>
    apply(details::index_range<ResultArgsIndices...>, BoundArgs &&...args) {
        return action<Res (typename std::tuple_element<ResultArgsIndices,
            std::tuple<Arguments...>>::type...)>(*this, std::forward<BoundArgs>(args)...);
    }

public:
    action() = default;
    action(const action &) = default;
    action &operator =(const action &) = default;
    action(action &&) = default;
    action &operator =(action &&) = default;

  template <typename Function, typename... BoundArgs>
    action(Function &&fun, BoundArgs &&...args)
        : action_base<Res (Arguments...)>(std::forward<Function>(fun), std::forward<BoundArgs>(args)...)
    { }

  template <typename... Args, typename = typename
      std::enable_if<std::is_constructible<std::tuple<Arguments...>, std::tuple<Args...>>::value>::type>
    inline Res operator()(Args &&...args) const {
        Res res; this->holder->invoke(std::addressof(res), std::forward<Args>(args)...);
        return res;
    }

  template <typename... Args, typename = typename
      std::enable_if<!std::is_constructible<std::tuple<Arguments...>, std::tuple<Args...>>::value>::type>
    auto operator()(Args &&...args) -> decltype(this->apply(
        std::declval<details::make_index_range<sizeof...(Args),
            sizeof...(Arguments) - sizeof...(Args)>>(), std::forward<Args>(args)...))
    {
        static_assert(sizeof...(Args) < sizeof...(Arguments),
            "Number of my arguments is greater than the number "
            "of parameters of the original action");
        return apply(details::make_index_range<sizeof...(Args),
            sizeof...(Arguments) - sizeof...(Args)>(), std::forward<Args>(args)...);
    }
};

template <typename... Arguments>
class action<void (Arguments...)> : public action_base<void (Arguments...)>
{
  template <typename... BoundArgs, std::size_t... ResultArgsIndices>
    action<void (typename std::tuple_element<ResultArgsIndices, std::tuple<Arguments...>>::type...)>
    apply(details::index_range<ResultArgsIndices...>, BoundArgs &&...args) {
        return action<void (typename std::tuple_element<ResultArgsIndices,
            std::tuple<Arguments...>>::type...)>(*this, std::forward<BoundArgs>(args)...);
    }

public:
    action() = default;
    action(const action &) = default;
    action &operator =(const action &) = default;
    action(action &&) = default;
    action &operator =(action &&) = default;

  template <typename Function, typename... BoundArgs>
    action(Function &&fun, BoundArgs &&...args)
        : action_base<void (Arguments...)>(std::forward<Function>(fun), std::forward<BoundArgs>(args)...)
    { }

  template <typename... Args, typename = typename
      std::enable_if<std::is_constructible<std::tuple<Arguments...>, std::tuple<Args...>>::value>::type>
    inline void operator()(Args &&...args) const { this->holder->invoke(nullptr, std::forward<Args>(args)...); }

  template <typename... Args, typename = typename
      std::enable_if<!std::is_constructible<std::tuple<Arguments...>, std::tuple<Args...>>::value>::type>
    auto operator()(Args &&...args) -> decltype(this->apply(
        std::declval<details::make_index_range<sizeof...(Args),
            sizeof...(Arguments) - sizeof...(Args)>>(), std::forward<Args>(args)...))
    {
        static_assert(sizeof...(Args) < sizeof...(Arguments),
            "Number of my arguments is greater than the number "
            "of parameters of the original action");
        return apply(details::make_index_range<sizeof...(Args),
            sizeof...(Arguments) - sizeof...(Args)>(), std::forward<Args>(args)...);
    }
};

namespace details {

template <typename Res, typename... Args, typename Functor,
          typename... BoundArgs, std::size_t... ResultArgsIndices>
  action<Res (typename std::tuple_element<ResultArgsIndices, std::tuple<Args...>>::type...)>
  make_action_aux(Functor &&f, details::index_range<ResultArgsIndices...>, BoundArgs &&...args) {
      return action<Res (typename std::tuple_element<ResultArgsIndices,
          std::tuple<Args...>>::type...)>(std::forward<Functor>(f), std::forward<BoundArgs>(args)...);
  }

} // namespace details

template <typename Res, typename... Args, typename... BoundArgs>
auto make_action(Res (f)(Args...), BoundArgs &&...args)
  -> decltype(details::make_action_aux<Res, Args...>(std::declval<decltype(f)>(),
    std::declval<details::make_index_range<sizeof...(BoundArgs),
        sizeof...(Args) - sizeof...(BoundArgs)>>(), std::forward<BoundArgs>(args)...))
{
    static_assert(sizeof...(BoundArgs) <= sizeof...(Args),
        "Number of my arguments is greater than the number "
        "of parameters of the original function");
    return details::make_action_aux<Res, Args...>(std::move(f),
        details::make_index_range<sizeof...(BoundArgs), sizeof...(Args) - sizeof...(BoundArgs)>(),
        std::forward<BoundArgs>(args)...);
}

template <typename Res, typename Tp, typename Up,
          typename... Args, typename... BoundArgs>
auto make_action(Res (Tp:: *f)(Args...), Up &&obj, BoundArgs &&...args)
  -> decltype(details::make_action_aux<Res, Args...>(
    std::declval<curried_function<decltype(std::mem_fn(f)), Up, BoundArgs...>>(),
    std::declval<details::make_index_range<sizeof...(BoundArgs), sizeof...(Args) - sizeof...(BoundArgs)>>()))
{
    static_assert(sizeof...(BoundArgs) <= sizeof...(Args),
        "Number of my arguments is greater than the number "
        "of parameters of the original function");
    return details::make_action_aux<Res, Args...>(
        curried_function<decltype(std::mem_fn(f)), Up, BoundArgs...>(std::mem_fn(f),
            std::forward<Up>(obj), std::forward<BoundArgs>(args)...),
        details::make_index_range<sizeof...(BoundArgs), sizeof...(Args) - sizeof...(BoundArgs)>());
}

template <typename Res, typename Tp, typename Up,
          typename... Args, typename... BoundArgs>
auto make_action(Res (Tp:: *f)(Args...) const, Up &&obj, BoundArgs &&...args)
  -> decltype(details::make_action_aux<Res, Args...>(
    std::declval<curried_function<decltype(std::mem_fn(f)), Up, BoundArgs...>>(),
    std::declval<details::make_index_range<sizeof...(BoundArgs), sizeof...(Args) - sizeof...(BoundArgs)>>()))
{
    static_assert(sizeof...(BoundArgs) <= sizeof...(Args),
        "Number of my arguments is greater than the number "
        "of parameters of the original function");
    return details::make_action_aux<Res, Args...>(
        curried_function<decltype(std::mem_fn(f)), Up, BoundArgs...>(std::mem_fn(f),
            std::forward<Up>(obj), std::forward<BoundArgs>(args)...),
        details::make_index_range<sizeof...(BoundArgs), sizeof...(Args) - sizeof...(BoundArgs)>());
}

} // namespace core

} // namespace ultra

#endif // ACTION_H
