#ifndef _MYDTL_DATABASE_H_
#define _MYDTL_DATABASE_H_

#if defined(_MSC_VER)
#if _MSC_VER<1800
#error MYDTL need C++11 compiler
#endif //MSC
#elif __cplusplus<201103L
#error MYDTL need C++11 compiler
#endif //C++11

#include <stdint.h>
#include <string.h>
#include <type_traits>
#include <tuple>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "apply_tuple.h"

namespace qtl
{

struct null { };

struct blob_data
{
	void* data;
	size_t size;

	blob_data() : data(NULL), size(0) { }
	blob_data(void* d, size_t n) : data(d), size(n) { }
};

struct const_blob_data
{
	const void* data;
	size_t size;

	const_blob_data() : data(NULL), size(0) { }
	const_blob_data(const void* d, size_t n) : data(d), size(n) { }
};

const size_t blob_buffer_size=64*1024;

inline std::string& trim_string(std::string& str, const char* target)
{
	str.erase(0, str.find_first_not_of(target));
	str.erase(str.find_last_not_of(target)+1);
	return str;
}

template<typename T>
struct indicator
{
	typedef T data_type;

	data_type data;
	size_t length;
	bool is_null;
	bool is_truncated;

	indicator() 
		: is_null(false), is_truncated(false), length(0) { }
	explicit indicator(const data_type& src) 
		: is_null(false), is_truncated(false), length(0), data(src) { }
	explicit indicator(data_type&& src) 
		: is_null(false), is_truncated(false), length(0), data(std::move(src)) { }

	operator bool() const { return is_null; }
	operator data_type&() { return data; }
	operator const data_type&() const { return data; }
};

template<typename StringT>
struct bind_string_helper
{
	typedef StringT string_type;
	typedef typename string_type::value_type char_type;
	bind_string_helper(string_type&& value) : m_value(std::forward<string_type>(value)) { }
	bind_string_helper(const bind_string_helper& src)
		: m_value(std::forward<std::string>(src.m_value))
	{
	}
	bind_string_helper(bind_string_helper&& src)
		: m_value(std::forward<std::string>(src.m_value))
	{
	}
	bind_string_helper& operator=(const bind_string_helper& src)
	{
		if (this != &src)
		{
			m_value = std::forward<std::string>(src.m_value);
		}
		return *this;
	}
	bind_string_helper& operator=(bind_string_helper&& src)
	{
		if (this != &src)
		{
			m_value = std::forward<std::string>(src.m_value);
		}
		return *this;
	}

