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
#include "packet_types.hpp"

static_assert(sizeof(float) == 4 && sizeof(double) == 8);
static_assert(sizeof(glm::vec3) == 12 && sizeof(glm::dvec3) == 24 && sizeof(glm::quat) == 16);
static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);

template <typename T>
struct WrappedType {
    using Type = T;
    Type val;
    constexpr static inline T defaultVal = T{};
    inline WrappedType(Type v): val(v) {}
};

template <typename T>
concept Is16BitUint = std::same_as<unsigned short, T> || std::same_as<uint16_t, T>;
template <typename T>
concept Is16BitInt = std::same_as<short, T> || std::same_as<int16_t, T>;
template <typename T>
concept Is32BitUint = std::same_as<unsigned, T> || std::same_as<unsigned int, T> || std::same_as<uint32_t, T>;
template <typename T>
concept Is32BitInt = std::same_as<int, T> || std::same_as<int32_t, T>;

template <typename T> //requires (!requires {Is16BitUint<T>; }) && (!requires {Is16BitInt<T>; }) && (!requires {Is32BitUint<T>; }) && (!requires {Is32BitInt<T>; })
struct SerializedType : std::false_type { };
template<> 
struct SerializedType<float> : public WrappedType<float> {};
template<>
struct SerializedType<double> : public WrappedType<double> {};
template<>
struct SerializedType<glm::vec3> : public WrappedType<glm::vec3> {};
template<>
struct SerializedType<glm::dvec3> : public WrappedType<glm::dvec3> {};
template <>
struct SerializedType<glm::quat> : public WrappedType<glm::quat> {};
template<typename T> requires Is32BitInt<T>
struct SerializedType<T> : public WrappedType<int32_t> {};
template<typename T> requires Is32BitUint<T>
struct SerializedType<T> : public WrappedType<uint32_t> {};
template<typename T> requires Is16BitInt<T>
struct SerializedType<T> : public WrappedType<int16_t> {};
template<typename T> requires Is16BitUint<T>
struct SerializedType<T> : public WrappedType<uint16_t> {};
template<>
struct SerializedType<int8_t> : public WrappedType<int8_t> {};
template<>
struct SerializedType<uint8_t> : public WrappedType<uint8_t> {};
template<>
struct SerializedType<char> : public WrappedType<unsigned char> {};

template <typename T>
concept IsSerializablePrimitiveType = !std::is_base_of < std::false_type, SerializedType<T>>::value;
template <typename T>
concept IsNotSerializablePrimitiveType = std::is_base_of < std::false_type, SerializedType<T>>::value;

static_assert(Is16BitUint<uint16_t>);
//static_assert(Is16BitUint<uint32_t>);
static_assert(!std::is_base_of<std::false_type, SerializedType<uint16_t>>());

template <typename T>
inline size_t SerializedSize(const T& val) { static_assert(false); }
template <IsSerializablePrimitiveType T>
inline size_t SerializedSize(const T& val = SerializedType<T>::defaultVal) { return sizeof(SerializedType<T>::Type); }
template <>
inline size_t SerializedSize(const bool& val) { return sizeof(bool); }
template <IsSerializablePrimitiveType T>
inline size_t SerializedSize(const std::vector<T>& val) { return sizeof(SerializedType<unsigned>::Type) + sizeof(SerializedType<T>::Type) * val.size(); }
template <IsSerializablePrimitiveType T, size_t ArrayLen>
inline size_t SerializedSize(const std::array<T, ArrayLen>& val) { return sizeof(SerializedType<T>::Type) * val.size(); }
template <>
inline size_t SerializedSize(const std::string& val) { return sizeof(SerializedType<uint32_t>::Type) + val.length(); }


template <IsNotSerializablePrimitiveType T>
inline size_t SerializedSize() { static_assert(false); }
template <>
inline size_t SerializedSize<TransformSync>() { return SerializedSize<decltype(TransformSync::position)>() + SerializedSize<decltype(TransformSync::rotation)>(); }
template <>
inline size_t SerializedSize<RigidbodySync>() { return SerializedSize<decltype(RigidbodySync::velocity)>() + SerializedSize<decltype(RigidbodySync::angularVelocity)>(); }
template <>
inline size_t SerializedSize<PacketStructs::TransformSyncSnapshot>() { 
    return SerializedSize<decltype(PacketStructs::TransformSyncSnapshot::syncId)>() + 
        SerializedSize<decltype(PacketStructs::TransformSyncSnapshot::transform)>(); }
template <>
inline size_t SerializedSize<PacketStructs::RigidbodySyncSnapshot>() {
    return SerializedSize<decltype(PacketStructs::RigidbodySyncSnapshot::syncId)>() +
        SerializedSize<decltype(PacketStructs::RigidbodySyncSnapshot::transform)>() +
        SerializedSize<decltype(PacketStructs::RigidbodySyncSnapshot::rigidbody)>();
}
template <>
inline size_t SerializedSize<PacketStructs::TransformSyncPacket>() {
    return SerializedSize<decltype(PacketStructs::TransformSyncPacket::identifier)>() +
        SerializedSize<decltype(PacketStructs::TransformSyncPacket::tick)>();
}
template <>
inline size_t SerializedSize<PacketStructs::RigidbodySyncPacket>() {
    return SerializedSize<decltype(PacketStructs::RigidbodySyncPacket::identifier)>() +
        SerializedSize<decltype(PacketStructs::RigidbodySyncPacket::tick)>();
}

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
template <>
inline void Serialize(bool val, void*& dest) {
    if (val == true) Serialize<uint8_t>(1, dest);
    else Serialize<uint8_t>(0, dest);
}

inline void Serialize(TransformSync val, void*& dest) {
    Serialize(val.position, dest);
    Serialize(val.rotation, dest);
}

inline void Serialize(RigidbodySync val, void*& dest) {
    Serialize(val.velocity, dest);
    Serialize(val.angularVelocity, dest);
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
inline bool Deserialize(void*& src) {
    uint8_t v = Deserialize<uint8_t>(src);
    if (v == 1) return true;
    else return false;
}

template <>
inline TransformSync Deserialize(void*& src) {
    glm::dvec3 pos = Deserialize<glm::dvec3>(src);
    glm::quat rot = Deserialize<glm::quat>(src);
    return TransformSync{
        .position = pos,
        .rotation = rot
    };
}

template <>
inline RigidbodySync Deserialize(void*& src) {
    glm::vec3 vel = Deserialize<glm::vec3>(src);
    glm::vec3 angVel = Deserialize<glm::vec3>(src);
    return RigidbodySync{
        .velocity = vel,
        .angularVelocity = angVel
    };
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