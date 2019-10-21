#ifndef SPONGE_LIBSPONGE_WRAPPING_INTEGERS_HH
#define SPONGE_LIBSPONGE_WRAPPING_INTEGERS_HH

#include <cstdint>
#include <ostream>

//! \brief A 32-bit integer, expressed relative to an arbitrary initial sequence number (ISN)
//! \note This is used to express TCP sequence numbers (seqno) and acknowledgment numbers (ackno)
class WrappingInt32 {
  private:
    uint32_t _raw_value;  //!< The raw 32-bit stored integer

  public:
    //! Construct from a raw 32-bit unsigned integer
    explicit WrappingInt32(uint32_t raw_value) : _raw_value(raw_value) {}

    // 隐式inline。
    uint32_t raw_value() const { return _raw_value; }  //!< Access raw stored value
};

//! Transform a 64-bit absolute sequence number (zero-indexed) into a 32-bit relative sequence number
//! \param n the absolute sequence number
//! \param isn the initial sequence number
//! \returns the relative sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn);

//! Transform a 32-bit relative sequence number into a 64-bit absolute sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute sequence number
//! \returns the absolute sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint);

//! \name Helper functions
//!@{

//! \brief The offset of `a` relative to `b`
//! \param b the starting point
//! \param a the ending point
//! \returns the number of increments needed to get from `b` to `a`,
//! negative if the number of decrements needed is less than or equal to
//! the number of increments
inline int32_t operator-(WrappingInt32 a, WrappingInt32 b) { return a.raw_value() - b.raw_value(); }

inline bool operator<(WrappingInt32 a, WrappingInt32 b) { return a.raw_value() < b.raw_value(); }

inline bool operator>=(WrappingInt32 a, WrappingInt32 b) { return !(a.raw_value() < b.raw_value()); }

inline bool operator>(WrappingInt32 a, WrappingInt32 b) { return a.raw_value() > b.raw_value(); }

inline bool operator<=(WrappingInt32 a, WrappingInt32 b) { return !(a.raw_value() > b.raw_value()); }

//! \brief Whether the two integers are equal.
inline bool operator==(WrappingInt32 a, WrappingInt32 b) { return a.raw_value() == b.raw_value(); }

//! \brief Whether the two integers are not equal.
inline bool operator!=(WrappingInt32 a, WrappingInt32 b) { return !(a == b); }

//! \brief Serializes the wrapping integer, `a`.
inline std::ostream &operator<<(std::ostream &os, WrappingInt32 a) { return os << a.raw_value(); }

//! \brief The point `b` steps past `a`.
// XXX 注意这个无符号加法，如果溢出了，自带绕圈操作，而不需要`(a.raw_value()+b)%UINT32_LEN`。
// `a.raw_value()+b`相当于从a.raw_value()这个点，往右走了b步，自带绕圈操作。
inline WrappingInt32 operator+(WrappingInt32 a, uint32_t b) { return WrappingInt32{a.raw_value() + b}; }

//! \brief The point `b` steps before `a`.
// `-b`相当于`0-b`，同样若溢出了，自带绕圈操作。`a+(-b)`相当于从a这个点，往左走了b步，自带绕圈操作。
inline WrappingInt32 operator-(WrappingInt32 a, uint32_t b) { return a + -b; }

// XXX 我们把uint32_t这样一个平面的环（其左右端点相邻），想象一个螺旋上升的环路，
// 加法溢出相当于通过uint32_t的右端点往上绕圈，减法溢出相当于通过uint32_t的左端点向下绕圈。
// 将这样一个上升的环路，起点从0开始，一直递增地编号，这个编号也就是absolute seqno，
// 而每个环都从0开始，到UINT32_MAX结束，从isn开始的编号就是seqno。
// 不对syn/fin编号的absolute seqno就是stream index。
// 一个window就是一系列连续的seqno的集合，注意，window本身是随着receiver的ackno移动的，它的左端点就是ackno。
// 它的大小则是receiver当前可接受的数据量。
// 当然，window的移动是螺旋上升式地移动，而不是只在一个平面内移动。
// TODO 画个图。

//!@}

#endif  // SPONGE_LIBSPONGE_WRAPPING_INTEGERS_HH