	void clear() { m_value.clear(); }
	char_type* alloc(size_t n) { m_value.resize(n); return (char_type*)m_value.data(); }
	void truncate(size_t n) { m_value.resize(n); }
	void assign(const char_type* str, size_t n) { m_value.assign(str, n); }
	const char_type* data() const { return m_value.data(); }
	size_t size() const { return m_value.size(); }
private:
	string_type&& m_value;
};

template<typename StringT>
inline bind_string_helper<typename std::decay<StringT>::type> bind_string(StringT&& value)
{
	typedef typename std::decay<StringT>::type string_type;
	return bind_string_helper<string_type>(std::forward<string_type>(value));
}

template<typename Command>
inline void bind_param(Command& command, size_t index, const std::string& param)
{
	command.bind_param(index, param.data(), param.size());
}

template<typename Command>
inline void bind_param(Command& command, size_t index, std::istream& param)
{
	command.bind_param(index, param);
}

template<typename Command, typename T>
inline void bind_param(Command& command, size_t index, const T& param)
{
	command.bind_param(index, param);
}

// The order of the overloaded functions 'bind_field' is very important
// The version with the most generic parameters is at the end

template<typename Command, size_t N>
inline void bind_field(Command& command, size_t index, char (&value)[N])
{
	command.bind_field(index, value, N);
}

template<typename Command, size_t N>
inline void bind_field(Command& command, size_t index, wchar_t (&value)[N])
{
	command.bind_field(index, value, N);
}

template<typename Command>
inline void bind_field(Command& command, size_t index, char* value, size_t length)
{
	command.bind_field(index, value, length);
}

template<typename Command>
inline void bind_field(Command& command, size_t index, wchar_t* value, size_t length)
{
	command.bind_field(index, value, length);
}

template<typename Command>
inline void bind_field(Command& command, size_t index, std::string&& value)
{
	command.bind_field(index, bind_string(std::forward<std::string>(value)));
}

template<typename Command>
inline void bind_field(Command& command, size_t index, std::wstring&& value)
{
	command.bind_field(index, bind_string(std::forward<std::wstring>(value)));
}

template<typename Command>
inline void bind_field(Command& command, size_t index, std::vector<char>&& value)
{
	command.bind_field(index, bind_string(std::forward<std::vector<char>>(value)));
}

template<typename Command>
inline void bind_field(Command& command, size_t index, std::vector<wchar_t>&& value)
{
	command.bind_field(index, bind_string(std::forward<std::vector<wchar_t>>(value)));
}

template<typename Command, typename T, typename=typename std::enable_if<!std::is_reference<T>::value>::type>
inline void bind_field(Command& command, size_t index, T&& value)
{
	command.bind_field(index, std::forward<T>(value));
}

template<typename Command, typename T>
inline void bind_field(Command& command, size_t index, T& value)
{
	bind_field(command, index, std::forward<T>(value));
}

template<typename FieldType, typename Command, typename BindType>
inline void bind_field(Command& command, size_t index, BindType& value)
{
	FieldType temp=FieldType();
	bind_field(command, index, temp);
	value=static_cast<BindType>(temp);
}

template<typename FieldType, typename Command, typename BindType, typename CastFun>
inline void bind_field(Command& command, size_t index, BindType& value, CastFun&& fun)
{
	FieldType temp=FieldType();
	bind_field(command, index, temp);
	fun(value, temp);
}

template<typename Command, typename T, typename=typename std::enable_if<!std::is_reference<T>::value>::type>
inline void bind_field(Command& command, const char* name, T&& value)
{
	size_t index=command.find_field(name);
	if(index==-1)
		value=T();
	else
		command.bind_field(index, std::forward<T>(value));
}

template<typename Command, typename T>
inline void bind_field(Command& command, const char* name, T& value)
{
	size_t index=command.find_field(name);
	if(index==-1)
		value=T();
	else
		bind_field(command, index, std::forward<T>(value));
}

template<typename FieldType, typename Command, typename BindType>
inline void bind_field(Command& command, const char* name, BindType& value)
{
	size_t index=command.find_field(name);
	if(index==-1)
		value=BindType();
	else
		bind_field<FieldType>(command, index, value);
}

template<typename FieldType, typename Command, typename BindType, typename CastFun>
inline void bind_field(Command& command, const char* name, BindType& value, CastFun&& fun)
{
	size_t index=command.find_field(name);
	if(index==-1)
		value=BindType();
	else
		bind_field<FieldType>(command, index, value, fun);
}

template<typename Command>
inline void bind_field(Command& command, const char* name, char* value, size_t length)
{
	size_t index=command.find_field(name);
	if(index==-1)
		value[0]='\0';
	else
		command.bind_field(index, value, length);
}

template<typename Command>
inline void bind_field(Command& command, const char* name, wchar_t* value, size_t length)
{
	size_t index=command.find_field(name);
	if(index==-1)
		value[0]='\0';
	else
		command.bind_field(index, value, length);
}

namespace detail
{

template<typename F, typename T>
struct apply_impl
{
private:
	typedef typename std::result_of<F(T)>::type raw_result_type;
	template<typename Ret, bool>
	struct impl {};
	template<typename Ret>
	struct impl<Ret, true>
	{
		typedef bool result_type;
		result_type operator()(F&& f, T&& v)
		{
			f(std::forward<T>(v));
			return true;
		}
	};
	template<typename Ret>
	struct impl<Ret, false>
	{
		typedef Ret result_type;
		result_type operator()(F&& f, T&& v)
		{
			return f(std::forward<T>(v));
		}
	};

public:
	typedef typename impl<raw_result_type, std::is_void<raw_result_type>::value>::result_type result_type;
	result_type operator()(F&& f, T&& v)
	{
		return impl<raw_result_type, std::is_void<raw_result_type>::value>()(std::forward<F>(f), std::forward<T>(v));
	}
};

template<typename F, typename... Types>
struct apply_impl<F, std::tuple<Types...>>
{
private:
	typedef typename std::remove_reference<F>::type fun_type;
	typedef std::tuple<Types...> arg_type;
	typedef typename std::result_of<F(Types...)>::type raw_result_type;
	template<typename Ret, bool>
	struct impl {};
	template<typename Ret>
	struct impl<Ret, true>
	{
		typedef bool result_type;
		result_type operator()(F&& f, arg_type&& v)
		{
			apply_tuple(std::forward<F>(f), std::forward<arg_type>(v));
			return true;
		}
	};
	template<typename Ret>
	struct impl<Ret, false>
	{
		typedef Ret result_type;
		result_type operator()(F&& f, arg_type&& v)
		{
			return apply_tuple(std::forward<F>(f), std::forward<arg_type>(v));
		}
	};

public:
	typedef typename impl<raw_result_type, std::is_void<raw_result_type>::value>::result_type result_type;
	result_type operator()(F&& f, arg_type&& v)
	{
		return impl<raw_result_type, std::is_void<raw_result_type>::value>()(std::forward<F>(f), std::forward<arg_type>(v));
	}
};

template<typename Type, typename R>
struct apply_impl<R (Type::*)(), Type>
{
private:
	typedef R (Type::*fun_type)();
	typedef R raw_result_type;
	template<typename Ret, bool>
	struct impl {};
	template<typename Ret>
	struct impl<Ret, true>
	{
		typedef bool result_type;
		result_type operator()(fun_type f, Type&& v)
		{
			(v.*f)();
			return true;
		}
	};
	template<typename Ret>
	struct impl<Ret, false>
	{
		typedef Ret result_type;
		result_type operator()(fun_type f, Type&& v)
		{
			return (v.*f)();
		}
	};

public:
	typedef typename impl<raw_result_type, std::is_void<raw_result_type>::value>::result_type result_type;
	result_type operator()(R (Type::*f)(), Type&& v)
	{
		return impl<raw_result_type, std::is_void<raw_result_type>::value>()(f, std::forward<Type>(v));
	}
};

template<typename Type, typename R>
struct apply_impl<R (Type::*)() const, Type>
{
private:
	typedef R (Type::*fun_type)() const;
	typedef R raw_result_type;
	template<typename Ret, bool>
	struct impl {};
	template<typename Ret>
	struct impl<Ret, true>
	{
		typedef bool result_type;
		result_type operator()(fun_type f, Type&& v)
		{
			(v.*f)();
			return true;
		}
	};
	template<typename Ret>
	struct impl<Ret, false>
	{
		typedef Ret result_type;
		result_type operator()(fun_type f, Type&& v)
		{
			return (v.*f)();
		}
	};

public:
	typedef typename impl<raw_result_type, std::is_void<raw_result_type>::value>::result_type result_type;
	result_type operator()(fun_type f, Type&& v)
	{
		return impl<raw_result_type, std::is_void<raw_result_type>::value>()(f, std::forward<Type>(v));
	}
};

template<typename F, typename T>
typename apply_impl<F, T>::result_type apply(F&& f, T&& v)
{
	return apply_impl<F, T>()(std::forward<F>(f),  std::forward<T>(v));
}

template<typename Command, size_t N, typename... Types>
struct bind_helper
{
public:
	explicit bind_helper(Command& command) : m_command(command) { }
	void operator()(const std::tuple<Types...>& params) const
	{
		bind_param(m_command, N-1, std::get<N-1>(params));
		(bind_helper<Command, N-1, Types...>(m_command))(params);
	}
	void operator()(std::tuple<Types...>&& params) const
	{
		typedef typename std::remove_reference<typename std::tuple_element<N-1, tuple_type>::type>::type param_type;
		bind_field(m_command, N-1, std::forward<param_type>(std::get<N-1>(std::forward<tuple_type>(params))));
		(bind_helper<Command, N-1, Types...>(m_command))(std::forward<tuple_type>(params));
	}
private:
	typedef std::tuple<Types...> tuple_type;
	Command& m_command;
};

template<typename Command, typename... Types>
struct bind_helper<Command, 1, Types...>
{
public:
	explicit bind_helper(Command& command) : m_command(command) { }
	void operator()(const std::tuple<Types...>& params) const
	{
		bind_param(m_command, 0, std::get<0>(params));
	}
	void operator()(std::tuple<Types...>&& params) const
	{
		typedef typename std::remove_reference<typename std::tuple_element<0, tuple_type>::type>::type param_type;
		bind_field(m_command, static_cast<size_t>(0), std::forward<param_type>(std::get<0>(std::forward<tuple_type>(params))));
	}
private:
	typedef std::tuple<Types...> tuple_type;
	Command& m_command;
};

template<typename Command, typename... Types>
struct bind_helper<Command, 0, Types...>
{
public:
	explicit bind_helper(Command& command) { }
	void operator()(const std::tuple<Types...>& params) const
	{
	}
	void operator()(std::tuple<Types...>&& params) const
	{
	}
};

#define QTL_ARGS_TUPLE(Arg, Others) \
	typename std::tuple<typename std::decay<Arg>::type, typename std::decay<Others>::type...>

template<typename Ret, typename Arg>
inline typename std::decay<Arg>::type make_values(Ret (*)(Arg))
{
	return typename std::decay<Arg>::type();
};

template<typename Ret, typename Arg, typename... Others>
inline auto make_values(Ret (*)(Arg, Others...)) -> QTL_ARGS_TUPLE(Arg, Others)
{
	return QTL_ARGS_TUPLE(Arg, Others)();
};

template<typename Type, typename Ret>
inline Type make_values(Ret (Type::*)())
{
	return Type();
};
template<typename Type, typename Ret>
inline Type make_values(Ret (Type::*)() const)
{
	return Type();
};

template<typename Type, typename Ret, typename... Args>
inline auto make_values(Ret (Type::*)(Args...)) -> QTL_ARGS_TUPLE(Type, Args)
{
	return QTL_ARGS_TUPLE(Type, Args)();
};
template<typename Type, typename Ret, typename... Args>
inline auto make_values(Ret (Type::*)(Args...) const) -> QTL_ARGS_TUPLE(Type, Args)
{
	return QTL_ARGS_TUPLE(Type, Args)();
};

template<typename Type, typename Ret, typename Arg>
inline typename std::decay<Arg>::type make_values_noclass(Ret (Type::*)(Arg))
{
	return typename std::decay<Arg>::type();
};
template<typename Type, typename Ret, typename Arg>
inline typename std::decay<Arg>::type make_values_noclass(Ret (Type::*)(Arg) const)
{
	return typename std::decay<Arg>::type();
};

template<typename Type, typename Ret, typename Arg, typename... Others>
inline auto make_values_noclass(Ret (Type::*)(Arg, Others...)) -> QTL_ARGS_TUPLE(Arg, Others)
{
	return QTL_ARGS_TUPLE(Arg, Others)();
};
template<typename Type, typename Ret, typename Arg, typename... Others>
inline auto make_values_noclass(Ret (Type::*)(Arg, Others...) const) -> QTL_ARGS_TUPLE(Arg, Others)
{
	return QTL_ARGS_TUPLE(Arg, Others)();
};

template<typename Functor, typename=typename std::enable_if<std::is_member_function_pointer<decltype(&Functor::operator())>::value>::type>
inline auto make_values(const Functor&) 
-> decltype(make_values_noclass(&Functor::operator()))
{
	return make_values_noclass(&Functor::operator());
}

template<typename Command, typename ValueProc>
inline void fetch_command(Command& command, ValueProc&& proc)
{
	auto values=make_values(proc);
	typedef decltype(values) values_type;
	while(command.fetch(std::forward<values_type>(values)))
	{
		if(!detail::apply(std::forward<ValueProc>(proc), std::forward<values_type>(values))) 
			break;
	}
}

template<typename Command, typename ValueProc, typename... OtherProc>
inline void fetch_command(Command& command, ValueProc&& proc, OtherProc&&... other)
{
	fetch_command(command, std::forward<ValueProc>(proc));
	if(command.next_result())
	{
		fetch_command(command, std::forward<OtherProc>(other)...);
	}
}

}

template<typename Command, typename T>
struct params_binder
{
	inline void operator()(Command& command, const T& param) const
	{
		qtl::bind_param(command, 0, param);
	}
};

template<typename Command, typename... Types>
struct params_binder<Command, std::tuple<Types...>>
{
	void operator()(Command& command, const std::tuple<Types...>& params) const
	{
		(detail::bind_helper<Command, std::tuple_size<std::tuple<Types...>>::value, Types...>(command))(params);
	}
};

template<typename Command, typename Type1, typename Type2>
struct params_binder<Command, std::pair<Type1, Type2>>
{
	void operator()(Command& command, std::pair<Type1, Type2>&& values) const
	{
		qtl::bind_param(command, 0, std::forward<Type1>(values.first));
		qtl::bind_param(command, 1, std::forward<Type2>(values.second));
	}
};

template<typename Command, typename T>
inline void bind_params(Command& command, const T& param)
{
	params_binder<Command, T> binder;
	binder(command, param);
}

template<typename Command, typename T>
struct record_binder
{
	inline void operator()(Command& command, T&& value) const
	{
		bind_field(command, static_cast<size_t>(0), std::forward<typename std::remove_reference<T>::type>(value));
	}
};

template<typename Command, typename... Types>
struct record_binder<Command, std::tuple<Types...>>
{
	void operator()(Command& command, std::tuple<Types...>&& values) const
	{
		(detail::bind_helper<Command, std::tuple_size<std::tuple<Types...>>::value, Types...>(command))
			(std::forward<std::tuple<Types...>>(values));
	}
};

template<typename Command, typename Type1, typename Type2>
struct record_binder<Command, std::pair<Type1, Type2>>
{
	void operator()(Command& command, std::pair<Type1, Type2>&& values) const
	{
		bind_field(command, static_cast<size_t>(0), std::forward<Type1>(values.first));
		bind_field(command, static_cast<size_t>(1), std::forward<Type2>(values.second));
	}
};

template<typename T, typename Tag>
struct record_with_tag : public T
{
};

template<typename Command, typename T>
inline void bind_record(Command& command, T&& value)
{
	record_binder<Command, T> binder;
	binder(command, std::forward<T>(value));
}

template<typename Command, typename Record>
class query_iterator final : public std::iterator<std::forward_iterator_tag, Record>
{
public:
	explicit query_iterator(Command& command) 
		: m_command(command) { }
	Record* operator->() const { return m_record.get(); }
	Record& operator*() const { return *m_record; }

