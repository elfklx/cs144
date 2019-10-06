#include "stream_reassembler.hh"

#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _toReassemble(0), _nUnassembled(0), _book() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (data.empty()) {
        if (eof)
            _output.end_input();
        return;
    }
    // 去重。
    // 但这个去重无法应对部分overlap的情况，只能处理**整个**segment与另一个segment overlap的情况。
    // int proposal = data.size();
    bool insert = true;
    for (const auto &p : _book) {
        if (p.first >= index && p.first + p.second.first.size() < index + data.size()) {
            _nUnassembled -= p.second.first.size();
            _book.erase(p.first);  // overlap
        // } else if (p.first > index && p.first + p.second.first.size() > index + data.size())
            //    proposal = p.first - index; // 部分交叉。
        } else if (p.first <= index && p.first + p.second.first.size() >= index + data.size())
            insert = false;  // overlap
    }
    if (insert) {
        size_t nRemain = _capacity - _nUnassembled - _output.buffer_size();
        if (data.size() > nRemain) {
            // If accepting all the data would overflow the `capacity` of this
            // `StreamReassembler`, then only the part of the data that fits will be
            // accepted. If the substring is only partially accepted, then the `eof`
            // will be disregarded.
            _book[index] = pair<string, bool>(string(data.cbegin(), data.cbegin() + nRemain), false);
            _nUnassembled += nRemain;
        } else {
            _book[index] = pair<string, bool>(data, eof);
            _nUnassembled += data.size();
        }
    }
    size_t nReassembled, nWriten;
    bool eof_;
    // 考察当前的segment及之前已到达的segment是否可以放入byte stream了。
    for (const auto &p : _book) {
        if (p.first > _toReassemble)
            break;  // 可以直接break，而不必continue，因为std::map是按键有序的哈希表。
        const string &s = p.second.first;
        eof_ = p.second.second;
        nReassembled = _toReassemble - p.first;
        if (nReassembled >= s.size()) {
            _nUnassembled -= s.size();
            _book.erase(p.first);
            continue;
        }
        // nWriten = _output.write(s.data()+nReassembled); // 不要使用这种构造方法，因为字符串字面值以'\0'结尾。
        nWriten = _output.write(s.substr(nReassembled));
        _toReassemble += nWriten;
        _nUnassembled -= nWriten;

        // 这个segment还没完全写入byte stream中。
        if (nWriten < s.size() - nReassembled) {
            // 上面的去重操作已经确保了，对于一个[p.first(), p.first()+p.second.first.size()]的segment，
            // 在_book中，不存在i属于(p.first(), p.first()+p.second.first.size()]，_book[i]存在。
            // 但这个去重无法应对部分overlap的情况。
            _book[_toReassemble] = pair<string, bool>(s.substr(nReassembled + nWriten), eof_);
            _book.erase(p.first);  // 擦掉旧的。
            break;                 // 没完全写入，说明_output满了，不必`continue`。
        }

        if (eof_)
            _output.end_input();
        _book.erase(p.first);
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    /*
    int nUnassembled = 0;
    for (const auto &p : _book)
        nUnassembled += p.second.first.size();
    return nUnassembled;
    */
    return _nUnassembled;
}

bool StreamReassembler::empty() const { return _book.empty(); }
