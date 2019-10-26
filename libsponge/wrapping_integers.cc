#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

static const uint64_t UINT32_LEN = (1ul << 32);

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // seqno是一个相对于isn的相对值（会绕圈，取值范围是[0, UINT32_MAX]），而absolute
    // seqno是一个相对于0的绝对值（不会绕圈，无限大）。 n %= UINT32_MAX; // %运算执行绕圈操作，将取值限定在wrapped
    // space。 上面的语句是错的，这样永远无法取到UINT32_MAX这个值。
    n %= UINT32_LEN;                        // 往下绕圈。
    return isn + static_cast<uint32_t>(n);  // 调用.hh中定义的加法操作符。
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN. (XXX 也就是每一个发送端有一个自己的ISN。)
//
// A checkpoint is required because any seqno corresponds to many absolute seqnos.
// E.g. the seqno “17” corresponds to the absolute seqno of 17,
// but also 2^32 + 17（往上绕了一圈）, or 2^33 + 17（往上绕了两圈，2^33=2*2^32）, or 234 + 17, etc.
// The checkpoint helps to resolve the ambiguity
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // 注意，seqno是从isn开始的，而不是从0开始。
    // 所以转换的第一步是先将从isn开始**映射到**从0开始，然后绕圈到大于等于checkpoint，然后检查是否最接近checkpoint。
    // 一次消息发送中，消息中的每一个字节都有对应的sqeno（不唯一），也有对应的absolute seqno（唯一）。
    // 因为会绕圈，所以n可能会比isn小。但无符号数相减自带绕圈，可以正确地得出两数的差值。
    uint64_t absSeqno = n.raw_value()-isn.raw_value();
    while (absSeqno < checkpoint)
        absSeqno += UINT32_LEN;
    if (absSeqno >= UINT32_LEN) {
        if (checkpoint - (absSeqno - UINT32_LEN) < absSeqno - checkpoint)
            absSeqno -= UINT32_LEN;
    }
    return absSeqno;
}
