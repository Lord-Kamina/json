#ifndef _NLOHMANN_JSON
#define _NLOHMANN_JSON

#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*!
- ObjectType trick from http://stackoverflow.com/a/9860911
*/
/*
template<typename C, typename=void>
struct container_resizable : std::false_type {};
template<typename C>
struct container_resizable<C, decltype(C().resize(0))> : std::true_type {};
*/

/*!
@see https://github.com/nlohmann
*/
namespace nlohmann
{

/*!
@brief JSON

@tparam ObjectType         type for JSON objects
                           (@c std::map by default)
@tparam ArrayType          type for JSON arrays
                           (@c std::vector by default)
@tparam StringType         type for JSON strings and object keys
                           (@c std::string by default)
@tparam BooleanType        type for JSON booleans
                           (@c bool by default)
@tparam NumberIntegerType  type for JSON integer numbers
                           (@c int64_t by default)
@tparam NumberFloatType    type for JSON floating-point numbers
                           (@c double by default)
*/
template <
    template<typename U, typename V, typename... Args> class ObjectType = std::map,
    //template<typename... Args> class ArrayType = std::vector,
    template<typename U, typename... Args> class ArrayType = std::vector,
    class StringType = std::string,
    class BooleanType = bool,
    class NumberIntegerType = int64_t,
    class NumberFloatType = double
    >
class basic_json
{
  public:
    /////////////////////
    // container types //
    /////////////////////

    class iterator;
    class const_iterator;

    /// the type of elements in a basic_json container
    using value_type = basic_json;
    /// the type of an element reference
    using reference = basic_json&;
    /// the type of an element const reference
    using const_reference = const basic_json&;
    /// the type of an element pointer
    using pointer = basic_json*;
    /// the type of an element const pointer
    using const_pointer = const basic_json*;
    /// a type to represent differences between iterators
    using difference_type = std::ptrdiff_t;
    /// a type to represent container sizes
    using size_type = std::size_t;
    /// an iterator for a basic_json container
    using iterator = basic_json::iterator;
    /// a const iterator for a basic_json container
    using const_iterator = basic_json::const_iterator;
    // a reverse iterator for a basic_json container
    using reverse_iterator = std::reverse_iterator<iterator>;
    /// a const reverse iterator for a basic_json container
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    ///////////////////////////
    // JSON value data types //
    ///////////////////////////

    /// a type for an object
    using object_t = ObjectType<StringType, basic_json>;
    /// a type for an array
    using array_t = ArrayType<basic_json>;
    /// a type for a string
    using string_t = StringType;
    /// a type for a boolean
    using boolean_t = BooleanType;
    /// a type for a number (integer)
    using number_integer_t = NumberIntegerType;
    /// a type for a number (floating-point)
    using number_float_t = NumberFloatType;
    /// a type for list initialization
    using list_init_t = std::initializer_list<basic_json>;


    ////////////////////////
    // JSON value storage //
    ////////////////////////

    /// a JSON value
    union json_value
    {
        /// object (stored with pointer to save storage)
        object_t* object;
        /// array (stored with pointer to save storage)
        array_t* array;
        /// string (stored with pointer to save storage)
        string_t* string;
        /// bolean
        boolean_t boolean;
        /// number (integer)
        number_integer_t number_integer;
        /// number (floating-point)
        number_float_t number_float;

        /// default constructor (for null values)
        json_value() = default;
        /// constructor for objects
        json_value(object_t* v) : object(v) {}
        /// constructor for arrays
        json_value(array_t* v) : array(v) {}
        /// constructor for strings
        json_value(string_t* v) : string(v) {}
        /// constructor for booleans
        json_value(boolean_t v) : boolean(v) {}
        /// constructor for numbers (integer)
        json_value(number_integer_t v) : number_integer(v) {}
        /// constructor for numbers (floating-point)
        json_value(number_float_t v) : number_float(v) {}
    };


    /////////////////////////////////
    // JSON value type enumeration //
    /////////////////////////////////

    /// JSON value type enumeration
    enum class value_t : uint8_t
    {
        /// null value
        null,
        /// object (unordered set of name/value pairs)
        object,
        /// array (ordered collection of values)
        array,
        /// string value
        string,
        /// boolean value
        boolean,
        /// number value (integer)
        number_integer,
        /// number value (floating-point)
        number_float
    };


    //////////////////
    // constructors //
    //////////////////

    /// create an empty value with a given type
    inline basic_json(const value_t value)
        : m_type(value)
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                break;
            }

            case (value_t::object):
            {
                m_value.object = new object_t();
                break;
            }

            case (value_t::array):
            {
                m_value.array = new array_t();
                break;
            }

            case (value_t::string):
            {
                m_value.string = new string_t("");
                break;
            }

            case (value_t::boolean):
            {
                m_value.boolean = boolean_t(false);
                break;
            }

            case (value_t::number_integer):
            {
                m_value.number_integer = number_integer_t(0);
                break;
            }

