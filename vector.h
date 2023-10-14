#ifndef OOP_ASSIGNMENTS_VECTOR_VECTOR_H_
#define OOP_ASSIGNMENTS_VECTOR_VECTOR_H_
#define VECTOR_MEMORY_IMPLEMENTED
#include <memory>
#include <iterator>
#include <stdexcept>
#include <exception>
#include <algorithm>

template <typename T, class Allocator = std::allocator<T>>
class Vector {
 public:
  using ValueType = T;
  using Pointer = T*;
  using ConstPointer = const T*;
  using Reference = T&;
  using ConstReference = const T&;
  using SizeType = size_t;
  using Iterator = Pointer;
  using ConstIterator = ConstPointer;
  using ReverseIterator = std::reverse_iterator<Iterator>;
  using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

 private:
  template <class Iter>
  using EnableIfForwardIter = std::enable_if_t<
      std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<Iter>::iterator_category>>;

 public:
  Vector() noexcept = default;
  Vector(const std::initializer_list<T>& init_lst) {
    try {
      size_ = init_lst.size();
      capacity_ = size_;
      if (init_lst.size() != 0) {
        buffer_ = AllocTraits::allocate(alloc_, capacity_);
        CopyIterRange(init_lst.begin(), init_lst.end());
      }
    } catch (...) {
      AllocTraits::deallocate(alloc_, buffer_, capacity_);
      size_ = 0;
      capacity_ = 0;
      buffer_ = nullptr;
      throw;
    }
  }  // copy safety

  Vector(std::initializer_list<T>&& init_lst) {
    try {
      size_ = init_lst.size();
      capacity_ = size_;
      if (init_lst.size() != 0) {
        buffer_ = AllocTraits::allocate(alloc_, capacity_);
        MoveIterRange(init_lst.begin(), init_lst.end());
      }
    } catch (...) {
      AllocTraits::deallocate(alloc_, buffer_, capacity_);
      size_ = 0;
      capacity_ = 0;
      buffer_ = nullptr;
      throw;
    }
  }  // copy safety

  template <typename InputIterator, typename = EnableIfForwardIter<InputIterator>>
  Vector(InputIterator begin, InputIterator end) {
    try {
      size_ = std::distance(begin, end);
      capacity_ = size_ * 2;
      if (size_ != 0) {
        buffer_ = AllocTraits::allocate(alloc_, capacity_);
        CopyIterRange(begin, end);
      }
    } catch (...) {
      AllocTraits::deallocate(alloc_, buffer_, capacity_);
      size_ = 0;
      capacity_ = 0;
      buffer_ = nullptr;
      throw;
    }
  }  // copy safety

  Vector(const Vector& other) {
    try {
      alloc_ = other.alloc_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      if (size_ != 0) {
        buffer_ = AllocTraits::allocate(alloc_, capacity_);
        CopyIterRange(other.begin(), other.end());
      }
    } catch (...) {
      AllocTraits::deallocate(alloc_, buffer_, capacity_);
      size_ = 0;
      capacity_ = 0;
      buffer_ = nullptr;
      throw;
    }
  }

  Vector(Vector&& other) noexcept
      : alloc_(other.alloc_)
      , buffer_(std::exchange(other.buffer_, nullptr))
      , size_(std::exchange(other.size_, 0))
      , capacity_(std::exchange(other.capacity_, 0)) {
  }

  explicit Vector(size_t size) {
    size_t count = 0;
    try {
      size_ = size;
      capacity_ = size;
      if (size != 0) {
        buffer_ = AllocTraits::allocate(alloc_, capacity_);
        for (auto iter = begin(); iter != end(); ++iter) {
          AllocTraits::construct(alloc_, iter);
          ++count;
        }
      }
    } catch (...) {
      for (size_t i = 0; i < count; i++) {
        AllocTraits::destroy(alloc_, buffer_ + i);
      }
      AllocTraits::deallocate(alloc_, buffer_, capacity_);
      size_ = 0;
      capacity_ = 0;
      buffer_ = nullptr;
      throw;
    }
  }

  Vector(size_t size, const T& value) {
    size_t count = 0;
    try {
      size_ = size;
      capacity_ = size;
      if (size != 0) {
        buffer_ = AllocTraits::allocate(alloc_, capacity_);
        for (auto iter = begin(); iter != end(); ++iter) {
          AllocTraits::construct(alloc_, iter, value);
          ++count;
        }
      }
    } catch (...) {
      for (size_t i = 0; i < count; i++) {
        AllocTraits::destroy(alloc_, buffer_ + i);
      }
      AllocTraits::deallocate(alloc_, buffer_, capacity_);
      size_ = 0;
      capacity_ = 0;
      buffer_ = nullptr;
      throw;
    }
  }

