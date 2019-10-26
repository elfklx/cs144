#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmissionTimeout{retx_timeout}
    , _stream(capacity)
    , _outstandingSegments() {}

uint64_t TCPSender::bytes_in_flight() const { return _nBytesInFlight; }

void TCPSender::fill_window() {
    if (_finSent) // 发送了fin后就不再发送数据了，即使fin还没被ack。
        return;
    TCPSegment seg;
    TCPHeader& hdr = seg.header();
    // XXX 这里ackno和win不需要sender设置，由本端的receiver设置。
    if (_next_seqno == 0) {
        // 发起三次握手。
        hdr.syn = true;
    } else {
        // 调用move()返回右值引用，使得可以匹配到Buffer的移动构造函数。
        // 使用min()确保payload大小不超过TCPConfig::MAX_PAYLOAD_SIZE。
        auto size = min(_notifyWinSize, static_cast<uint16_t>(TCPConfig::MAX_PAYLOAD_SIZE));
        seg.payload() = Buffer(move(_stream.read(size)));
        if (_stream.input_ended() && seg.length_in_sequence_space()<_notifyWinSize) { // 如果接受窗口还有空间的话。
            hdr.fin = true;
            _finSent = true;
        }
    }
    auto len = seg.length_in_sequence_space();
    if (len) {
        hdr.seqno = wrap(_next_seqno, _isn);
        _nBytesInFlight += len;
        _next_seqno += len;
        _notifyWinSize -= len;
        _segments_out.push(seg);
        _outstandingSegments.insert(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if (ackno > wrap(_next_seqno, _isn))
        return false;
    // (a) Set the RTO back to its “initial value.” 
    _retransmissionTimeout = _initial_retransmission_timeout;
    _notifyWinSize = window_size;
    _notifyWinStart = unwrap(ackno, _isn, _next_seqno); // 因为ackno之前的序号标识的byte都已被收到了，所以可以推进通知窗口。
    auto it = _outstandingSegments.begin();
    vector<decltype(it)> toDelete;
    for (; it!=_outstandingSegments.end(); it++) {
        if (it->header().seqno+it->length_in_sequence_space() <= ackno) {
            _nBytesInFlight -= it->length_in_sequence_space();
            toDelete.push_back(it);
        } else {
            break;
        }
    }
    for (auto i: toDelete)
        _outstandingSegments.erase(i);
    // (b) If the sender has any outstanding data, restart the retransmission timer
    // so that it will expire after RTO milliseconds (for the current value of RTO). 
    if (!_outstandingSegments.empty())
        _retransmissionTimer = 0;
    // (c) Reset the count of “consecutive retransmissions” back to zero.
    _nConsecutiveretransmissions = 0;
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _retransmissionTimer += ms_since_last_tick;
    if (_retransmissionTimer >= _retransmissionTimeout) {
        // (a) Retransmit the earliest (lowest sequence number) segment that hasn’t been fully acknowledged by the TCP receiver.
        _segments_out.push(*_outstandingSegments.begin());
        if (_notifyWinSize!=0 || _next_seqno==1) { // 一开始通知窗口大小为1，所以要加一个条件。
            // i. Keep track of the number of consecutive retransmissions, and increment it because you just retransmitted something.
            _retransmissionTimeout *= 2;
            // ii. Double the value of RTO.
            _nConsecutiveretransmissions++;
        }
        // (c) Start the retransmission timer, such that it expires after RTO milliseconds
        _retransmissionTimer = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _nConsecutiveretransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