	query_iterator& operator++()
	{
		if(!m_record)
			m_record=std::make_shared<Record>();
		if(m_record.unique())
		{
			if(!m_command.fetch(std::forward<Record>(*m_record)))
				m_record.reset();
		}
		else
		{
			std::shared_ptr<Record> record=std::make_shared<Record>();
			if(m_command.fetch(std::forward<Record>(*record)))
				m_record=record;
			else
				m_record.reset();
		}
		return *this;
	}
	query_iterator operator++(int)
	{
		query_iterator temp=*this;
		m_record=std::make_shared<Record>();
		if(!m_command.fetch(std::forward<Record>(*m_record)))
			m_record.reset();
		return temp;
	}

	bool operator ==(const query_iterator& rhs)
	{
		return &this->m_command==&rhs.m_command &&
			this->m_record==rhs.m_record;
	}
	bool operator !=(const query_iterator& rhs)
	{
		return !(*this==rhs);
	}

private:
	Command& m_command;
	std::shared_ptr<Record> m_record;
};

template<typename Command, typename Record>
class query_result final
{
public:
	typedef typename query_iterator<Command, Record>::value_type value_type;
	typedef typename query_iterator<Command, Record>::pointer pointer;
	typedef typename query_iterator<Command, Record>::reference reference;
	typedef query_iterator<Command, Record> iterator;