  Vector& operator=(const Vector& other) {
    if (this != &other) {
      auto backup_buff = std::exchange(buffer_, nullptr);
      auto backup_size = std::exchange(size_, 0);
      auto backup_capacity = std::exchange(capacity_, 0);
      try {
        Vector(other).Swap(*this);
        for (size_t i = 0; i < backup_size; i++) {
          AllocTraits::destroy(alloc_, backup_buff + i);
        }
        AllocTraits::deallocate(alloc_, backup_buff, backup_capacity);
      } catch (...) {
        buffer_ = backup_buff;
        size_ = backup_size;
        capacity_ = backup_capacity;
        throw;
      }
    }
    return *this;
  }
  Vector& operator=(Vector&& other) noexcept {
    if (this != &other) {
      Vector(std::move(other)).Swap(*this);
    }
    return *this;
  }

  ~Vector() noexcept(std::is_nothrow_destructible_v<ValueType>) {
    for (size_t i = 0; i < size_; i++) {
      AllocTraits::destroy(alloc_, buffer_ + i);
    }
    AllocTraits::deallocate(alloc_, buffer_, capacity_);
  }

  [[nodiscard]] SizeType Size() const noexcept {
    return size_;
  }
  [[nodiscard]] SizeType Capacity() const noexcept {
    return capacity_;
  }
  [[nodiscard]] bool Empty() const noexcept {
    return size_ == 0;
  }
  [[nodiscard]] ConstReference Front() const noexcept {
    return buffer_[0];
  }
  [[nodiscard]] Reference Front() noexcept {
    return const_cast<Reference>(const_cast<const Vector&>(*this).Front());
  }
  [[nodiscard]] ConstReference Back() const noexcept {
    return buffer_[size_ - 1];
  }
  [[nodiscard]] Reference Back() noexcept {
    return const_cast<Reference>(const_cast<const Vector&>(*this).Back());
  }
  [[nodiscard]] ConstReference At(size_t idx) const {
    if (idx >= size_) {
      throw std::out_of_range("");
    }
    return (*this)[idx];
  }
  [[nodiscard]] Reference At(size_t idx) {
    return const_cast<T&>(const_cast<const Vector&>(*this).At(idx));
  }
  [[nodiscard]] ConstPointer Data() const noexcept {
    return buffer_;
  }
  [[nodiscard]] Pointer Data() noexcept {
    return const_cast<Pointer>(const_cast<const Vector&>(*this).Data());
  }
  [[nodiscard]] ConstReference operator[](size_t idx) const noexcept {
    return buffer_[idx];
  }
  [[nodiscard]] Reference operator[](size_t idx) noexcept {
    return const_cast<T&>(const_cast<const Vector&>(*this)[idx]);
  }