            case (value_t::number_float):
            {
                m_value.number_float = number_float_t(0.0);
                break;
            }
        }
    }

    /// create a null object (implicitly)
    inline basic_json() noexcept
        : m_type(value_t::null)
    {}

    /// create a null object (explicitly)
    inline basic_json(std::nullptr_t) noexcept
        : m_type(value_t::null)
    {}

    /// create an object (explicit)
    inline basic_json(const object_t& value)
        : m_type(value_t::object), m_value(new object_t(value))
    {}

    /// create an object (implicit)
    template <class V, typename
              std::enable_if<
                  std::is_constructible<string_t, typename V::key_type>::value and
                  std::is_constructible<basic_json, typename V::mapped_type>::value, int>::type
              = 0>
    inline basic_json(const V& value)
        : m_type(value_t::object), m_value(new object_t(value.begin(), value.end()))
    {}

    /// create an array (explicit)
    inline basic_json(const array_t& value)
        : m_type(value_t::array), m_value(new array_t(value))
    {}

    /// create an array (implicit)
    template <class V, typename
              std::enable_if<
                  not std::is_same<V, basic_json::iterator>::value and
                  not std::is_same<V, basic_json::const_iterator>::value and
                  not std::is_same<V, basic_json::reverse_iterator>::value and
                  not std::is_same<V, basic_json::const_reverse_iterator>::value and
                  std::is_constructible<basic_json, typename V::value_type>::value, int>::type
              = 0>
    inline basic_json(const V& value)
        : m_type(value_t::array), m_value(new array_t(value.begin(), value.end()))
    {}

    /// create a string (explicit)
    inline basic_json(const string_t& value)
        : m_type(value_t::string), m_value(new string_t(value))
    {}

    /// create a string (explicit)
    inline basic_json(const typename string_t::value_type* value)
        : m_type(value_t::string), m_value(new string_t(value))
    {}

    /// create a string (implicit)
    template <class V, typename
              std::enable_if<
                  std::is_constructible<string_t, V>::value, int>::type
              = 0>
    inline basic_json(const V& value)
        : basic_json(string_t(value))
    {}

    /// create a boolean (explicit)
    inline basic_json(boolean_t value)
        : m_type(value_t::boolean), m_value(value)
    {}

    /// create an integer number (explicit)
    inline basic_json(const number_integer_t& value)
        : m_type(value_t::number_integer), m_value(value)
    {}

    /// create an integer number (implicit)
    template<typename T, typename
             std::enable_if<
                 std::is_constructible<number_integer_t, T>::value and
                 std::numeric_limits<T>::is_integer, T>::type
             = 0>
    inline basic_json(const T value) noexcept
        : m_type(value_t::number_integer), m_value(number_integer_t(value))
    {}

    /// create a floating-point number (explicit)
    inline basic_json(const number_float_t& value)
        : m_type(value_t::number_float), m_value(value)
    {}

    /// create a floating-point number (implicit)
    template<typename T, typename = typename
             std::enable_if<
                 std::is_constructible<number_float_t, T>::value and
                 std::is_floating_point<T>::value>::type
             >
    inline basic_json(const T value) noexcept
        : m_type(value_t::number_float), m_value(number_float_t(value))
    {}

    /// create a container (array or object) from an initializer list
    inline basic_json(list_init_t l, bool type_deduction = true, value_t manual_type = value_t::array)
    {
        // the initializer list could describe an object
        bool is_object = true;

        // check if each element is an array with two elements whose first element
        // is a string
        for (const auto& element : l)
        {
            if ((element.m_final and element.m_type == value_t::array)
                    or (element.m_type != value_t::array or element.size() != 2
                        or element[0].m_type != value_t::string))
            {
                // we found an element that makes it impossible to use the
                // initializer list as object
                is_object = false;
                break;
            }
        }

        // adjust type if type deduction is not wanted
        if (not type_deduction)
        {
            // mark this object's type as final
            m_final = true;

            // if array is wanted, do not create an object though possible
            if (manual_type == value_t::array)
            {
                is_object = false;
            }

            // if object is wanted but impossible, throw an exception
            if (manual_type == value_t::object and not is_object)
            {
                throw std::logic_error("cannot create JSON object from initializer list");
            }
        }

        if (is_object)
        {
            // the initializer list is a list of pairs -> create object
            m_type = value_t::object;
            m_value = new object_t();
            for (auto& element : l)
            {
                m_value.object->emplace(std::move(*(element[0].m_value.string)), std::move(element[1]));
            }
        }
        else
        {
            // the initializer list describes an array -> create array
            m_type = value_t::array;
            m_value = new array_t(std::move(l));
        }
    }

    /// explicitly create an array from an initializer list
    inline static basic_json array(list_init_t l = list_init_t())
    {
        return basic_json(l, false, value_t::array);
    }

    /// explicitly create an object from an initializer list
    inline static basic_json object(list_init_t l = list_init_t())
    {
        return basic_json(l, false, value_t::object);
    }

    ///////////////////////////////////////
    // other constructors and destructor //
    ///////////////////////////////////////

    /// copy constructor
    inline basic_json(const basic_json& other)
        : m_type(other.m_type)
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                break;
            }
            case (value_t::object):
            {
                m_value.object = new object_t(*other.m_value.object);
                break;
            }
            case (value_t::array):
            {
                m_value.array = new array_t(*other.m_value.array);
                break;
            }
            case (value_t::string):
            {
                m_value.string = new string_t(*other.m_value.string);
                break;
            }
            case (value_t::boolean):
            {
                m_value.boolean = other.m_value.boolean;
                break;
            }
            case (value_t::number_integer):
            {
                m_value.number_integer = other.m_value.number_integer;
                break;
            }
            case (value_t::number_float):
            {
                m_value.number_float = other.m_value.number_float;
                break;
            }
        }
    }

    /// move constructor
    inline basic_json(basic_json&& other) noexcept
        : m_type(std::move(other.m_type)),
          m_value(std::move(other.m_value))
    {
        // invalidate payload
        other.m_type = value_t::null;
        other.m_value = {};
    }

    /// copy assignment
    inline reference operator=(basic_json other) noexcept
    {
        std::swap(m_type, other.m_type);
        std::swap(m_value, other.m_value);
        return *this;
    }

    /// destructor
    inline ~basic_json() noexcept
    {
        switch (m_type)
        {
            case (value_t::object):
            {
                delete m_value.object;
                m_value.object = nullptr;
                break;
            }

            case (value_t::array):
            {
                delete m_value.array;
                m_value.array = nullptr;
                break;
            }

            case (value_t::string):
            {
                delete m_value.string;
                m_value.string = nullptr;
                break;
            }

            default:
            {
                // all other types need no specific destructor
                break;
            }
        }
    }


  public:
    ///////////////////////
    // object inspection //
    ///////////////////////

    /*!
    @brief serialization

    Serialization function for JSON objects. The function tries to mimick Python's
    @p json.dumps() function, and currently supports its @p indent parameter.

    @param indent  if indent is nonnegative, then array elements and object members
                   will be pretty-printed with that indent level. An indent level
                   of 0 will only insert newlines. -1 (the default) selects the
                   most compact representation

    @see https://docs.python.org/2/library/json.html#json.dump
    */
    inline string_t dump(int indent = -1) const noexcept
    {
        if (indent >= 0)
        {
            return dump(true, static_cast<unsigned int>(indent));
        }
        else
        {
            return dump(false, 0);
        }
    }

    /// return the type of the object (explicit)
    inline value_t type() const noexcept
    {
        return m_type;
    }

    /// return the type of the object (implicit)
    operator value_t() const noexcept
    {
        return m_type;
    }


    //////////////////////
    // value conversion //
    //////////////////////

    /// get an object (explicit)
    template <class T, typename
              std::enable_if<
                  std::is_constructible<string_t, typename T::key_type>::value and
                  std::is_constructible<basic_json, typename T::mapped_type>::value, int>::type
              = 0>
    inline T get() const
    {
        switch (m_type)
        {
            case (value_t::object):
                return T(m_value.object->begin(), m_value.object->end());
            default:
                throw std::logic_error("cannot cast " + type_name() + " to " + typeid(T).name());
        }
    }

    /// get an array (explicit)
    template <class T, typename
              std::enable_if<
                  not std::is_same<T, string_t>::value and
                  std::is_constructible<basic_json, typename T::value_type>::value, int>::type
              = 0>
    inline T get() const
    {
        switch (m_type)
        {
            case (value_t::array):
                return T(m_value.array->begin(), m_value.array->end());
            default:
                throw std::logic_error("cannot cast " + type_name() + " to " + typeid(T).name());
        }
    }

    /// get a string (explicit)
    template <typename T, typename
              std::enable_if<
                  std::is_constructible<T, string_t>::value, int>::type
              = 0>
    inline T get() const
    {
        switch (m_type)
        {
            case (value_t::string):
                return *m_value.string;
            default:
                throw std::logic_error("cannot cast " + type_name() + " to " + typeid(T).name());
        }
    }

    /// get a boolean (explicit)
    template <typename T, typename
              std::enable_if<
                  std::is_same<boolean_t, T>::value, int>::type
              = 0>
    inline T get() const
    {
        switch (m_type)
        {
            case (value_t::boolean):
                return m_value.boolean;
            default:
                throw std::logic_error("cannot cast " + type_name() + " to " + typeid(T).name());
        }
    }

    /// get a number (explicit)
    template<typename T, typename
             std::enable_if<
                 not std::is_same<boolean_t, T>::value and
                 std::is_arithmetic<T>::value, int>::type
             = 0>
    inline T get() const
    {
        switch (m_type)
        {
            case (value_t::number_integer):
                return static_cast<T>(m_value.number_integer);
            case (value_t::number_float):
                return static_cast<T>(m_value.number_float);
            default:
                throw std::logic_error("cannot cast " + type_name() + " to " + typeid(T).name());
        }
    }

    /// get a value (implicit)
    template<typename T>
    inline operator T() const
    {
        return get<T>();
    }


    ////////////////////
    // element access //
    ////////////////////

    /// access specified element with bounds checking
    inline reference at(size_type pos)
    {
        // at only works for arrays
        if (m_type != value_t::array)
        {
            throw std::runtime_error("cannot use at with " + type_name());
        }

        return m_value.array->at(pos);
    }

    /// access specified element with bounds checking
    inline const_reference at(size_type pos) const
    {
        // at only works for arrays
        if (m_type != value_t::array)
        {
            throw std::runtime_error("cannot use at with " + type_name());
        }

        return m_value.array->at(pos);
    }

    /// access specified element
    inline reference operator[](size_type pos)
    {
        // at only works for arrays
        if (m_type != value_t::array)
        {
            throw std::runtime_error("cannot use [] with " + type_name());
        }

        return m_value.array->operator[](pos);
    }

    /// access specified element
    inline const_reference operator[](size_type pos) const
    {
        // at only works for arrays
        if (m_type != value_t::array)
        {
            throw std::runtime_error("cannot use [] with " + type_name());
        }

        return m_value.array->operator[](pos);
    }

    /// access specified element with bounds checking
    inline reference at(const typename object_t::key_type& key)
    {
        // at only works for objects
        if (m_type != value_t::object)
        {
            throw std::runtime_error("cannot use at with " + type_name());
        }

        return m_value.object->at(key);
    }

    /// access specified element with bounds checking
    inline const_reference at(const typename object_t::key_type& key) const
    {
        // at only works for objects
        if (m_type != value_t::object)
        {
            throw std::runtime_error("cannot use at with " + type_name());
        }

        return m_value.object->at(key);
    }

    /// access specified element
    inline reference operator[](const typename object_t::key_type& key)
    {
        // at only works for objects
        if (m_type != value_t::object)
        {
            throw std::runtime_error("cannot use [] with " + type_name());
        }

        return m_value.object->operator[](key);
    }

    /// access specified element (needed for clang)
    template<typename T, size_t n>
    inline reference operator[](const T (&key)[n])
    {
        // at only works for objects
        if (m_type != value_t::object)
        {
            throw std::runtime_error("cannot use [] with " + type_name());
        }

        return m_value.object->operator[](key);
    }

    /// access specified element
    inline reference operator[](typename object_t::key_type&& key)
    {
        // at only works for objects
        if (m_type != value_t::object)
        {
            throw std::runtime_error("cannot use [] with " + type_name());
        }

        return m_value.object->operator[](std::move(key));
    }


    /// find an element in an object
    inline iterator find(typename object_t::key_type key)
    {
        auto result = end();

        if (m_type == value_t::object)
        {
            result.m_it.object_iterator = m_value.object->find(key);
        }

        return result;
    }

    /// find an element in an object
    inline const_iterator find(typename object_t::key_type key) const
    {
        auto result = cend();

        if (m_type == value_t::object)
        {
            result.m_it.object_iterator = m_value.object->find(key);
        }

        return result;
    }


    ///////////////
    // iterators //
    ///////////////

    /// returns an iterator to the beginning of the container
    inline iterator begin() noexcept
    {
        iterator result(this);
        result.set_begin();
        return result;
    }

    /// returns a const iterator to the beginning of the container
    inline const_iterator begin() const noexcept
    {
        const_iterator result(this);
        result.set_begin();
        return result;
    }

    /// returns a const iterator to the beginning of the container
    inline const_iterator cbegin() const noexcept
    {
        const_iterator result(this);
        result.set_begin();
        return result;
    }

    /// returns an iterator to the end of the container
    inline iterator end() noexcept
    {
        iterator result(this);
        result.set_end();
        return result;
    }

    /// returns a const iterator to the end of the container
    inline const_iterator end() const noexcept
    {
        const_iterator result(this);
        result.set_end();
        return result;
    }

    /// returns a const iterator to the end of the container
    inline const_iterator cend() const noexcept
    {
        const_iterator result(this);
        result.set_end();
        return result;
    }

    /// returns a reverse iterator to the end of the container
    inline reverse_iterator rbegin() const noexcept
    {
        reverse_iterator result(this);
        result.set_end();
        return result;
    }

    /// returns a reverse iterator to the beginning of the container
    inline reverse_iterator rend() const noexcept
    {
        reverse_iterator result(this);
        result.set_begin();
        return result;
    }

    /// returns a const reverse iterator to the end of the container
    inline const_reverse_iterator crbegin() const noexcept
    {
        const_reverse_iterator result(this);
        result.set_end();
        return result;
    }

    /// returns a const reverse iterator to the beginning of the container
    inline const_reverse_iterator crend() const noexcept
    {
        const_reverse_iterator result(this);
        result.set_begin();
        return result;
    }


    //////////////
    // capacity //
    //////////////

    /// checks whether the container is empty
    inline bool empty() const noexcept
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                return true;
            }

            case (value_t::array):
            {
                return m_value.array->empty();
            }

            case (value_t::object):
            {
                return m_value.object->empty();
            }

            default:
            {
                // all other types are nonempty
                return false;
            }
        }
    }

    /// returns the number of elements
    inline size_type size() const noexcept
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                return 0;
            }

            case (value_t::array):
            {
                return m_value.array->size();
            }

            case (value_t::object):
            {
                return m_value.object->size();
            }

            default:
            {
                // all other types have size 1
                return 1;
            }
        }
    }

    /// returns the maximum possible number of elements
    inline size_type max_size() const noexcept
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                return 0;
            }

            case (value_t::array):
            {
                return m_value.array->max_size();
            }

            case (value_t::object):
            {
                return m_value.object->max_size();
            }

            default:
            {
                // all other types have max_size 1
                return 1;
            }
        }
    }


    ///////////////
    // modifiers //
    ///////////////

    /// clears the contents
    inline void clear() noexcept
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                break;
            }

            case (value_t::number_integer):
            {
                m_value.number_integer = 0;
                break;
            }

            case (value_t::number_float):
            {
                m_value.number_float = 0.0;
                break;
            }

            case (value_t::boolean):
            {
                m_value.boolean = false;
                break;
            }

            case (value_t::string):
            {
                m_value.string->clear();
                break;
            }

            case (value_t::array):
            {
                m_value.array->clear();
                break;
            }

            case (value_t::object):
            {
                m_value.object->clear();
                break;
            }
        }
    }

    /// add an object to an array
    inline void push_back(basic_json&& value)
    {
        // push_back only works for null objects or arrays
        if (not(m_type == value_t::null or m_type == value_t::array))
        {
            throw std::runtime_error("cannot add element to " + type_name());
        }

        // transform null object into an array
        if (m_type == value_t::null)
        {
            m_type = value_t::array;
            m_value.array = new array_t;
        }

        // add element to array (move semantics)
        m_value.array->push_back(std::move(value));
        // invalidate object
        value.m_type = value_t::null;
    }

    /// add an object to an array
    inline void push_back(const basic_json& value)
    {
        // push_back only works for null objects or arrays
        if (not(m_type == value_t::null or m_type == value_t::array))
        {
            throw std::runtime_error("cannot add element to " + type_name());
        }

        // transform null object into an array
        if (m_type == value_t::null)
        {
            m_type = value_t::array;
            m_value.array = new array_t;
        }

        // add element to array
        m_value.array->push_back(value);
    }

    /*
    /// add an object to an array
    inline reference operator+=(const basic_json& value)
    {
        push_back(value);
        return *this;
    }
    */

    /// add constructible objects to an array
    template<class T, typename std::enable_if<std::is_constructible<basic_json, T>::value>::type = 0>
    inline void push_back(const T& value)
    {
        assert(false); // not sure if function will ever be called
        push_back(basic_json(value));
    }

    /*
    /// add constructible objects to an array
    template<class T, typename std::enable_if<std::is_constructible<basic_json, T>::value>::type = 0>
    inline reference operator+=(const T& value)
    {
        push_back(basic_json(value));
        return *this;
    }
    */

    /// add an object to an object
    inline void push_back(const typename object_t::value_type& value)
    {
        // push_back only works for null objects or objects
        if (not(m_type == value_t::null or m_type == value_t::object))
        {
            throw std::runtime_error("cannot add element to " + type_name());
        }

        // transform null object into an object
        if (m_type == value_t::null)
        {
            m_type = value_t::object;
            m_value.object = new object_t;
        }

        // add element to array
        m_value.object->insert(value);
    }

    /*
    /// add an object to an object
    inline reference operator+=(const typename object_t::value_type& value)
    {
        push_back(value);
        return operator[](value.first);
    }
    */

    /// constructs element in-place at the end of an array
    template <typename T, typename
              std::enable_if<
                  std::is_constructible<basic_json, T>::value, int>::type
              = 0>
    inline void emplace_back(T && arg)
    {
        // push_back only works for null objects or arrays
        if (not(m_type == value_t::null or m_type == value_t::array))
        {
            throw std::runtime_error("cannot add element to " + type_name());
        }

        // transform null object into an array
        if (m_type == value_t::null)
        {
            m_type = value_t::array;
            m_value.array = new array_t;
        }

        // add element to array
        m_value.array->emplace_back(std::forward<T>(arg));
    }

    /// swaps the contents
    inline void swap(reference other) noexcept
    {
        std::swap(m_type, other.m_type);
        std::swap(m_value, other.m_value);
    }

    /// swaps the contents
    inline void swap(array_t& other)
    {
        // swap only works for arrays
        if (m_type != value_t::array)
        {
            throw std::runtime_error("cannot use swap with " + type_name());
        }

        // swap arrays
        std::swap(*(m_value.array), other);
    }

    /// swaps the contents
    inline void swap(object_t& other)
    {
        // swap only works for objects
        if (m_type != value_t::object)
        {
            throw std::runtime_error("cannot use swap with " + type_name());
        }

        // swap arrays
        std::swap(*(m_value.object), other);
    }

    /// swaps the contents
    inline void swap(string_t& other)
    {
        // swap only works for strings
        if (m_type != value_t::string)
        {
            throw std::runtime_error("cannot use swap with " + type_name());
        }

        // swap arrays
        std::swap(*(m_value.string), other);
    }


    //////////////////////////////////////////
    // lexicographical comparison operators //
    //////////////////////////////////////////

    /// comparison: equal
    friend bool operator==(const_reference lhs, const_reference rhs)
    {
        switch (lhs.type())
        {
            case (value_t::array):
            {
                if (rhs.type() == value_t::array)
                {
                    return *lhs.m_value.array == *rhs.m_value.array;
                }
                break;
            }
            case (value_t::object):
            {
                if (rhs.type() == value_t::object)
                {
                    return *lhs.m_value.object == *rhs.m_value.object;
                }
                break;
            }
            case (value_t::null):
            {
                if (rhs.type() == value_t::null)
                {
                    return true;
                }
                break;
            }
            case (value_t::string):
            {
                if (rhs.type() == value_t::string)
                {
                    return *lhs.m_value.string == *rhs.m_value.string;
                }
                break;
            }
            case (value_t::boolean):
            {
                if (rhs.type() == value_t::boolean)
                {
                    return lhs.m_value.boolean == rhs.m_value.boolean;
                }
                break;
            }
            case (value_t::number_integer):
            {
                if (rhs.type() == value_t::number_integer)
                {
                    return lhs.m_value.number_integer == rhs.m_value.number_integer;
                }
                if (rhs.type() == value_t::number_float)
                {
                    return lhs.m_value.number_integer == static_cast<number_integer_t>(rhs.m_value.number_float);
                }
                break;
            }
            case (value_t::number_float):
            {
                if (rhs.type() == value_t::number_integer)
                {
                    return lhs.m_value.number_float == static_cast<number_float_t>(rhs.m_value.number_integer);
                }
                if (rhs.type() == value_t::number_float)
                {
                    return lhs.m_value.number_float == rhs.m_value.number_float;
                }
                break;
            }
        }

        return false;
    }

    /// comparison: not equal
    friend bool operator!=(const_reference lhs, const_reference rhs)
    {
        return not (lhs == rhs);
    }

    /// comparison: less than
    friend bool operator<(const_reference lhs, const_reference rhs)
    {
        switch (lhs.type())
        {
            case (value_t::array):
            {
                if (rhs.type() == value_t::array)
                {
                    return *lhs.m_value.array < *rhs.m_value.array;
                }
                break;
            }
            case (value_t::object):
            {
                if (rhs.type() == value_t::object)
                {
                    return *lhs.m_value.object < *rhs.m_value.object;
                }
                break;
            }
            case (value_t::null):
            {
                if (rhs.type() == value_t::null)
                {
                    return false;
                }
                break;
            }
            case (value_t::string):
            {
                if (rhs.type() == value_t::string)
                {
                    return *lhs.m_value.string < *rhs.m_value.string;
                }
                break;
            }
            case (value_t::boolean):
            {
                if (rhs.type() == value_t::boolean)
                {
                    return lhs.m_value.boolean < rhs.m_value.boolean;
                }
                break;
            }
            case (value_t::number_integer):
            {
                if (rhs.type() == value_t::number_integer)
                {
                    return lhs.m_value.number_integer < rhs.m_value.number_integer;
                }
                if (rhs.type() == value_t::number_float)
                {
                    return lhs.m_value.number_integer < static_cast<number_integer_t>(rhs.m_value.number_float);
                }
                break;
            }
            case (value_t::number_float):
            {
                if (rhs.type() == value_t::number_integer)
                {
                    return lhs.m_value.number_float < static_cast<number_float_t>(rhs.m_value.number_integer);
                }
                if (rhs.type() == value_t::number_float)
                {
                    return lhs.m_value.number_float < rhs.m_value.number_float;
                }
                break;
            }
        }

        return false;
    }

    /// comparison: less than or equal
    friend bool operator<=(const_reference lhs, const_reference rhs)
    {
        return not (rhs < lhs);
    }

    /// comparison: greater than
    friend bool operator>(const_reference lhs, const_reference rhs)
    {
        return not (lhs <= rhs);
    }

    /// comparison: greater than or equal
    friend bool operator>=(const_reference lhs, const_reference rhs)
    {
        return not (lhs < rhs);
    }


    ///////////////////
    // serialization //
    ///////////////////

    /// serialize to stream
    friend std::ostream& operator<<(std::ostream& o, const basic_json& j)
    {
        o << j.dump();
        return o;
    }

    /// serialize to stream
    friend std::ostream& operator>>(const basic_json& j, std::ostream& o)
    {
        o << j.dump();
        return o;
    }


    /////////////////////
    // deserialization //
    /////////////////////

    /// deserialize from string
    static basic_json parse(const std::string& s)
    {
        return parser(s).parse();
    }

    /// deserialize from stream
    friend std::istream& operator>>(std::istream& i, basic_json& j)
    {
        j = parser(i).parse();
        return i;
    }

    /// deserialize from stream
    friend std::istream& operator<<(basic_json& j, std::istream& i)
    {
        j = parser(i).parse();
        return i;
    }


  private:
    ///////////////////////////
    // convenience functions //
    ///////////////////////////

    /// return the type as string
    inline string_t type_name() const noexcept
    {
        switch (m_type)
        {
            case (value_t::null):
            {
                return "null";
            }
            case (value_t::object):
            {
                return "object";
            }
            case (value_t::array):
            {
                return "array";
            }
            case (value_t::string):
            {
                return "string";
            }
            case (value_t::boolean):
            {
                return "boolean";
            }
            case (value_t::number_integer):
            case (value_t::number_float):
            {
                return "number";
            }
        }
    }

    /*!
    @brief escape a string

    Escape a string by replacing certain special characters by a sequence of an
    escape character (backslash) and another character and other control
    characters by a sequence of "\u" followed by a four-digit hex
    representation.

    @param s  the string to escape
    @return escaped string
    */
    static string_t escape_string(const string_t& s) noexcept
    {
        // create a result string of at least the size than s
        string_t result;
        result.reserve(s.size());

        for (const auto c : s)
        {
            switch (c)
            {
                // quotation mark (0x22)
                case '"':
                {
                    result += "\\\"";
                    break;
                }
                // reverse solidus (0x5c)
                case '\\':
                {
                    result += "\\\\";
                    break;
                }
                // backspace (0x08)
                case '\b':
                {
                    result += "\\b";
                    break;
                }
                // formfeed (0x0c)
                case '\f':
                {
                    result += "\\f";
                    break;
                }
                // newline (0x0a)
                case '\n':
                {
                    result += "\\n";
                    break;
                }
                // carriage return (0x0d)
                case '\r':
                {
                    result += "\\r";
                    break;
                }
                // horizontal tab (0x09)
                case '\t':
                {
                    result += "\\t";
                    break;
                }

                default:
                {
                    if (c <= 0x1f)
                    {
                        // control characters (everything between 0x00 and 0x1f)
                        // -> create four-digit hex representation
                        std::stringstream ss;
                        ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c);
                        result += ss.str();
                    }
                    else
                    {
                        // all other characters are added as-is
                        result.append(1, c);
                    }
                    break;
                }
            }
        }

        return result;
    }


    /*!
    @brief internal implementation of the serialization function

    This function is called by the public member function dump and organizes
    the serializaion internally. The indentation level is propagated as
    additional parameter. In case of arrays and objects, the function is called
    recursively. Note that

    - strings and object keys are escaped using escape_string()
    - numbers are converted to a string before output using std::to_string()

    @param prettyPrint    whether the output shall be pretty-printed
    @param indentStep     the indent level
    @param currentIndent  the current indent level (only used internally)
    */
    inline string_t dump(const bool prettyPrint, const unsigned int indentStep,
                         unsigned int currentIndent = 0) const noexcept
    {
        // helper function to return whitespace as indentation
        const auto indent = [prettyPrint, &currentIndent]()
        {
            return prettyPrint ? string_t(currentIndent, ' ') : string_t();
        };

        switch (m_type)
        {
            case (value_t::null):
            {
                return "null";
            }

            case (value_t::object):
            {
                if (m_value.object->empty())
                {
                    return "{}";
                }

                string_t result = "{";

                // increase indentation
                if (prettyPrint)
                {
                    currentIndent += indentStep;
                    result += "\n";
                }

                for (auto i = m_value.object->cbegin(); i != m_value.object->cend(); ++i)
                {
                    if (i != m_value.object->cbegin())
                    {
                        result += prettyPrint ? ",\n" : ",";
                    }
                    result += indent() + "\"" + escape_string(i->first) + "\":" + (prettyPrint ? " " : "")
                              + i->second.dump(prettyPrint, indentStep, currentIndent);
                }

                // decrease indentation
                if (prettyPrint)
                {
                    currentIndent -= indentStep;
                    result += "\n";
                }

                return result + indent() + "}";
            }

            case (value_t::array):
            {
                if (m_value.array->empty())
                {
                    return "[]";
                }

                string_t result = "[";

                // increase indentation
                if (prettyPrint)
                {
                    currentIndent += indentStep;
                    result += "\n";
                }

                for (auto i = m_value.array->cbegin(); i != m_value.array->cend(); ++i)
                {
                    if (i != m_value.array->cbegin())
                    {
                        result += prettyPrint ? ",\n" : ",";
                    }
                    result += indent() + i->dump(prettyPrint, indentStep, currentIndent);
                }

                // decrease indentation
                if (prettyPrint)
                {
                    currentIndent -= indentStep;
                    result += "\n";
                }

                return result + indent() + "]";
            }

            case (value_t::string):
            {
                return string_t("\"") + escape_string(*m_value.string) + "\"";
            }

            case (value_t::boolean):
            {
                return m_value.boolean ? "true" : "false";
            }

            case (value_t::number_integer):
            {
                return std::to_string(m_value.number_integer);
            }

            case (value_t::number_float):
            {
                return std::to_string(m_value.number_float);
            }
        }
    }


  private:
    //////////////////////
    // member variables //
    //////////////////////

    /// the type of the current element
    value_t m_type = value_t::null;

    /// whether the type of JSON object may change later
    bool m_final = false;

    /// the value of the current element
    json_value m_value = {};


  public:
    ///////////////
    // iterators //
    ///////////////

    /// a bidirectional iterator for the basic_json class
    class iterator : public std::iterator<std::bidirectional_iterator_tag, basic_json>
    {
      public:
        /// the type of the values when the iterator is dereferenced
        using value_type = basic_json::value_type;
        /// a type to represent differences between iterators
        using difference_type = basic_json::difference_type;
        /// defines a pointer to the type iterated over (value_type)
        using pointer = basic_json::pointer;
        /// defines a reference to the type iterated over (value_type)
        using reference = basic_json::reference;
        /// the category of the iterator
        using iterator_category = std::bidirectional_iterator_tag;

        /// values of a generic iterator type of non-container JSON values
        enum class generic_iterator_value
        {
            /// the iterator was not initialized
            uninitialized,
            /// the iterator points to the only value
            begin,
            /// the iterator points past the only value
            end,
            /// the iterator points to an invalid value
            invalid
        };

        /// an iterator value
        union internal_iterator
        {
            /// iterator for JSON objects
            typename object_t::iterator object_iterator;
            /// iterator for JSON arrays
            typename array_t::iterator array_iterator;
            /// generic iteraotr for all other value types
            generic_iterator_value generic_iterator;

            /// default constructor
            internal_iterator() : generic_iterator(generic_iterator_value::uninitialized) {}
            /// constructor for object iterators
            internal_iterator(typename object_t::iterator v) : object_iterator(v) {}
            /// constructor for array iterators
            internal_iterator(typename array_t::iterator v) : array_iterator(v) {}
            /// constructor for generic iterators
            internal_iterator(generic_iterator_value v) : generic_iterator(v) {}
        };

        /// constructor for a given JSON instance
        inline iterator(pointer object) : m_object(object)
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator = typename object_t::iterator();
                    break;
                }
                case (basic_json::value_t::array):
                {
                    m_it.array_iterator = typename array_t::iterator();
                    break;
                }
                default:
                {
                    m_it.generic_iterator = generic_iterator_value::uninitialized;
                    break;
                }
            }
        }

        /// copy assignment
        inline iterator& operator=(const iterator& other) noexcept
        {
            m_object = other.m_object;
            m_it = other.m_it;
            return *this;
        }

        /// set the iterator to the first value
        inline void set_begin() noexcept
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator = m_object->m_value.object->begin();
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator = m_object->m_value.array->begin();
                    break;
                }

                case (basic_json::value_t::null):
                {
                    // set to end so begin()==end() is true: null is empty
                    m_it.generic_iterator = generic_iterator_value::end;
                    break;
                }

                default:
                {
                    m_it.generic_iterator = generic_iterator_value::begin;
                    break;
                }
            }
        }

        /// set the iterator past the last value
        inline void set_end() noexcept
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator = m_object->m_value.object->end();
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator = m_object->m_value.array->end();
                    break;
                }

                default:
                {
                    m_it.generic_iterator = generic_iterator_value::end;
                    break;
                }
            }
        }

        /// return a reference to the value pointed to by the iterator
        inline reference operator*() const
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    return m_it.object_iterator->second;
                }

                case (basic_json::value_t::array):
                {
                    return *m_it.array_iterator;
                }

                case (basic_json::value_t::null):
                {
                    throw std::out_of_range("cannot get value");
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        return *m_object;
                    }
                    else
                    {
                        throw std::out_of_range("cannot get value");
                    }
                }
            }
        }

        /// dereference the iterator
        inline pointer operator->() const
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    return &(m_it.object_iterator->second);
                }

                case (basic_json::value_t::array):
                {
                    return &*m_it.array_iterator;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        return m_object;
                    }
                    else
                    {
                        throw std::out_of_range("cannot get value");
                    }
                }
            }
        }

        /// post-increment (it++)
        inline iterator operator++(int)
        {
            iterator result = *this;

            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator++;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator++;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        m_it.generic_iterator = generic_iterator_value::end;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return result;
        }

        /// pre-increment (++it)
        inline iterator& operator++()
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    ++m_it.object_iterator;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    ++m_it.array_iterator;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        m_it.generic_iterator = generic_iterator_value::end;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return *this;
        }

        /// post-decrement (it--)
        inline iterator operator--(int)
        {
            iterator result = *this;

            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator--;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator--;
                    break;
                }

                case (basic_json::value_t::null):
                {
                    m_it.generic_iterator = generic_iterator_value::invalid;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::end)
                    {
                        m_it.generic_iterator = generic_iterator_value::begin;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return result;
        }

        /// pre-decrement (--it)
        inline iterator& operator--()
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    --m_it.object_iterator;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    --m_it.array_iterator;
                    break;
                }

                case (basic_json::value_t::null):
                {
                    m_it.generic_iterator = generic_iterator_value::invalid;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::end)
                    {
                        m_it.generic_iterator = generic_iterator_value::begin;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return *this;
        }

        /// comparison: equal
        inline bool operator==(const iterator& other) const
        {
            if (m_object != other.m_object or m_object->m_type != other.m_object->m_type)
            {
                return false;
            }

            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    return (m_it.object_iterator == other.m_it.object_iterator);
                }

                case (basic_json::value_t::array):
                {
                    return (m_it.array_iterator == other.m_it.array_iterator);
                }

                default:
                {
                    return (m_it.generic_iterator == other.m_it.generic_iterator);
                }
            }
        }

        /// comparison: not equal
        inline bool operator!=(const iterator& other) const
        {
            return not operator==(other);
        }

      private:
        /// associated JSON instance
        pointer m_object = nullptr;
        /// the actual iterator of the associated instance
        internal_iterator m_it;
    };

    /// a const bidirectional iterator for the basic_json class
    class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const basic_json>
    {
      public:
        /// the type of the values when the iterator is dereferenced
        using value_type = basic_json::value_type;
        /// a type to represent differences between iterators
        using difference_type = basic_json::difference_type;
        /// defines a pointer to the type iterated over (value_type)
        using pointer = basic_json::const_pointer;
        /// defines a reference to the type iterated over (value_type)
        using reference = basic_json::const_reference;
        /// the category of the iterator
        using iterator_category = std::bidirectional_iterator_tag;

        /// values of a generic iterator type of non-container JSON values
        enum class generic_iterator_value
        {
            /// the iterator was not initialized
            uninitialized,
            /// the iterator points to the only value
            begin,
            /// the iterator points past the only value
            end,
            /// the iterator points to an invalid value
            invalid
        };

        /// an iterator value
        union internal_const_iterator
        {
            /// iterator for JSON objects
            typename object_t::const_iterator object_iterator;
            /// iterator for JSON arrays
            typename array_t::const_iterator array_iterator;
            /// generic iteraotr for all other value types
            generic_iterator_value generic_iterator;

            /// default constructor
            internal_const_iterator() : generic_iterator(generic_iterator_value::uninitialized) {}
            /// constructor for object iterators
            internal_const_iterator(typename object_t::iterator v) : object_iterator(v) {}
            /// constructor for array iterators
            internal_const_iterator(typename array_t::iterator v) : array_iterator(v) {}
            /// constructor for generic iterators
            internal_const_iterator(generic_iterator_value v) : generic_iterator(v) {}
        };

        /// constructor for a given JSON instance
        inline const_iterator(pointer object) : m_object(object)
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator = typename object_t::const_iterator();
                    break;
                }
                case (basic_json::value_t::array):
                {
                    m_it.array_iterator = typename array_t::const_iterator();
                    break;
                }
                default:
                {
                    m_it.generic_iterator = generic_iterator_value::uninitialized;
                    break;
                }
            }
        }

        /// copy constructor given a nonconst iterator
        inline const_iterator(const iterator& other) : m_object(other.m_object), m_it(other.m_it)
        {}

        /// copy assignment
        inline const_iterator operator=(const const_iterator& other) noexcept
        {
            m_object = other.m_object;
            m_it = other.m_it;
            return *this;
        }

        /// set the iterator to the first value
        inline void set_begin() noexcept
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator = m_object->m_value.object->cbegin();
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator = m_object->m_value.array->cbegin();
                    break;
                }

                case (basic_json::value_t::null):
                {
                    // set to end so begin()==end() is true: null is empty
                    m_it.generic_iterator = generic_iterator_value::end;
                    break;
                }

                default:
                {
                    m_it.generic_iterator = generic_iterator_value::begin;
                    break;
                }
            }
        }

        /// set the iterator past the last value
        inline void set_end() noexcept
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator = m_object->m_value.object->cend();
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator = m_object->m_value.array->cend();
                    break;
                }

                default:
                {
                    m_it.generic_iterator = generic_iterator_value::end;
                    break;
                }
            }
        }

        /// return a reference to the value pointed to by the iterator
        inline reference operator*() const
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    return m_it.object_iterator->second;
                }

                case (basic_json::value_t::array):
                {
                    return *m_it.array_iterator;
                }

                case (basic_json::value_t::null):
                {
                    throw std::out_of_range("cannot get value");
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        return *m_object;
                    }
                    else
                    {
                        throw std::out_of_range("cannot get value");
                    }
                }
            }
        }

        /// dereference the iterator
        inline pointer operator->() const
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    return &(m_it.object_iterator->second);
                }

                case (basic_json::value_t::array):
                {
                    return &*m_it.array_iterator;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        return m_object;
                    }
                    else
                    {
                        throw std::out_of_range("cannot get value");
                    }
                }
            }
        }

        /// post-increment (it++)
        inline const_iterator operator++(int)
        {
            const_iterator result = *this;

            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator++;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator++;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        m_it.generic_iterator = generic_iterator_value::end;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return result;
        }

        /// pre-increment (++it)
        inline const_iterator& operator++()
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    ++m_it.object_iterator;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    ++m_it.array_iterator;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::begin)
                    {
                        m_it.generic_iterator = generic_iterator_value::end;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return *this;
        }

        /// post-decrement (it--)
        inline const_iterator operator--(int)
        {
            iterator result = *this;

            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    m_it.object_iterator--;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    m_it.array_iterator--;
                    break;
                }

                case (basic_json::value_t::null):
                {
                    m_it.generic_iterator = generic_iterator_value::invalid;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::end)
                    {
                        m_it.generic_iterator = generic_iterator_value::begin;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return result;
        }

        /// pre-decrement (--it)
        inline const_iterator& operator--()
        {
            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    --m_it.object_iterator;
                    break;
                }

                case (basic_json::value_t::array):
                {
                    --m_it.array_iterator;
                    break;
                }

                case (basic_json::value_t::null):
                {
                    m_it.generic_iterator = generic_iterator_value::invalid;
                    break;
                }

                default:
                {
                    if (m_it.generic_iterator == generic_iterator_value::end)
                    {
                        m_it.generic_iterator = generic_iterator_value::begin;
                    }
                    else
                    {
                        m_it.generic_iterator = generic_iterator_value::invalid;
                    }
                    break;
                }
            }

            return *this;
        }

        /// comparison: equal
        inline bool operator==(const const_iterator& other) const
        {
            if (m_object != other.m_object or m_object->m_type != other.m_object->m_type)
            {
                return false;
            }

            switch (m_object->m_type)
            {
                case (basic_json::value_t::object):
                {
                    return (m_it.object_iterator == other.m_it.object_iterator);
                }

                case (basic_json::value_t::array):
                {
                    return (m_it.array_iterator == other.m_it.array_iterator);
                }

                default:
                {
                    return (m_it.generic_iterator == other.m_it.generic_iterator);
                }
            }
        }

        /// comparison: not equal
        inline bool operator!=(const const_iterator& other) const
        {
            return not operator==(other);
        }

      private:
        /// associated JSON instance
        pointer m_object = nullptr;
        /// the actual iterator of the associated instance
        internal_const_iterator m_it;
    };


  private:
    ////////////
    // parser //
    ////////////

    class parser
    {
      private:
        /// token types for the parser
        enum class token_type
        {
            uninitialized,
            literal_true,
            literal_false,
            literal_null,
            value_string,
            value_number,
            begin_array,
            begin_object,
            end_array,
            end_object,
            name_separator,
            value_separator,
            parse_error
        };

        /// the type of a lexer character
        using lexer_char_t = unsigned char;

      public:
        /// constructor for strings
        inline parser(const std::string& s) : buffer(s)
        {
            // set buffer for RE2C
            buffer_re2c = reinterpret_cast<const lexer_char_t*>(buffer.c_str());
            // set a pointer past the end of the buffer
            buffer_re2c_limit = buffer_re2c + buffer.size();
            // read first token
            get_token();
        }

        /// a parser reading from an input stream
        inline parser(std::istream& _is)
        {
            while (_is)
            {
                std::string input_line;
                std::getline(_is, input_line);
                buffer += input_line;
            }

            // set buffer for RE2C
            buffer_re2c = reinterpret_cast<const lexer_char_t*>(buffer.c_str());
            // set a pointer past the end of the buffer
            buffer_re2c_limit = buffer_re2c + buffer.size();
            // read first token
            get_token();
        }

        inline basic_json parse()
        {
            switch (last_token)
            {
                case (token_type::begin_object):
                {
                    // explicitly set result to object to cope with {}
                    basic_json result(value_t::object);

                    // read next token
                    get_token();

                    // closing } -> we are done
                    if (last_token == token_type::end_object)
                    {
                        return result;
                    }

                    // otherwise: parse key-value pairs
                    do
                    {
                        // store key
                        expect_new(token_type::value_string);
                        const auto key = get_string();

                        // parse separator (:)
                        get_token();
                        expect_new(token_type::name_separator);

                        // parse value
                        get_token();
                        result[key] = parse();

                        // read next character
                        get_token();
                    }
                    while (last_token == token_type::value_separator
                            and get_token() == last_token);

                    // closing }
                    expect_new(token_type::end_object);

                    return result;
                }

                case (token_type::begin_array):
                {
                    // explicitly set result to object to cope with []
                    basic_json result(value_t::array);

                    // read next token
                    get_token();

                    // closing ] -> we are done
                    if (last_token == token_type::end_array)
                    {
                        return result;
                    }

                    // otherwise: parse values
                    do
                    {
                        // parse value
                        result.push_back(parse());

                        // read next character
                        get_token();
                    }
                    while (last_token == token_type::value_separator
                            and get_token() == last_token);

                    // closing ]
                    expect_new(token_type::end_array);

                    return result;
                }

                case (token_type::literal_null):
                {
                    return basic_json(nullptr);
                }

                case (token_type::value_string):
                {
                    return basic_json(get_string());
                }

                case (token_type::literal_true):
                {
                    return basic_json(true);
                }

                case (token_type::literal_false):
                {
                    return basic_json(false);
                }

                case (token_type::value_number):
                {
                    // The pointer current_re2c points to the beginning of the
                    // parsed number. We pass this pointer to std::strtod which
                    // sets endptr to the first character past the converted
                    // number. If this pointer is not the same as buffer_re2c,
                    // then either more or less characters have been used
                    // during the comparison. This can happen for inputs like
                    // "01" which will be treated like number 0 followed by
                    // number 1.

                    // conversion
                    char* endptr;
                    const auto float_val = std::strtod(reinterpret_cast<const char*>(current_re2c), &endptr);

                    // check if strtod read beyond the end of the lexem
                    if (reinterpret_cast<const lexer_char_t*>(endptr) != buffer_re2c)
                    {
                        throw std::invalid_argument(std::string("parse error - ") +
                                                    reinterpret_cast<const char*>(current_re2c) + " is not a number");
                    }

                    // check if conversion loses precision
                    const auto int_val = static_cast<int>(float_val);
                    if (float_val == int_val)
                    {
                        // we basic_json not lose precision -> return int
                        return basic_json(int_val);
                    }
                    else
                    {
                        // we would lose precision -> returnfloat
                        return basic_json(float_val);
                    }
                }

                default:
                {
                    std::string error_msg = "parse error - unexpected \'";
                    error_msg += static_cast<char>(current_re2c[0]);
                    error_msg += "\' (";
                    error_msg += token_type_name(last_token) + ")";
                    throw std::invalid_argument(error_msg);
                }
            }
        }

      private:
        /*!
        This function implements a scanner for JSON. It is specified using
        regular expressions that try to follow RFC 7159 and ECMA-404 as close
        as possible. These regular expressions are then translated into a
        deterministic finite automaton (DFA) by the tool RE2C. As a result, the
        translated code for this function consists of a large block of code
        with goto jumps.

        @return the class of the next token read from the buffer

        @todo Unicode support needs to be checked.
        */
        inline token_type get_token()
        {
            // needed by RE2C
            const lexer_char_t* marker;

            // set up RE2C


            for (;;)
            {
                // set current to the begin of the buffer
                current_re2c = buffer_re2c;


                {
                    lexer_char_t yych;
                    unsigned int yyaccept = 0;
                    static const unsigned char yybm[] =
                    {
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64, 192, 192,  64,  64, 192,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        192,  64,   0,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        96,  96,  96,  96,  96,  96,  96,  96,
                        96,  96,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,   0,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                        64,  64,  64,  64,  64,  64,  64,  64,
                    };

                    yych = *buffer_re2c;
                    if (yych <= ':')
                    {
                        if (yych <= '!')
                        {
                            if (yych <= '\f')
                            {
                                if (yych <= 0x08)
                                {
                                    goto json_parser_3;
                                }
                                if (yych <= '\n')
                                {
                                    goto json_parser_5;
                                }
                                goto json_parser_3;
                            }
                            else
                            {
                                if (yych <= '\r')
                                {
                                    goto json_parser_5;
                                }
                                if (yych == ' ')
                                {
                                    goto json_parser_5;
                                }
                                goto json_parser_3;
                            }
                        }
                        else
                        {
                            if (yych <= '-')
                            {
                                if (yych <= '"')
                                {
                                    goto json_parser_6;
                                }
                                if (yych <= '+')
                                {
                                    goto json_parser_3;
                                }
                                if (yych <= ',')
                                {
                                    goto json_parser_7;
                                }
                                goto json_parser_9;
                            }
                            else
                            {
                                if (yych <= '/')
                                {
                                    goto json_parser_3;
                                }
                                if (yych <= '0')
                                {
                                    goto json_parser_10;
                                }
                                if (yych <= '9')
                                {
                                    goto json_parser_12;
                                }
                                goto json_parser_13;
                            }
                        }
                    }
                    else
                    {
                        if (yych <= 'm')
                        {
                            if (yych <= '\\')
                            {
                                if (yych == '[')
                                {
                                    goto json_parser_15;
                                }
                                goto json_parser_3;
                            }
                            else
                            {
                                if (yych <= ']')
                                {
                                    goto json_parser_17;
                                }
                                if (yych == 'f')
                                {
                                    goto json_parser_19;
                                }
                                goto json_parser_3;
                            }
                        }
                        else
                        {
                            if (yych <= 'z')
                            {
                                if (yych <= 'n')
                                {
                                    goto json_parser_20;
                                }
                                if (yych == 't')
                                {
                                    goto json_parser_21;
                                }
                                goto json_parser_3;
                            }
                            else
                            {
                                if (yych <= '{')
                                {
                                    goto json_parser_22;
                                }
                                if (yych == '}')
                                {
                                    goto json_parser_24;
                                }
                                goto json_parser_3;
                            }
                        }
                    }
json_parser_2:
                    {
                        continue;
                    }
json_parser_3:
                    ++buffer_re2c;
json_parser_4:
                    {
                        return last_token = token_type::parse_error;
                    }
json_parser_5:
                    yych = *++buffer_re2c;
                    goto json_parser_60;
json_parser_6:
                    yyaccept = 0;
                    yych = *(marker = ++buffer_re2c);
                    goto json_parser_51;
json_parser_7:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::value_separator;
                    }
json_parser_9:
                    yych = *++buffer_re2c;
                    if (yych <= '/')
                    {
                        goto json_parser_4;
                    }
                    if (yych <= '0')
                    {
                        goto json_parser_49;
                    }
                    if (yych <= '9')
                    {
                        goto json_parser_40;
                    }
                    goto json_parser_4;
json_parser_10:
                    yyaccept = 1;
                    yych = *(marker = ++buffer_re2c);
                    if (yych <= 'D')
                    {
                        if (yych == '.')
                        {
                            goto json_parser_42;
                        }
                    }
                    else
                    {
                        if (yych <= 'E')
                        {
                            goto json_parser_43;
                        }
                        if (yych == 'e')
                        {
                            goto json_parser_43;
                        }
                    }
json_parser_11:
                    {
                        return last_token = token_type::value_number;
                    }
json_parser_12:
                    yyaccept = 1;
                    yych = *(marker = ++buffer_re2c);
                    goto json_parser_41;
json_parser_13:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::name_separator;
                    }
json_parser_15:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::begin_array;
                    }
json_parser_17:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::end_array;
                    }