	explicit query_result(Command&& command) : m_command(std::move(command)) { }
	query_result(query_result&& src) : m_command(std::move(src.m_command)) { }
	query_result& operator=(query_result&& src)
	{
		if(this!=&src)
			m_command=std::move(src.m_command);
		return *this;
	}

	template<typename Params>
	iterator begin(const Params& params)
	{
		query_iterator<Command, Record> it(m_command);
		++it;
		return it;
	}
	iterator begin()
	{
		return begin(std::make_tuple());
	}

	iterator end()
	{
		return query_iterator<Command, Record>(m_command);
	}

private:
	Command m_command;
};

template<typename T, class Command>
class base_database
{
public:
	template<typename Params>
	void execute(const char* query_text, size_t text_length, const Params& params, uint64_t* affected=NULL)
	{
		T* pThis=static_cast<T*>(this);
		Command command=pThis->open_command(query_text, text_length);
		command.execute(params);
		if(affected) *affected=command.affetced_rows();
		command.close();
	}
	template<typename Params>
	void execute(const char* query_text, const Params& params, uint64_t* affected=NULL)
	{
		return execute(query_text, strlen(query_text), params, affected);
	}
	template<typename Params>
	void execute(const std::string& query_text, const Params& params, uint64_t* affected=NULL)
	{
		return execute(query_text.data(), query_text.length(), params, affected);
	}