  void Swap(Vector& other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    std::swap(alloc_, other.alloc_);
  }
  void Clear() noexcept(std::is_nothrow_destructible_v<ValueType>) {
    for (size_t i = 0; i < size_; i++) {
      AllocTraits::destroy(alloc_, buffer_ + i);
    }
    size_ = 0;
  }
  void Resize(size_t size) {
    if (size == size_) {
      return;
    }
    if (size < size_) {
      for (size_t i = size; i < size_; i++) {
        AllocTraits::destroy(alloc_, buffer_ + i);
      }
      size_ = size;
      return;
    }
    size_t count = 0;
    if (size <= capacity_) {
      auto backup_size = size_;
      try {
        for (size_t i = size_; i < size; ++i) {
          AllocTraits::construct(alloc_, buffer_ + i);
          ++count;
        }
        size_ = size;
      } catch (...) {
        size_ = backup_size;
        for (size_t i = size_; i < size_ + count; i++) {
          AllocTraits::destroy(alloc_, buffer_ + i);
        }
        throw;
      }
      return;
    }
    if (size > capacity_) {
      auto backup_buff = buffer_;
      auto backup_size = size_;
      auto backup_capacity = capacity_;
      auto new_buff = AllocTraits::allocate(alloc_, size * 2);
      try {
        for (size_t i = size_; i < size; ++i) {
          AllocTraits::construct(alloc_, new_buff + i);
          ++count;
        }
        MoveIterTo(begin(), end(), new_buff);
        for (size_t i = 0; i < size_; i++) {
          AllocTraits::destroy(alloc_, buffer_ + i);
        }
        AllocTraits::deallocate(alloc_, buffer_, capacity_);
        capacity_ = size * 2;
        buffer_ = new_buff;
        size_ = size;
      } catch (...) {
        for (size_t i = size_; i < size_ + count; i++) {
          AllocTraits::destroy(alloc_, new_buff + i);
        }
        AllocTraits::deallocate(alloc_, new_buff, capacity_);
        capacity_ = backup_capacity;
        size_ = backup_size;
        buffer_ = backup_buff;
        throw;
      }
    }
  }
  void Resize(size_t size, const T& value) {
    if (size == size_) {
      return;
    }
    if (size < size_) {
      for (size_t i = size; i < size_; i++) {
        AllocTraits::destroy(alloc_, buffer_ + i);
      }
      size_ = size;
      return;
    }
    size_t count = 0;
    if (size <= capacity_) {
      auto backup_size = size_;
      try {
        for (size_t i = size_; i < size; ++i) {
          AllocTraits::construct(alloc_, buffer_ + i, value);
          ++count;
        }
        size_ = size;
      } catch (...) {
        for (size_t i = size_; i < size_ + count; i++) {
          AllocTraits::destroy(alloc_, buffer_ + i);
        }
        size_ = backup_size;
        throw;
      }
      return;
    }
    if (size > capacity_) {
      auto backup_buff = buffer_;
      auto backup_size = size_;
      auto backup_capacity = capacity_;
      auto new_buff = AllocTraits::allocate(alloc_, size * 2);
      try {
        for (size_t i = size_; i < size; ++i) {
          AllocTraits::construct(alloc_, new_buff + i, value);
          ++count;
        }
        MoveIterTo(begin(), end(), new_buff);
        for (size_t i = 0; i < size_; i++) {
          AllocTraits::destroy(alloc_, buffer_ + i);
        }
        AllocTraits::deallocate(alloc_, buffer_, capacity_);
        capacity_ = size * 2;
        buffer_ = new_buff;
        size_ = size;
      } catch (...) {
        for (size_t i = size_; i < size_ + count; i++) {
          AllocTraits::destroy(alloc_, new_buff + i);
        }
        AllocTraits::deallocate(alloc_, new_buff, capacity_);
        capacity_ = backup_capacity;
        size_ = backup_size;
        buffer_ = backup_buff;
        throw;
      }
    }
  }
  void Reserve(size_t capacity) {
    if (capacity_ < capacity) {
      auto backup_buff = buffer_;
      auto backup_capacity = capacity_;
      try {
        auto new_buff = AllocTraits::allocate(alloc_, capacity);
        MoveIterTo(begin(), end(), new_buff);
        for (size_t i = 0; i < size_; i++) {
          AllocTraits::destroy(alloc_, buffer_ + i);
        }
        AllocTraits::deallocate(alloc_, buffer_, capacity_);
        capacity_ = capacity;
        buffer_ = new_buff;
      } catch (...) {
        capacity_ = backup_capacity;
        buffer_ = backup_buff;
        throw;
      }
    }
  }
  void ShrinkToFit() {
    auto backup_buff = buffer_;
    auto backup_size = size_;
    auto backup_capacity = capacity_;
    try {
      if (size_ == 0) {
        AllocTraits::deallocate(alloc_, buffer_, capacity_);
        buffer_ = nullptr;
      } else {
        auto new_buff = AllocTraits::allocate(alloc_, size_);
        MoveIterTo(begin(), end(), new_buff);
        for (size_t i = 0; i < size_; i++) {
          AllocTraits::destroy(alloc_, buffer_ + i);
        }
        AllocTraits::deallocate(alloc_, buffer_, capacity_);
        buffer_ = new_buff;
      }
      capacity_ = size_;
    } catch (...) {
      capacity_ = backup_capacity;
      size_ = backup_size;
      buffer_ = backup_buff;
      throw;
    }
  }

  template <class... Args>
  void EmplaceBack(Args&&... args) {
    if (size_ < capacity_) {
      try {
        AllocTraits::construct(alloc_, buffer_ + size_, std::forward<Args>(args)...);
      } catch (...) {
        AllocTraits::destroy(alloc_, buffer_ + size_);
        throw;
      }
      ++size_;
    } else {
      auto backup_buff = buffer_;
      auto backup_size = size_;
      auto backup_capacity = capacity_;
      if (capacity_ == 0) {
        buffer_ = AllocTraits::allocate(alloc_, 1);
        try {
          AllocTraits::construct(alloc_, buffer_ + size_, std::forward<Args>(args)...);
          capacity_ = 1;
          ++size_;
        } catch (...) {
          size_ = backup_size;
          capacity_ = backup_capacity;
          AllocTraits::destroy(alloc_, buffer_ + size_);
          AllocTraits::deallocate(alloc_, buffer_, 1);
          buffer_ = backup_buff;
          throw;
        }
      } else {
        auto new_buff = AllocTraits::allocate(alloc_, capacity_ * 2);
        try {
          AllocTraits::construct(alloc_, new_buff + size_, std::forward<Args>(args)...);
          MoveIterTo(begin(), end(), new_buff);
          for (size_t i = 0; i < size_; i++) {
            AllocTraits::destroy(alloc_, buffer_ + i);
          }
          AllocTraits::deallocate(alloc_, buffer_, capacity_);
          buffer_ = new_buff;
          capacity_ = capacity_ * 2;
          ++size_;
        } catch (...) {
          size_ = backup_size;
          capacity_ = backup_capacity;
          AllocTraits::destroy(alloc_, new_buff + size_);
          AllocTraits::deallocate(alloc_, new_buff, capacity_ * 2);
          buffer_ = backup_buff;
          throw;
        }
      }
    }
  }