json_parser_19:
                    yyaccept = 0;
                    yych = *(marker = ++buffer_re2c);
                    if (yych == 'a')
                    {
                        goto json_parser_35;
                    }
                    goto json_parser_4;
json_parser_20:
                    yyaccept = 0;
                    yych = *(marker = ++buffer_re2c);
                    if (yych == 'u')
                    {
                        goto json_parser_31;
                    }
                    goto json_parser_4;
json_parser_21:
                    yyaccept = 0;
                    yych = *(marker = ++buffer_re2c);
                    if (yych == 'r')
                    {
                        goto json_parser_26;
                    }
                    goto json_parser_4;
json_parser_22:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::begin_object;
                    }
json_parser_24:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::end_object;
                    }
json_parser_26:
                    yych = *++buffer_re2c;
                    if (yych == 'u')
                    {
                        goto json_parser_28;
                    }
json_parser_27:
                    buffer_re2c = marker;
                    if (yyaccept == 0)
                    {
                        goto json_parser_4;
                    }
                    else
                    {
                        goto json_parser_11;
                    }
json_parser_28:
                    yych = *++buffer_re2c;
                    if (yych != 'e')
                    {
                        goto json_parser_27;
                    }
                    ++buffer_re2c;
                    {
                        return last_token = token_type::literal_true;
                    }
