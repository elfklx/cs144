#include "stream_reassembler.hh"

#include <iostream>
#include <cassert>
#include <vector>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /*unused*/) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _firstUnassembled(0), _nUnassembled(0), _book() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
// XXX 一个点是不能用相对于_firstUnassembled的指针，而要用相对于0的绝对指针，不然当_firstUnassembled更新后，所有相对指针都失效了。
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // 若data为空或已写入过了。
    if (data.empty() || index+data.size()<=_firstUnassembled) { // 左闭加区间长度等于右开，所以可以等于。
        if (eof)
            _output.end_input();
        return;
    }
    size_t firstUnacceptable = _firstUnassembled+(_capacity-_output.buffer_size());
    size_t dataStart = index;
    // 使用min()确保不越过firstUnacceptable。
    size_t dataEnd = min(index+data.size(), firstUnacceptable); // 右开。
    if (index < _firstUnassembled) { // 部分已写入byte stream。
        dataStart = _firstUnassembled;
    }
    size_t n = dataEnd - dataStart;
    size_t pEnd, pStart;
    // XXX 如果用注释掉的迭代方式，那么在迭代时，调用了erase()，就可能出错，有一个测试用例是这样的。
    // for (const auto& p: _book) {
    for (auto it=_book.begin(); it!=_book.end(); ) {
        pStart = it->first;
        pEnd = it->first+it->second.first.size(); // 右开。
        // 先是两种不相交的情况。
        if (pEnd <= dataStart) {
            it++;
            continue;
        }
        if (dataEnd <= pStart)
            break;
        // 以下是四种相交的情况。
        if (pStart<=dataStart && dataEnd<=pEnd) {
            // n = 0;
            return;
        } else if (dataStart<=pStart && pEnd<=dataEnd) {
            _nUnassembled -= it->second.first.size();
            it = _book.erase(it);
        } else if (pStart<dataStart && dataStart<pEnd) {
            n -= (pEnd-dataStart);
            dataStart = pEnd;
            it++;
        } else if (pStart<dataEnd && dataEnd<pEnd) {
            n -= (dataEnd-pStart);
            dataEnd = pStart;
            it++;
        }
    }
    /*
    for (const auto& p: _book) {
        pStart = p.first;
        pEnd = p.first+p.second.first.size();
        assert(pEnd<=dataStart || pStart>=dataEnd); // 检查前面的循环是否保证了两两不相交。
    }
    */
    auto dataRelativeStart = data.begin() + (dataStart - index);
    if (dataStart == _firstUnassembled) {
        size_t n1;
        n1 = _output.write(string(dataRelativeStart, dataRelativeStart+n));
        assert(n1 == n);
        _firstUnassembled += n;
        // If the substring is only partially accepted, then the `eof` will be disregarded.
        if (dataEnd==index+data.size() && eof) {
            _output.end_input();
            return;
        }
        for (auto it=_book.begin(); it!=_book.end(); ) {
            assert(it->first >= _firstUnassembled);
            if (it->first == _firstUnassembled) {
                n1 = _output.write(it->second.first);
                assert(n1 == it->second.first.size());
                _firstUnassembled += it->second.first.size();
                _nUnassembled -= it->second.first.size();
                if (it->second.second)
                    _output.end_input();
                it = _book.erase(it);
            } else {
                break;
            }
        }
    } else {
        _book[dataStart] = pair<string, int>(string(dataRelativeStart, dataRelativeStart+n), dataEnd==index+data.size()&&eof); // 如果该segment的数据没有完全接收完，就忽略eof。
        _nUnassembled += n;
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return _nUnassembled;
}

bool StreamReassembler::empty() const { return _book.empty(); }
