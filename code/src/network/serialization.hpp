// All data is serialized into little-endian byte order.

#pragma once
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_double3.hpp>
#include <concepts>
#include <climits>
#include <vector>
#include <array>
#include <string>
#include <bit>

static_assert(sizeof(float) == 4 && sizeof(double) == 8);
static_assert(sizeof(glm::vec3) == 12 && sizeof(glm::dvec3) == 24);
static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

template <typename T>
struct WrappedType {
    using Type = T;
    Type val;

    inline WrappedType(Type v): val(v) {}
};

template <typename T>
struct SerializedType : std::false_type { };
template<> 
struct SerializedType<float> : public WrappedType<float> {};
template<>
struct SerializedType<double> : public WrappedType<double> {};
template<>
struct SerializedType<int> : public WrappedType<int32_t> {};
template<>
struct SerializedType<unsigned> : public WrappedType<uint32_t> {};
template<>
struct SerializedType<char> : public WrappedType<unsigned char> {};

template <typename T>
concept IsSerializablePrimitiveType = !std::is_base_of < std::false_type, SerializedType<T>>::value;

template <typename T>
inline size_t SerializedSize(const T& val) { static_assert(false); }
template <IsSerializablePrimitiveType T>
inline size_t SerializedSize(const T& val) { return sizeof(SerializedType<T>::Type); }
template <IsSerializablePrimitiveType T>
inline size_t SerializedSize(const std::vector<T>& val) { return sizeof(SerializedType<unsigned>::Type) + sizeof(SerializedType<T>::Type) * val.size(); }
template <IsSerializablePrimitiveType T, size_t ArrayLen>
inline size_t SerializedSize(const std::array<T, ArrayLen>& val) { return sizeof(SerializedType<T>::Type) * val.size(); }
template <>
inline size_t SerializedSize(const std::string& val) { return sizeof(SerializedType<unsigned>::Type) + val.length(); }

template <typename T>
inline T swap_endian(T u) // from https://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c
{
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

// Increments the given void*
template<typename T> 
inline void Serialize(T val, void*&) { static_assert(false); }
template <IsSerializablePrimitiveType T>
inline void Serialize(T val, void*& dest) {
    //DebugLogInfo("Serializing, at ", dest);
    SerializedType<T> v = static_cast<SerializedType<T>>(val);

    if (std::endian::native == std::endian::big) {
        v.val = swap_endian(v.val);
    }

	memcpy(dest, &v.val, sizeof(v.val));
    dest = (uint8_t*)dest + sizeof(v.val);
}
template <IsSerializablePrimitiveType T>
inline void Serialize(std::vector<T> val, void*& dest) {
    Serialize<unsigned>(val.size(), dest);
    for (T& v : val) {
        Serialize<T>(v, dest);
    }
}
template <>
inline void Serialize(std::string val, void*& dest) {
    //DebugLogInfo("Serializing, at ", dest, " str ", val.length());
    Serialize<unsigned>(val.length(), dest);
    for (char& v : val) {
        Serialize<char>(v, dest);
    }
}
template <IsSerializablePrimitiveType T, size_t ArrayLen>
inline void Serialize(std::array<T, ArrayLen> val, void*& dest) {
    for (T& v : val) {
        Serialize<T>(v, dest);
    }
}

// Increments the given void*
template<typename T>
inline T Deserialize(void*& src) { static_assert(false); }
template <IsSerializablePrimitiveType T>
inline T Deserialize(void*& src) {
    T v = ((SerializedType<T>*)src)->val;

    if (std::endian::native == std::endian::big) {
        v = swap_endian(v);
    }

    src = (uint8_t*)src + SerializedSize<T>(v);
    return v;
}
template <>
inline std::string Deserialize(void*& src) {
    unsigned size = Deserialize<unsigned>(src);
    std::string retval;
    retval.reserve(size);
    for (unsigned i = 0; i < size; i++) {
        retval += Deserialize<char>(src);
    }
    //src = (uint8_t*)src + size; NO because Deserialize<char> does this for us.
    return retval;
}

// aarray isntead of array because apparently is_array already exists???
template <typename T>
struct is_aarray : std::false_type {};
template <typename T, size_t N>
struct is_aarray<std::array<T, N>> : std::true_type {};
template<typename T>
concept SerializableArray = is_aarray<T>::value;

template <typename T>
struct is_vector : std::false_type {};
template <typename T, typename A>
struct is_vector<std::vector<T, A>> : std::true_type {};
template<typename T>
concept SerializableVec = is_vector<T>::value;

template <SerializableVec VecT>
inline VecT Deserialize(void*& src) {
    unsigned size = Deserialize<unsigned>(src);
    auto retval = VecT();
    retval.reserve(size);
    using T = VecT::value_type;
    for (unsigned i = 0; i < size; i++) {
        retval.emplace_back(Deserialize<T>(src));
    }
    return retval;
}

template <SerializableArray ArrT>
inline ArrT Deserialize(void*& src) {
    auto retval = ArrT();
    using T = ArrT::value_type;
    for (unsigned i = 0; i < retval.size(); i++) {
        retval[i] =Deserialize<T>(src) ;
    }
    return retval;
}