  void PushBack(const T& value) {
    EmplaceBack(value);
  }
  void PushBack(T&& value) {
    EmplaceBack(std::move(value));
  }
  void PopBack() noexcept(std::is_nothrow_destructible_v<ValueType>) {
    if (!Empty()) {
      --size_;
      AllocTraits::destroy(alloc_, buffer_ + size_);
    }
  }

  [[nodiscard]] ConstIterator cbegin() const noexcept {  // NOLINT
    return Data();
  }
  [[nodiscard]] ConstIterator begin() const noexcept {  // NOLINT
    return cbegin();
  }
  [[nodiscard]] Iterator begin() noexcept {  // NOLINT
    return Data();
  }
  [[nodiscard]] ConstIterator cend() const noexcept {  // NOLINT
    return Data() + size_;
  }
  [[nodiscard]] ConstIterator end() const noexcept {  // NOLINT
    return cend();
  }
  [[nodiscard]] Iterator end() noexcept {  // NOLINT
    return Data() + size_;
  }
  [[nodiscard]] ConstReverseIterator crbegin() const noexcept {  // NOLINT
    return ConstReverseIterator(cend());
  }
  [[nodiscard]] ConstReverseIterator rbegin() const noexcept {  // NOLINT
    return crbegin();
  }
  [[nodiscard]] ReverseIterator rbegin() noexcept {  // NOLINT
    return ReverseIterator(end());
  }
  [[nodiscard]] ConstReverseIterator crend() const noexcept {  // NOLINT
    return ConstReverseIterator(cbegin());
  }
  [[nodiscard]] ConstReverseIterator rend() const noexcept {  // NOLINT
    return crend();
  }
  [[nodiscard]] ReverseIterator rend() noexcept {  // NOLINT
    return ReverseIterator(begin());
  }

 private:
  using AllocTraits = std::allocator_traits<Allocator>;

  template <typename MoveIterator>
  void MoveIterRange(MoveIterator begin, MoveIterator end) {
    std::move_iterator<MoveIterator> mbegin(begin);
    std::move_iterator<MoveIterator> mend(end);
    CopyIterRange(mbegin, mend);
  }

  template <typename CopyIterator>
  void CopyIterRange(CopyIterator begin, CopyIterator end) {
    size_t curr_size = 0;
    try {
      for (auto iter = begin; iter != end; ++iter) {
        AllocTraits::construct(alloc_, buffer_ + curr_size, *iter);
        ++curr_size;
      }
    } catch (...) {
      for (size_t i = 0; i < curr_size; i++) {
        AllocTraits::destroy(alloc_, buffer_ + i);
      }
      throw;
    }
  }

  template <typename MoveIterator>
  void MoveIterTo(MoveIterator begin, MoveIterator end, Pointer buffer) {
    std::move_iterator<MoveIterator> mbegin(begin);
    std::move_iterator<MoveIterator> mend(end);
    auto curr_size = 0;
    try {
      for (auto iter = mbegin; iter != mend; ++iter) {
        AllocTraits::construct(alloc_, buffer + curr_size, *iter);
        ++curr_size;
      }
    } catch (...) {
      throw;
    }
  }

 private:
  Allocator alloc_{Allocator{}};
  T* buffer_{nullptr};
  size_t size_{0};
  size_t capacity_{0};
};

template <typename T, class Alloc>
[[nodiscard]] bool operator==(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) noexcept {
  if (lhs.Size() != rhs.Size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.Size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template <typename T, class Alloc>
[[nodiscard]] bool operator<(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) noexcept {
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, class Alloc>
[[nodiscard]] bool operator!=(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) noexcept {
  return !(lhs == rhs);
}

template <typename T, class Alloc>
[[nodiscard]] bool operator>(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) noexcept {
  return rhs < lhs;
}

template <typename T, class Alloc>
[[nodiscard]] bool operator<=(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) noexcept {
  return !(lhs > rhs);
}

template <typename T, class Alloc>
[[nodiscard]] bool operator>=(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) noexcept {
  return !(lhs < rhs);
}

#endif  // OOP_ASSIGNMENTS_VECTOR_VECTOR_H_
