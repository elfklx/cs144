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
    // inside the receiver’s window. 左闭+长度=右开，减一=右闭。 Segment overflowing the window on both sides is
    // unacceptable.
    if (hdr.seqno + seg.length_in_sequence_space() - 1 < _ackno || hdr.seqno >= _ackno + window_size() ||
        (hdr.seqno < _ackno && hdr.seqno + seg.length_in_sequence_space() >= _ackno + window_size()))
        return false;

    // Push any data, or end-of-stream marker, to the StreamReassembler.
    // 这里不用检查空间是否足够，因为push_substring()只会接受能放下的那一部分数据，而且它也会处理overlap。
    // 因为stream index忽略syn/fin，所以要减一。
    _reassembler.push_substring(
        seg.payload().copy(), unwrap(hdr.seqno, _isn, _reassembler.stream_out().bytes_written()) - 1, hdr.fin);

    // 注意，下面这段代码要放在上面的push_substring()后面，因为push_substring()可能会改变window_size();

    if (hdr.seqno == _ackno) {
        _ackno = _ackno + seg.length_in_sequence_space();  // XXX syn和fin有对应的seqno，虽然它们并不是消息的一部分。
        for (const auto &p : _pendingSeqnos) {  // std::map是按键有序的。
            if (p.first == _ackno) {
                _ackno = _ackno + p.second;
                _pendingSeqnos.erase(p.first);
            }
        }
    } else {
        /*
        if (hdr.seqno < _ackno)
            hdr.seqno = _ackno;
        */
        /*
        if (seg.length_in_sequence_space() <= window_size())
            _pendingSeqnos[hdr.seqno] = seg.length_in_sequence_space();
        else
            _pendingSeqnos[hdr.seqno] = window_size();
        */
        // 写出上面的代码是因为没有理解window的概念：
        // XXX the “window” refers to a range of byte indices, or sequence numbers, that are currently acceptable to the
        // receiver. _pendingSeqnos[hdr.seqno] = seg.length_in_sequence_space(); size_t free = (_ackno+window_size()) -
        // hdr.seqno; // 该segment的数据从hdr.seqno开始。 要换算成abs
        // seqno，避免出错，因为_ackno和hdr.seqno都是在wrapped space中的相对值。 但好像又不会出错？
        size_t free = unwrap(_ackno, _isn, _reassembler.stream_out().bytes_written()) + window_size() -
                      unwrap(hdr.seqno, _isn, _reassembler.stream_out().bytes_written());
        if (free > seg.length_in_sequence_space())
            _pendingSeqnos[hdr.seqno] = seg.length_in_sequence_space();
        else
            _pendingSeqnos[hdr.seqno] = free;
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
