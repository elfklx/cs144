#include "tcp_receiver.hh"

#include <iostream>
using namespace std;

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &hdr = seg.header();
    // If the ISN hasn’t been set yet, a segment is acceptable if (and only if) it has the SYN bit set.
    if (!_synReceived && !hdr.syn)
        return false;
    if (!_synReceived) {
        _synReceived = true;
        // The sequence number of the ﬁrstarriving segment that has the SYN ﬂag set is the initial sequence number.
        _isn = hdr.seqno;  // 因为没有使用外部资源，所以可以使用编译器生成的拷贝ctor。
        _ackno = hdr.seqno;
    }
    // A segment is acceptable (and the method should return true) if any of the sequence numbers it occupies falls
    // inside the receiver’s window.
    // 左闭+长度=右开，减一=右闭。
    // Segment overflowing the window on both sides is unacceptable.
    if (hdr.seqno + seg.length_in_sequence_space() - 1 < _ackno || hdr.seqno >= _ackno + window_size() ||
        (hdr.seqno < _ackno && hdr.seqno + seg.length_in_sequence_space() >= _ackno + window_size()))
        return false;

    // Push any data, or end-of-stream marker, to the StreamReassembler.
    // 因为stream index忽略syn/fin，所以要减一。
    _reassembler.push_substring(
        seg.payload().copy(), unwrap(hdr.seqno, _isn, _reassembler.stream_out().bytes_written()) - 1, hdr.fin);

    if (hdr.seqno == _ackno) {
        _ackno = wrap(_reassembler.first_unassembled(), _isn) + 1lu; // 加一是因为byte stream不包括对syn和fin编号。
        if (hdr.fin)
            _ackno = _ackno + 1;
    }
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_synReceived)
        return _ackno;
    return optional<WrappingInt32>{};
}

// In practice, your receiver will announce a window size equal to
// its capacity minus the number of bytes being held in its StreamReassembler’s ByteStream.
size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