	template<typename... Params>
	void execute_direct(const char* query_text, size_t text_length, uint64_t* affected, const Params&... params)
	{
		execute(query_text, text_length, std::forward_as_tuple(params...), affected);
	}
	template<typename... Params>
	void execute_direct(const char* query_text, uint64_t* affected, const Params&... params)
	{
		execute(query_text, std::forward_as_tuple(params...), affected);
	}
	template<typename... Params>
	void execute_direct(const std::string& query_text, uint64_t* affected, const Params&... params)
	{
		execute(query_text, std::forward_as_tuple(params...), affected);
	}

	template<typename Params>
	uint64_t insert(const char* query_text, size_t text_length, const Params& params)
	{
		uint64_t id=0;
		T* pThis=static_cast<T*>(this);
		Command command=pThis->open_command(query_text, text_length);
		command.execute(params);
		if(command.affetced_rows()>0)
			id=command.insert_id();
		return id;
	}

	template<typename Params>
	uint64_t insert(const char* query_text, const Params& params)
	{
		return insert(query_text, strlen(query_text), params);
	}

	template<typename Params>
	uint64_t insert(const std::string& query_text, const Params& params)
	{
		return insert(query_text.data(), query_text.length(), params);
	}

	template<typename... Params>
	uint64_t insert_direct(const char* query_text, size_t text_length, const Params&... params)
	{
		return insert(query_text, text_length, std::forward_as_tuple(params...));
	}