json_parser_31:
                    yych = *++buffer_re2c;
                    if (yych != 'l')
                    {
                        goto json_parser_27;
                    }
                    yych = *++buffer_re2c;
                    if (yych != 'l')
                    {
                        goto json_parser_27;
                    }
                    ++buffer_re2c;
                    {
                        return last_token = token_type::literal_null;
                    }
json_parser_35:
                    yych = *++buffer_re2c;
                    if (yych != 'l')
                    {
                        goto json_parser_27;
                    }
                    yych = *++buffer_re2c;
                    if (yych != 's')
                    {
                        goto json_parser_27;
                    }
                    yych = *++buffer_re2c;
                    if (yych != 'e')
                    {
                        goto json_parser_27;
                    }
                    ++buffer_re2c;
                    {
                        return last_token = token_type::literal_false;
                    }
json_parser_40:
                    yyaccept = 1;
                    marker = ++buffer_re2c;
                    yych = *buffer_re2c;
json_parser_41:
                    if (yybm[0 + yych] & 32)
                    {
                        goto json_parser_40;
                    }
                    if (yych <= 'D')
                    {
                        if (yych != '.')
                        {
                            goto json_parser_11;
                        }
                    }
                    else
                    {
                        if (yych <= 'E')
                        {
                            goto json_parser_43;
                        }
                        if (yych == 'e')
                        {
                            goto json_parser_43;
                        }
                        goto json_parser_11;
                    }
