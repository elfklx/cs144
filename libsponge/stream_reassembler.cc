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
    vector<size_t> toDelete;
    for (const auto& p: _book) {
        pStart = p.first;
        pEnd = p.first+p.second.first.size(); // 右开。
        // 先是两种不相交的情况。
        if (pEnd <= dataStart)
            continue;
        if (dataEnd <= pStart)
            break;
        // 以下是四种相交的情况。
        if (pStart<=dataStart && dataEnd<=pEnd) {
            // n = 0;
            return;
        } else if (dataStart<=pStart && pEnd<=dataEnd) {
            _nUnassembled -= p.second.first.size();
            // XXX 不要在遍历容器/map时对容器做删除或插入操作，会使得迭代器失效。
            // 虽然在数据量小的时候测试时是正常的，但大数据量时就会出错。
            // lab1最后一个测试过不了就是因为这个。
            // _book.erase(p.first);
            toDelete.push_back(p.first);
        } else if (pStart<dataStart && dataStart<pEnd) {
            n -= (pEnd-dataStart); // 注意不要调换这两句的顺序，因为dataStart会再其中一条语句更新，而两条语句都依赖于旧的dataStart。
            assert(n>0); // 检查无符号数减法不溢出。
            dataStart = pEnd;
        } else if (pStart<dataEnd && dataEnd<pEnd) {
            n -= (dataEnd-pStart);
            assert(n>0);
            dataEnd = pStart;
        }
    }
    for (size_t i: toDelete) {
        _book.erase(i);
    }
    for (const auto& p: _book) {
        pStart = p.first;
        pEnd = p.first+p.second.first.size();
        assert(pEnd<=dataStart || pStart>=dataEnd); // 检查前面的循环是否保证了两两不相交。
    }
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
        for (const auto& p: _book) {
            assert(p.first >= _firstUnassembled);
            if (p.first == _firstUnassembled) {
                n1 = _output.write(p.second.first);
                assert(n1 == p.second.first.size());
                _firstUnassembled += p.second.first.size();
                _nUnassembled -= p.second.first.size();
                if (p.second.second)
                    _output.end_input();
                _book.erase(p.first);
            } else {
                break;
            }
        }
    } else {
        _book[dataStart] = pair<string, int>(string(dataRelativeStart, dataRelativeStart+n), dataEnd==index+data.size()&&eof);
        _nUnassembled += n;
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return _nUnassembled;
}

bool StreamReassembler::empty() const { return _book.empty(); }