	template<typename... Params>
	uint64_t insert_direct(const char* query_text, const Params&... params)
	{
		return insert(query_text, strlen(query_text), std::forward_as_tuple(params...));
	}

	template<typename... Params>
	uint64_t insert_direct(const std::string& query_text, const Params&... params)
	{
		return insert(query_text.data(), query_text.length(), std::forward_as_tuple(params...));
	}

	template<typename Record, typename Params>
	query_result<Command, Record> result(const char* query_text, size_t text_length, const Params& params)
	{
		T* pThis=static_cast<T*>(this);
		Command command=pThis->open_command(query_text, text_length);
		command.execute(params);
		return query_result<Command, Record>(std::move(command));
	}
	template<typename Record, typename Params>
	query_result<Command, Record> result(const char* query_text, const Params& params)
	{
		return result<Record, Params>(query_text, strlen(query_text), params);
	}
	template<typename Record, typename Params>
	query_result<Command, Record> result(const std::string& query_text, const Params& params)
	{
		return result<Record, Params>(query_text.data(), query_text.length(), params);
	}
	template<typename Record>
	query_result<Command, Record> result(const char* query_text, size_t text_length)
	{
		return result<Record>(query_text, text_length, std::make_tuple());
	}
	template<typename Record>
	query_result<Command, Record> result(const char* query_text)
	{
		return result<Record>(query_text, strlen(query_text), std::make_tuple());
	}
	template<typename Record>
	query_result<Command, Record> result(const std::string& query_text)
	{
		return result<Record>(query_text.data(), query_text.length(), std::make_tuple());
	}