json_parser_42:
                    yych = *++buffer_re2c;
                    if (yych <= '/')
                    {
                        goto json_parser_27;
                    }
                    if (yych <= '9')
                    {
                        goto json_parser_47;
                    }
                    goto json_parser_27;
json_parser_43:
                    yych = *++buffer_re2c;
                    if (yych <= ',')
                    {
                        if (yych != '+')
                        {
                            goto json_parser_27;
                        }
                    }
                    else
                    {
                        if (yych <= '-')
                        {
                            goto json_parser_44;
                        }
                        if (yych <= '/')
                        {
                            goto json_parser_27;
                        }
                        if (yych <= '9')
                        {
                            goto json_parser_45;
                        }
                        goto json_parser_27;
                    }
json_parser_44:
                    yych = *++buffer_re2c;
                    if (yych <= '/')
                    {
                        goto json_parser_27;
                    }
                    if (yych >= ':')
                    {
                        goto json_parser_27;
                    }
json_parser_45:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= '/')
                    {
                        goto json_parser_11;
                    }
                    if (yych <= '9')
                    {
                        goto json_parser_45;
                    }
                    goto json_parser_11;
json_parser_47:
                    yyaccept = 1;
                    marker = ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= 'D')
                    {
                        if (yych <= '/')
                        {
                            goto json_parser_11;
                        }
                        if (yych <= '9')
                        {
                            goto json_parser_47;
                        }
                        goto json_parser_11;
                    }
                    else
                    {
                        if (yych <= 'E')
                        {
                            goto json_parser_43;
                        }
                        if (yych == 'e')
                        {
                            goto json_parser_43;
                        }
                        goto json_parser_11;
                    }
