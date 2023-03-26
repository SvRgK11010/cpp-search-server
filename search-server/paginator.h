#pragma once
#include <vector>
#include <iterator>


template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) {
        begin_ = begin;
        end_ = end;
        iter_size_ = distance(begin, end);
    }

    const Iterator begin() const {
        return begin_;
    }

    const Iterator end() const {
        return end_;
    }

    Iterator size() const {
        return iter_size_;
    }

    Iterator begin() {
        return begin_;
    }

    Iterator end() {
        return end_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t iter_size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& iterator_range) {
    for (auto it = iterator_range.begin(); it != iterator_range.end(); it++) {
        output << *it;
    }
    return output;
}


template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        if (distance(begin, end) < page_size) {
            IteratorRange page(begin, end);
            pages_.push_back(page);
        }
        else {
            size_t pages_count = distance(begin, end) / page_size;
            Iterator pg_begin = begin;
            Iterator pg_end = begin + page_size;
            if (distance(begin, end) % page_size == 0) {
                for (size_t it = 0; it < pages_count; it++) {
                    IteratorRange page(pg_begin, pg_end);
                    pages_.push_back(page);
                    advance(pg_begin, page_size);
                    advance(pg_end, page_size);
                }
            }
            else if (distance(begin, end) % page_size != 0) {
                pages_count += 1;
                for (size_t it = 0; it < pages_count; it++) {
                    if (it != pages_count - 1) {
                        IteratorRange page(pg_begin, pg_end);
                        pages_.push_back(page);
                        advance(pg_begin, page_size);
                        advance(pg_end, page_size);
                    }
                    else {
                        IteratorRange page(pg_begin, end);
                        pages_.push_back(page);
                    }
                }
            }
        }
    }

    auto size() const {
        return pages_.size();
    }

    auto end() const {
        return pages_.end();
    }

    auto begin() const {
        return pages_.begin();
    }



private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}