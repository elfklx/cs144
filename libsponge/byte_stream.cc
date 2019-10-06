#include "byte_stream.hh"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _stream(), _size(0), _capacity(capacity), _eof(false), _input_ended(false), _nWritten(0), _nRead(0) {}

size_t ByteStream::write(const string &data) {
    long unsigned n, idle = _capacity - _size;
    for (n = 0; n < idle && n < data.size(); n++) {
        _stream.push_back(data[n]);
    }
    _size += n;
    _nWritten += n;
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // 只是peek，所以不改变_size和_nRead。
    if (len > _size) {
        // set_error(); // const指针不能调用非const方法。
        throw runtime_error("len > _size");
    }
    string s;
    s.resize(len);
    auto begin = _stream.begin();
    copy(begin, begin + len, s.begin());
    return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len > _size) {
        set_error();
        throw runtime_error("len > _size");
    }
    for (long unsigned i = 0; i < len; i++)
        _stream.pop_front();
    _size -= len;
    _nRead += len;
    if (buffer_empty() && input_ended())
        _eof = true;
}

void ByteStream::end_input() {
    _input_ended = true;
    if (buffer_empty())
        _eof = true;
}

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return _size == 0; }

bool ByteStream::eof() const { return _eof; }

size_t ByteStream::bytes_written() const { return _nWritten; }

size_t ByteStream::bytes_read() const { return _nRead; }

size_t ByteStream::remaining_capacity() const { return _capacity - _size; }