json_parser_49:
                    yyaccept = 1;
                    yych = *(marker = ++buffer_re2c);
                    if (yych <= 'D')
                    {
                        if (yych == '.')
                        {
                            goto json_parser_42;
                        }
                        goto json_parser_11;
                    }
                    else
                    {
                        if (yych <= 'E')
                        {
                            goto json_parser_43;
                        }
                        if (yych == 'e')
                        {
                            goto json_parser_43;
                        }
                        goto json_parser_11;
                    }
json_parser_50:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
json_parser_51:
                    if (yybm[0 + yych] & 64)
                    {
                        goto json_parser_50;
                    }
                    if (yych <= '"')
                    {
                        goto json_parser_53;
                    }
                    ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= 'e')
                    {
                        if (yych <= '/')
                        {
                            if (yych == '"')
                            {
                                goto json_parser_50;
                            }
                            if (yych <= '.')
                            {
                                goto json_parser_27;
                            }
                            goto json_parser_50;
                        }
                        else
                        {
                            if (yych <= '\\')
                            {
                                if (yych <= '[')
                                {
                                    goto json_parser_27;
                                }
                                goto json_parser_50;
                            }
                            else
                            {
                                if (yych == 'b')
                                {
                                    goto json_parser_50;
                                }
                                goto json_parser_27;
                            }
                        }
                    }
                    else
                    {
                        if (yych <= 'q')
                        {
                            if (yych <= 'f')
                            {
                                goto json_parser_50;
                            }
                            if (yych == 'n')
                            {
                                goto json_parser_50;
                            }
                            goto json_parser_27;
                        }
                        else
                        {
                            if (yych <= 's')
                            {
                                if (yych <= 'r')
                                {
                                    goto json_parser_50;
                                }
                                goto json_parser_27;
                            }
                            else
                            {
                                if (yych <= 't')
                                {
                                    goto json_parser_50;
                                }
                                if (yych <= 'u')
                                {
                                    goto json_parser_55;
                                }
                                goto json_parser_27;
                            }
                        }
                    }
