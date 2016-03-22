#pragma once

#include <tuple>
#include <utility>
#include <core/function_traits.hpp>
#include <core/traits.hpp>
#include <core/tuple_meta.hpp>

namespace fc
{
template <class... ports>
struct mux_port;
template <class op>
struct unloaded_merge_port;
template <class op, class... ports>
struct loaded_merge_port;

struct default_tag {};
struct merge_tag {};
struct mux_tag {};

template <typename T>
struct port_traits
{
	using mux_category = default_tag;
};

template <class... ports>
struct port_traits<mux_port<ports...>>
{
	using mux_category = mux_tag;
};

template <class op>
struct port_traits<unloaded_merge_port<op>>
{
	using mux_category = merge_tag;
};

namespace detail
{
template <bool... vals>
constexpr bool any()
{
	bool values[] = {vals...};
	for (auto value : values)
		if (value)
			return true;
	return false;
}
template <typename... T>
constexpr bool always_false_fun(T...)
{
	return false;
}
} // namespace detail

template <class base>
struct node_aware;

struct many_to_many_tag {};

template <size_t left_ports, size_t right_ports>
struct mux_traits
{
	static_assert(
	    detail::always_false_fun(left_ports, right_ports),
	    "Only N->N, 1->N and N->1 connections possible between muxed ports. PS: don't try 1->1.");
};
template <size_t ports>
struct mux_traits<ports, ports>
{
	using connection_category = many_to_many_tag;
};

template <class... port_ts>
struct mux_port
{
	std::tuple<port_ts...> ports;

	template <class T>
	auto connect(T t, merge_tag)
	{
		static_assert(
		    detail::has_result_of_type<decltype(t.merge), decltype(std::declval<port_ts>()())...>(),
		    "The muxed ports can not be merged using the provided merge function.");
		static_assert(!detail::any<detail::is_derived_from<node_aware, port_ts>::value...>(),
		              "Merge port can not be used with node aware ports. See merge_node.");
		return loaded_merge_port<decltype(t.merge), port_ts...>{t.merge, std::move(ports)};
	}

	template <class other_mux>
	auto connect(other_mux&& other, mux_tag)
	{
		constexpr size_t this_ports = sizeof...(port_ts);
		constexpr size_t other_ports = std::tuple_size<decltype(other.ports)>::value;
		using connection_tag = typename mux_traits<this_ports, other_ports>::connection_category;
		return connect_mux(std::forward<other_mux>(other), connection_tag{});
	}

	template <class other_mux_port>
	auto connect_mux(other_mux_port&& other, many_to_many_tag)
	{
		auto pairwise_connect = [](auto&& l, auto&& r)
		{
			return std::forward<decltype(l)>(l) >> std::forward<decltype(r)>(r);
		};
		return fc::tuple::transform(std::move(ports), std::forward<other_mux_port>(other).ports,
		                            pairwise_connect);
	}

	template <class T>
	auto connect(T&& t, default_tag)
	{
		auto connect_to_copy = [&t](auto&& elem)
		{
			// t needs to be copied if it's not an lvalue ref, forward won't work because can't move
			// from smth multiple times.
			return std::forward<decltype(elem)>(elem) >> static_cast<T>(t);
		};
		return mux_from_tuple(fc::tuple::transform(std::move(ports), connect_to_copy));
	}

	template <class T>
	auto operator>>(T&& t)
	{
		using decayed = std::decay_t<T>;
		using tag = typename fc::port_traits<decayed>::mux_category;
		return this->connect(std::forward<T>(t), tag{});
	}
};

template <class... port_ts>
auto mux(port_ts&... ports)
{
	return mux_port<std::remove_const_t<port_ts>&...>{std::tie(ports...)};
}

template <class... conn_ts>
mux_port<conn_ts...> mux_from_tuple(std::tuple<conn_ts...> tuple_)
{
	return mux_port<conn_ts...>{std::move(tuple_)};
}

template <class merge_op>
struct unloaded_merge_port
{
	merge_op merge;
};

template <class merge_op, class... port_ts>
struct loaded_merge_port
{
	merge_op op;
	std::tuple<port_ts...> ports;

	auto operator()()
	{
		auto op = this->op;
		auto call_and_apply = [op](auto&&... src)
		{
			return op(std::forward<decltype(src)>(src)()...);
		};
		return tuple::invoke_function(call_and_apply, ports,
		                              std::make_index_sequence<sizeof...(port_ts)>{});
	}
};

template <class merge_op>
auto merge(merge_op op)
{
	return unloaded_merge_port<merge_op>{op};
}
} // namespace fc