	template<typename Params, typename Values, typename ValueProc>
	void query_explicit(const char* query_text, size_t text_length, const Params& params, Values&& values, ValueProc&& proc)
	{
		T* pThis=static_cast<T*>(this);
		Command command=pThis->open_command(query_text, text_length);
		command.execute(params);
		while(command.fetch(std::forward<Values>(values)))
		{
			if(!detail::apply(std::forward<ValueProc>(proc), std::forward<Values>(values))) break;
		}
		command.close();
	}

	template<typename Params, typename Values, typename ValueProc>
	void query_explicit(const char* query_text, const Params& params, Values&& values, ValueProc&& proc)
	{
		query_explicit(query_text, strlen(query_text), params, std::forward<Values>(values), std::forward<ValueProc>(proc));
	}
	template<typename Params, typename Values, typename ValueProc>
	void query_explicit(const std::string& query_text, const Params& params, Values&& values, ValueProc&& proc)
	{
		query_explicit(query_text.data(), query_text.size(), params, std::forward<Values>(values), std::forward<ValueProc>(proc));
	}
	template<typename Values, typename ValueProc>
	void query_explicit(const char* query_text, size_t text_length, Values&& values, ValueProc&& proc)
	{
		query_explicit(query_text, text_length, std::make_tuple(), std::forward<Values>(values), std::forward<ValueProc>(proc));
	}
	template<typename Values, typename ValueProc>
	void query_explicit(const char* query_text, Values&& values, ValueProc&& proc)
	{
		query_explicit(query_text, strlen(query_text), std::make_tuple(), std::forward<Values>(values), std::forward<ValueProc>(proc));
	}
	template<typename Values, typename ValueProc>
	void query_explicit(const std::string& query_text, Values&& values, ValueProc&& proc)
	{
		query_explicit(query_text, std::make_tuple(), std::forward<Values>(values), std::forward<ValueProc>(proc));
	}

	template<typename Params, typename ValueProc>
	void query(const char* query_text, size_t text_length, const Params& params, ValueProc&& proc)
	{
		query_explicit(query_text, text_length, params, detail::make_values(proc),  std::forward<ValueProc>(proc));
	}
	template<typename Params, typename ValueProc>
	void query(const char* query_text, const Params& params, ValueProc&& proc)
	{
		query_explicit(query_text, params, detail::make_values(proc),  std::forward<ValueProc>(proc));
	}
	template<typename Params, typename ValueProc>
	void query(const std::string& query_text, const Params& params, ValueProc&& proc)
	{
		query_explicit(query_text, params, detail::make_values(proc),  std::forward<ValueProc>(proc));
	}
	template<typename ValueProc>
	void query(const char* query_text, size_t text_length, ValueProc&& proc)
	{
		query_explicit(query_text, text_length, detail::make_values(proc),  std::forward<ValueProc>(proc));
	}
	template<typename ValueProc>
	void query(const char* query_text, ValueProc&& proc)
	{
		query_explicit(query_text, detail::make_values(proc),  std::forward<ValueProc>(proc));
	}
	template<typename ValueProc>
	void query(const std::string& query_text, ValueProc&& proc)
	{
		query_explicit(query_text, detail::make_values(proc), std::forward<ValueProc>(proc));
	}