json_parser_53:
                    ++buffer_re2c;
                    {
                        return last_token = token_type::value_string;
                    }
json_parser_55:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= '@')
                    {
                        if (yych <= '/')
                        {
                            goto json_parser_27;
                        }
                        if (yych >= ':')
                        {
                            goto json_parser_27;
                        }
                    }
                    else
                    {
                        if (yych <= 'F')
                        {
                            goto json_parser_56;
                        }
                        if (yych <= '`')
                        {
                            goto json_parser_27;
                        }
                        if (yych >= 'g')
                        {
                            goto json_parser_27;
                        }
                    }
json_parser_56:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= '@')
                    {
                        if (yych <= '/')
                        {
                            goto json_parser_27;
                        }
                        if (yych >= ':')
                        {
                            goto json_parser_27;
                        }
                    }
                    else
                    {
                        if (yych <= 'F')
                        {
                            goto json_parser_57;
                        }
                        if (yych <= '`')
                        {
                            goto json_parser_27;
                        }
                        if (yych >= 'g')
                        {
                            goto json_parser_27;
                        }
                    }
json_parser_57:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= '@')
                    {
                        if (yych <= '/')
                        {
                            goto json_parser_27;
                        }
                        if (yych >= ':')
                        {
                            goto json_parser_27;
                        }
                    }
                    else
                    {
                        if (yych <= 'F')
                        {
                            goto json_parser_58;
                        }
                        if (yych <= '`')
                        {
                            goto json_parser_27;
                        }
                        if (yych >= 'g')
                        {
                            goto json_parser_27;
                        }
                    }
