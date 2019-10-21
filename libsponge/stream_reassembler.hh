#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <map>
#include <string>

// XXX 在头文件中不能用`using namespace std;`，这会有意或无意地影响到包含头文件的文件。

// XXX
// The TCP sender is dividing its byte stream up into short segments (substrings no more than
// about 1,460 bytes apiece) so that they each fit inside a datagram. But the network might
// reorder these datagrams, or drop them, or deliver them more than once. The receiver must
// reassemble the segments into the contiguous stream of bytes that they started out as.

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    size_t _firstUnassembled;
    size_t _nUnassembled;
    std::map<size_t, std::pair<std::string, bool>> _book;  // 登记已到达但还不能放入byte stream的segment。

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note XXX This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then only the part of the data that fits will be
    //! accepted. If the substring is only partially accepted, then the `eof`
    //! will be disregarded.
    //!
    //! \param data the string being added
    //! \param index the index of the first byte in `data`（这个英文应该理解为“`data`中第一个字节（在整个byte stream）的下标”而不是"第一个字节在`data`中的下标"）
    //! \param eof whether or not this segment ends with the end of the stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been submitted twice, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