	template<typename Params, typename... ValueProc>
	void query_multi_with_params(const char* query_text, size_t text_length, const Params& params, ValueProc&&... proc)
	{
		T* pThis=static_cast<T*>(this);
		Command command=pThis->open_command(query_text, text_length);
		command.execute(params);
		detail::fetch_command(command, std::forward<ValueProc>(proc)...);
		command.close();
	}
	template<typename Params, typename... ValueProc>
	void query_multi_with_params(const char* query_text, const Params& params, ValueProc&&... proc)
	{
		query_multi_with_params(query_text, strlen(query_text), params, std::forward<ValueProc>(proc)...);
	}
	template<typename Params, typename... ValueProc>
	void query_multi_with_params(const std::string& query_text, const Params& params, ValueProc&&... proc)
	{
		query_multi_with_params(query_text.data(), query_text.size(), params, std::forward<ValueProc>(proc)...);
	}
	template<typename... ValueProc>
	void query_multi(const char* query_text, size_t text_length, ValueProc&&... proc)
	{
		query_multi_with_params<std::tuple<>, ValueProc...>(query_text, text_length, std::make_tuple(), std::forward<ValueProc>(proc)...);
	}
	template<typename... ValueProc>
	void query_multi(const char* query_text, ValueProc&&... proc)
	{
		query_multi_with_params<std::tuple<>, ValueProc...>(query_text, strlen(query_text), std::make_tuple(), std::forward<ValueProc>(proc)...);
	}
	template<typename... ValueProc>
	void query_multi(const std::string& query_text, ValueProc&&... proc)
	{
		query_multi_with_params<std::tuple<>, ValueProc...>(query_text.data(), query_text.size(), std::make_tuple(), std::forward<ValueProc>(proc)...);
	}

	template<typename Params, typename Values>
	void query_first(const char* query_text, size_t text_length, const Params& params, Values&& values)
	{
		query_explicit(query_text, text_length, params, std::forward<Values>(values), first_record());
	}

	template<typename Params, typename Values>
	void query_first(const char* query_text, const Params& params, Values&& values)
	{
		query_explicit(query_text, strlen(query_text), params, std::forward<Values>(values), first_record());
	}

	template<typename Params, typename Values>
	void query_first(const std::string& query_text, const Params& params, Values&& values)
	{
		query_explicit(query_text, params, values, first_record());
	}

	template<typename Values>
	void query_first(const char* query_text, size_t text_length, Values&& values)
	{
		query_explicit(query_text, text_length, std::make_tuple(), std::forward<Values>(values), first_record());
	}

	template<typename Values>
	void query_first(const char* query_text, Values&& values)
	{
		query_explicit(query_text, strlen(query_text), std::make_tuple(), std::forward<Values>(values), first_record());
	}

	template<typename Values>
	void query_first(const std::string& query_text, Values&& values)
	{
		query_explicit(query_text, std::make_tuple(), std::forward<Values>(values), first_record());
	}

	template<typename... Values>
	void query_first_direct(const char* query_text, size_t text_length, Values&... values)
	{
		query_first(query_text, text_length, std::tie(values...));
	}

	template<typename... Values>
	void query_first_direct(const char* query_text, Values&... values)
	{
		query_first(query_text, std::tie(values...));
	}

	template<typename... Values>
	void query_first_direct(const std::string& query_text, Values&... values)
	{
		query_first(query_text, std::tie(values...));
	}

protected:
	struct nothing 
	{
		template<typename... Values> bool operator()(Values&&...) const {  return true; }
	};
	struct first_record 
	{
		template<typename... Values> bool operator()(Values&&...) const {  return false; }
	};
};

template<typename Database>
struct transaction
{
	transaction(Database& db) : m_db(db), m_commited(true)
	{
		begin();
	}
	~transaction()
	{
		rollback();
	}
	void begin()
	{
		if(m_commited)
		{
			m_db.begin_transaction();
			m_commited=false;
		}
	}
	void rollback()
	{
		if(!m_commited)
		{
			m_db.rollback();
			m_commited=true;
		}
	}
	void commit()
	{
		if(!m_commited)
		{
			m_db.commit();
			m_commited=true;
		}
	}
private:
	bool m_commited;
	Database& m_db;
};

template<typename Command, typename Params, typename... Others>
inline void execute(Command& command, uint64_t* affected, const Params& params)
{
	command.reset();
	command.execute(params);
	if(affected) *affected+=command.affetced_rows();
}

template<typename Command, typename Params, typename... Others>
inline void execute(Command& command, uint64_t* affected, const Params& params, const Others&... others)
{
	execute(command, affected, params);
	execute(command, affected, others...);
}

}

#endif //_MYDTL_DATABASE_H_