json_parser_58:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
                    if (yych <= '@')
                    {
                        if (yych <= '/')
                        {
                            goto json_parser_27;
                        }
                        if (yych <= '9')
                        {
                            goto json_parser_50;
                        }
                        goto json_parser_27;
                    }
                    else
                    {
                        if (yych <= 'F')
                        {
                            goto json_parser_50;
                        }
                        if (yych <= '`')
                        {
                            goto json_parser_27;
                        }
                        if (yych <= 'f')
                        {
                            goto json_parser_50;
                        }
                        goto json_parser_27;
                    }
json_parser_59:
                    ++buffer_re2c;
                    yych = *buffer_re2c;
json_parser_60:
                    if (yybm[0 + yych] & 128)
                    {
                        goto json_parser_59;
                    }
                    goto json_parser_2;
                }

            }
        }

        inline std::string token_type_name(token_type t)
        {
            switch (t)
            {
                case (token_type::uninitialized):
                    return "<uninitialized>";
                case (token_type::literal_true):
                    return "true literal";
                case (token_type::literal_false):
                    return "false literal";
                case (token_type::literal_null):
                    return "null literal";
                case (token_type::value_string):
                    return "string literal";
                case (token_type::value_number):
                    return "number literal";
                case (token_type::begin_array):
                    return "[";
                case (token_type::begin_object):
                    return "{";
                case (token_type::end_array):
                    return "]";
                case (token_type::end_object):
                    return "}";
                case (token_type::name_separator):
                    return ":";
                case (token_type::value_separator):
                    return ",";
                case (token_type::parse_error):
                    return "<parse error>";
            }
        }

        inline void expect_new(token_type t)
        {
            if (t != last_token)
            {
                std::string error_msg = "parse error - unexpected \'";
                error_msg += static_cast<char>(current_re2c[0]);
                error_msg += "\' (" + token_type_name(last_token);
                error_msg += "); expected " + token_type_name(t);
                throw std::invalid_argument(error_msg);
            }
        }

        /*!
        The pointer current_re2c points to the opening quote of the string, and
        buffer_re2c past the closing quote of the string. We create a std::string from
        the character after the opening quotes (current_re2c+1) until the character
        before the closing quotes (hence subtracting 2 characters from the pointer
        difference of the two pointers).

        @return string value of current token without opening and closing quotes

        @todo Take care of Unicode.
        */
        inline std::string get_string() const
        {
            return std::string(
                       reinterpret_cast<const char*>(current_re2c + 1),
                       static_cast<std::size_t>(buffer_re2c - current_re2c - 2)
                   );
        }

      private:
        /// the buffer
        std::string buffer;
        /// a pointer to the next character to read from the buffer
        const lexer_char_t* buffer_re2c = nullptr;
        /// a pointer past the last character of the buffer
        const lexer_char_t* buffer_re2c_limit = nullptr;
        /// a pointer to the beginning of the current token
        const lexer_char_t* current_re2c = nullptr;
        /// the type of the last read token
        token_type last_token = token_type::uninitialized;
    };
};


/////////////
// presets //
/////////////

/// default JSON class
using json = basic_json<>;

}


/////////////////////////
// nonmember functions //
/////////////////////////

// specialization of std::swap, and std::hash
namespace std
{
/// swaps the values of two JSON objects
template <>
inline void swap(nlohmann::json& j1,
                 nlohmann::json& j2) noexcept(
                     is_nothrow_move_constructible<nlohmann::json>::value and
                     is_nothrow_move_assignable<nlohmann::json>::value
                 )
{
    j1.swap(j2);
}

/// hash value for JSON objects
template <>
struct hash<nlohmann::json>
{
    /// return a hash value for a JSON object
    inline size_t operator()(const nlohmann::json& j) const
    {
        // a naive hashing via the string representation
        return hash<std::string>()(j.dump());
    }
};
}

/*!
This operator implements a user-defined string literal for JSON objects. It can
be used by adding \p "_json" to a string literal and returns a JSON object if
no parse error occurred.

@param s  a string representation of a JSON object
@return a JSON object
*/
inline nlohmann::json operator "" _json(const char* s, std::size_t)
{
    return nlohmann::json::parse(s);
}

#endif
