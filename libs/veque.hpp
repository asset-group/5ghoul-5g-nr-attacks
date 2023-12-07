/*
 * veque.hpp
 *
 * Efficient generic C++ container combining useful features of std::vector and std::deque
 *
 * Copyright (C) 2019 Drew Dormann
 *
 */

#ifndef VEQUE_HEADER_GUARD
#define VEQUE_HEADER_GUARD

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <ratio>
#include <string>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace veque
{
    // Very fast resizing behavior
    struct fast_resize_traits
    {
        // Relative to size(), amount of unused space to reserve when reallocating
        using allocation_before_front = std::ratio<1>;
        using allocation_after_back = std::ratio<1>;

        // If true, arbitrary insert and erase operations are twice the speed of
        // std::vector, but those operations invalidate all iterators
        static constexpr auto resize_from_closest_side = true;
    };

    // Match std::vector iterator invalidation rules
    struct vector_compatible_resize_traits
    {
        // Relative to size(), amount of unused space to reserve when reallocating
        using allocation_before_front = std::ratio<1>;
        using allocation_after_back = std::ratio<1>;

        // If false, veque is a 100% compatible drop-in replacement for
        // std::vector including iterator invalidation rules
        static constexpr auto resize_from_closest_side = false;
    };

    // Resizing behavior resembling std::vector.  Also ideal for queue-like push_back/pop_front behavior.
    struct std_vector_traits
    {
        // Reserve storage only at back, like std::vector
        using allocation_before_front = std::ratio<0>;
        using allocation_after_back = std::ratio<1>;

        // Same iterator invalidation rules as std::vector
        static constexpr auto resize_from_closest_side = false;
    };

    // Never reallocate more storage than is needed
    struct no_reserve_traits
    {
        // Any operation requiring a greater size reserves only that size
        using allocation_before_front = std::ratio<0>;
        using allocation_after_back = std::ratio<0>;

        // Same iterator invalidation rules as std::vector
        static constexpr auto resize_from_closest_side = false;
    };

    template< typename T, typename ResizeTraits = fast_resize_traits, typename Allocator = std::allocator<T> >
    class veque
    {
    public:
        
        // Types
        using allocator_type = Allocator;
        using alloc_traits = std::allocator_traits<allocator_type>;
        using value_type = T;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = T *;
        using const_iterator = const T *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;
        using ssize_type = std::ptrdiff_t;

        // Common member functions
        veque() noexcept ( noexcept(Allocator()) )
            : veque( Allocator() )
        {
        }

        explicit veque( const Allocator& alloc ) noexcept
            : _data { 0, alloc }
        {
        }

        explicit veque( size_type n, const Allocator& alloc = Allocator() )
            : veque( _allocate_uninitialized_tag{}, n, alloc )
        {
            _value_construct_range( begin(), end() );
        }

        veque( size_type n, const T &value, const Allocator& alloc = Allocator() )
            : veque( _allocate_uninitialized_tag{}, n, alloc )
        {
            _value_construct_range( begin(), end(), value );
        }

        template< typename InputIt, typename ItCat = typename std::iterator_traits<InputIt>::iterator_category >
        veque( InputIt b,  InputIt e, const Allocator& alloc = Allocator() )
            : veque( b, e, alloc, ItCat{} )
        {
        }

        veque( std::initializer_list<T> lst, const Allocator& alloc = Allocator() )
            : veque( _allocate_uninitialized_tag{}, lst.size(), alloc )
        {
            _copy_construct_range( lst.begin(), lst.end(), begin() );
        }

        veque( const veque & other )
            : veque( _allocate_uninitialized_tag{}, other.size(), alloc_traits::select_on_container_copy_construction( other._allocator() ) )
        {
            _copy_construct_range( other.begin(), other.end(), begin() );
        }

        template< typename OtherResizeTraits >
        veque( const veque<T,OtherResizeTraits,Allocator> & other )
            : veque( _allocate_uninitialized_tag{}, other.size(), alloc_traits::select_on_container_copy_construction( other._allocator() ) )
        {
            _copy_construct_range( other.begin(), other.end(), begin() );
        }

        template< typename OtherResizeTraits >
        veque( const veque<T,OtherResizeTraits,Allocator> & other, const Allocator & alloc )
            : veque( _allocate_uninitialized_tag{}, other.size(), alloc )
        {
            _copy_construct_range( other.begin(), other.end(), begin() );
        }

        veque( veque && other ) noexcept
        {
            _swap_with_allocator( std::move(other) );
        }

        template< typename OtherResizeTraits >
        veque( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            _swap_with_allocator( std::move(other) );
        }

        template< typename OtherResizeTraits >
        veque( veque<T,OtherResizeTraits,Allocator> && other, const Allocator & alloc ) noexcept
            : veque( alloc )
        {
            if constexpr ( !alloc_traits::is_always_equal::value )
            {
                if ( alloc != other._allocator() )
                {
                    // Incompatible allocators.  Allocate new storage.
                    auto replacement = veque( _allocate_uninitialized_tag{}, other.size(), alloc );
                    _nothrow_move_construct_range( other.begin(), other.end(), replacement.begin() );
                    _swap_without_allocator( std::move(replacement) );
                    return;
                }
            }
            _swap_without_allocator( std::move(other) );
        }

        ~veque()
        {
            _destroy( begin(), end() );
        }

        veque & operator=( const veque & other )
        {
            return _copy_assignment( other );
        }

        template< typename OtherResizeTraits >
        veque & operator=( const veque<T,OtherResizeTraits,Allocator> & other )
        {
            return _copy_assignment( other );
        }

        veque & operator=( veque && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            return _move_assignment( std::move(other) );
        }

        template< typename OtherResizeTraits >
        veque & operator=( veque<T,OtherResizeTraits,Allocator> && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            return _move_assignment( std::move(other) );
        }

        veque & operator=( std::initializer_list<T> lst )
        {
            _assign( lst.begin(), lst.end() );
            return *this;
        }

        void assign( size_type count, const T &value )
        {
            if ( count > capacity_full() )
            {
                _swap_without_allocator( veque( count, value, _allocator() ) );
            }
            else
            {
                _reassign_existing_storage( count, value );
            }
        }

        template< typename InputIt, typename ItCat = typename std::iterator_traits<InputIt>::iterator_category >
        void assign( InputIt b, InputIt e )
        {
            _assign( b, e, ItCat{} );
        }

        void assign( std::initializer_list<T> lst )
        {
            _assign( lst.begin(), lst.end() );
        }

        allocator_type get_allocator() const
        {
            return _allocator();
        }

        // Element access
        reference at( size_type idx )
        {
            if ( idx >= size() )
            {
                throw std::out_of_range("veque<T,ResizeTraits,Alloc>::at(" + std::to_string(idx) + ") out of range");
            }
            return (*this)[idx];
        }

        const_reference at( size_type idx ) const
        {
            if ( idx >= size() )
            {
                throw std::out_of_range("veque<T,ResizeTraits,Alloc>::at(" + std::to_string(idx) + ") out of range");
            }
            return (*this)[idx];
        }

        reference operator[]( size_type idx )
        {
            return *(begin() + idx);
        }

        const_reference operator[]( size_type idx ) const
        {
            return *(begin() + idx);
        }

        reference front()
        {
            return (*this)[0];
        }

        const_reference front() const
        {
            return (*this)[0];
        }

        reference back()
        {
            return (*this)[size() - 1];
        }

        const_reference back() const
        {
            return (*this)[size() - 1];
        }

        T * data() noexcept
        {
            return begin();
        }

        const T * data() const noexcept
        {
            return begin();
        }

        // Iterators
        const_iterator cbegin() const noexcept
        {
            return _storage_begin() + _offset;
        }

        iterator begin() noexcept
        {
            return _storage_begin() + _offset;
        }

        const_iterator begin() const noexcept
        {
            return cbegin();
        }

        const_iterator cend() const noexcept
        {
            return _storage_begin() + _offset + size();
        }

        iterator end() noexcept
        {
            return _storage_begin() + _offset + size();
        }

        const_iterator end() const noexcept
        {
            return cend();
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(cend());
        }

        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return crbegin();
        }

        const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(cbegin());
        }

        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        const_reverse_iterator rend() const noexcept
        {
            return crend();
        }

        // Capacity
        [[nodiscard]] bool empty() const noexcept
        {
            return size() == 0;
        }

        size_type size() const noexcept
        {
            return _size;
        }

        ssize_type ssize() const noexcept
        {
            return _size;
        }

        size_type max_size() const noexcept
        {
            constexpr auto compile_time_limit = std::min(
                // The ssize type's ceiling
                std::numeric_limits<ssize_type>::max() / sizeof(T),
                // Ceiling imposed by std::ratio math
                std::numeric_limits<size_type>::max() / _full_realloc::num
            );

            // The allocator's ceiling
            auto runtime_limit = alloc_traits::max_size(_allocator() );

            return std::min( compile_time_limit, runtime_limit );
        }

        // Reserve front and back capacity, in one operation.
        void reserve( size_type front, size_type back )
        {
            if ( front > capacity_front() || back > capacity_back() )
            {
                auto allocated_before_begin = std::max( capacity_front(), front ) - size();
                auto allocated_after_begin = std::max( capacity_back(), back );
                auto new_full_capacity = allocated_before_begin + allocated_after_begin;
                
                if ( new_full_capacity > max_size() )
                {
                    throw std::length_error("veque<T,ResizeTraits,Alloc>::reserve(" + std::to_string(front) + ", " + std::to_string(back) + ") exceeds max_size()");
                }
                _reallocate( new_full_capacity, allocated_before_begin );
            }
        }

        void reserve_front( size_type count )
        {
            reserve( count, 0 );
        }

        void reserve_back( size_type count )
        {
            reserve( 0, count );
        }

        void reserve( size_type count )
        {
            reserve( count, count );
        }

        // Returns current size + unused allocated storage before front()
        size_type capacity_front() const noexcept
        {
            return _offset + size();
        }

        // Returns current size + unused allocated storage after back()
        size_type capacity_back() const noexcept
        {
            return capacity_full() - _offset;
        }

        // Returns current size + all unused allocated storage
        size_type capacity_full() const noexcept
        {
            return _data._allocated;
        }

        // To achieve interface parity with std::vector, capacity() returns capacity_back();
        size_type capacity() const noexcept
        {
            return capacity_back();
        }

        void shrink_to_fit()
        {
            if ( size() < capacity_full() )
            {
                _reallocate( size(), 0 );
            }
        }

        // Modifiers
        void clear() noexcept
        {
            _destroy( begin(), end() );
            _size = 0;
            _offset = 0;
            if constexpr ( std::ratio_greater_v<_unused_realloc, std::ratio<0>> )
            {
                using unused_front_ratio = std::ratio_divide<_front_realloc,_unused_realloc>;
                _offset = capacity_full() * unused_front_ratio::num / unused_front_ratio::den;
            }
        }

        iterator insert( const_iterator it, const T & value )
        {
            return emplace( it, value );
        }

        iterator insert( const_iterator it, T && value )
        {
            return emplace( it, std::move(value) );
        }

        iterator insert( const_iterator it, size_type count, const T & value )
        {
            auto res = _insert_storage( it, count );
            _value_construct_range( res, res + count, value );
            return res;
        }

        template< typename InputIt, typename ItCat = typename std::iterator_traits<InputIt>::iterator_category >
        iterator insert( const_iterator it, InputIt b, InputIt e )
        {
            return _insert( it, b, e, ItCat{} );
        }

        iterator insert( const_iterator it, std::initializer_list<T> lst )
        {
            return insert( it, lst.begin(), lst.end() );
        }

        template< typename ...Args >
        iterator emplace( const_iterator it, Args && ... args )
        {
            auto res = _insert_storage( it, 1 );
            alloc_traits::construct( _allocator(), res, std::forward<Args>(args)... );
            return res;
        }

        iterator erase( const_iterator it )
        {
            return erase( it, std::next(it) );
        }

        iterator erase( const_iterator b, const_iterator e )
        {
            auto count = std::distance( b, e );
            if constexpr ( _resize_from_closest_side )
            {
                auto elements_before = std::distance( cbegin(), b );
                auto elements_after = std::distance( e, cend( ) );
                if (  elements_before < elements_after )
                {
                    _shift_back( begin(), b, count );
                    _move_begin( count );
                    return _mutable_iterator(e);
                }
            }
            _shift_front( e, end(), count );
            _move_end(-count);
            return _mutable_iterator(b);
        }

        void push_back( const T & value )
        {
            emplace_back( value );
        }

        void push_back( T && value )
        {
            emplace_back( std::move(value) );
        }

        template< typename ... Args>
        reference emplace_back( Args && ...args )
        {
            if ( size() == capacity_back() )
            {
                _reallocate_space_at_back( size() + 1 );
            }
            alloc_traits::construct( _allocator(), end(), std::forward<Args>(args)... );
            _move_end( 1 );
            return back();
        }

        void push_front( const T & value )
        {
            emplace_front( value );
        }

        void push_front( T && value )
        {
            emplace_front( std::move(value) );
        }

        template< typename ... Args>
        reference emplace_front( Args && ...args )
        {
            if ( size() == capacity_front() )
            {
                _reallocate_space_at_front( size() + 1 );
            }
            alloc_traits::construct( _allocator(), begin()-1, std::forward<Args>(args)... );
            _move_begin( -1 );
            return front();
        }

        void pop_back()
        {
            alloc_traits::destroy( _allocator(), &back() );
            _move_end( -1 );
        }

        // Move-savvy pop back with strong exception guarantee
        T pop_back_element()
        {
            auto res( _nothrow_construct_move(back()) );
            pop_back();
            return res;
        }

        void pop_front()
        {
            alloc_traits::destroy( _allocator(), &front() );
            _move_begin( 1 );
        }

        // Move-savvy pop front with strong exception guarantee
        T pop_front_element()
        {
            auto res( _nothrow_construct_move(front()) );
            pop_front();
            return res;
        }

        // Resizes the veque, by adding or removing from the front. 
        void resize_front( size_type count )
        {
            _resize_front( count );
        }

        void resize_front( size_type count, const T & value )
        {
            _resize_front( count, value );
        }

        // Resizes the veque, by adding or removing from the back.
        void resize_back( size_type count )
        {
            _resize_back( count );
        }

        void resize_back( size_type count, const T & value )
        {
            _resize_back( count, value );
        }

        // To achieve interface parity with std::vector, resize() performs resize_back();
        void resize( size_type count )
        {
            _resize_back( count );
        }

        void resize( size_type count, const T & value )
        {
            _resize_back( count, value );
        }

        template< typename OtherResizeTraits >
        void swap( veque<T,OtherResizeTraits,Allocator> & other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_swap::value
            || alloc_traits::is_always_equal::value))
        {
            if constexpr ( alloc_traits::propagate_on_container_swap::value )
            {
                _swap_with_allocator( std::move(other) );
            }
            else
            {
                if ( _allocator() == other._allocator() )
                {
                    _swap_without_allocator( std::move(other) );
                }
                else
                {
                    // std::vector would declare this UB.  Allocate compatible storage and make it work.
                    auto new_this = veque( _allocate_uninitialized_tag{}, other.size(), _allocator() );
                    _nothrow_move_construct_range( other.begin(), other.end(), new_this.begin() );

                    auto new_other = veque( _allocate_uninitialized_tag{}, size(), other._allocator() );
                    _nothrow_move_construct_range( begin(), end(), new_other.begin() );

                    _swap_without_allocator( std::move(new_this) );
                    other._swap_without_allocator( std::move(new_other) );
                }
            }
        }

    private:

        using _front_realloc = typename ResizeTraits::allocation_before_front::type;
        using _back_realloc = typename ResizeTraits::allocation_after_back::type;
        using _unused_realloc = std::ratio_add< _front_realloc, _back_realloc >;
        using _full_realloc = std::ratio_add< std::ratio<1>, _unused_realloc >;

        static constexpr auto _resize_from_closest_side = ResizeTraits::resize_from_closest_side;

        static_assert( _front_realloc::den > 0  );
        static_assert( _back_realloc::den > 0  );
        static_assert( std::ratio_greater_equal_v<_front_realloc,std::ratio<0>>, "Reserving negative space is not well-defined" );
        static_assert( std::ratio_greater_equal_v<_back_realloc,std::ratio<0>>, "Reserving negative space is not well-defined" );
        static_assert( std::ratio_greater_equal_v<_unused_realloc,std::ratio<0>>, "Reserving negative space is not well-defined" );

        // Confirmation that allocator_traits will only directly call placement new(ptr)T()
        static constexpr auto _calls_default_constructor_directly = 
            std::is_same_v<allocator_type,std::allocator<T>>;
        // Confirmation that allocator_traits will only directly call placement new(ptr)T(const T&)
        static constexpr auto _calls_copy_constructor_directly = 
            std::is_same_v<allocator_type,std::allocator<T>>;
        // Confirmation that allocator_traits will only directly call ~T()
        static constexpr auto _calls_destructor_directly =
            std::is_same_v<allocator_type,std::allocator<T>>;

        size_type _size = 0;    // Number of elements in use
        size_type _offset = 0;  // Number of uninitialized elements before begin()

        // Deriving from allocator to leverage empty base optimization
        struct Data : Allocator
        {
            T *_storage = nullptr;
            size_type _allocated = 0;

            Data() = default;
            Data( size_type size, const Allocator & alloc )
                : Allocator{alloc}
                , _storage{size ? std::allocator_traits<Allocator>::allocate( allocator(), size ) : nullptr}
                , _allocated{size}
            {
            }
            Data( const Data& ) = delete;
            Data( Data && other )
            {
                *this = std::move(other);
            }
            ~Data()
            {
                if ( _storage )
                {
                    std::allocator_traits<Allocator>::deallocate( allocator(), _storage, _allocated );
                }
            }
            Data& operator=( const Data & ) = delete;
            Data& operator=( Data && other )
            {
                using std::swap;
                if constexpr( ! std::is_empty_v<Allocator> )
                {
                    swap(allocator(), other.allocator());
                }
                swap(_allocated,  other._allocated);
                swap(_storage,    other._storage);
                return *this;
            }
            Allocator& allocator() { return *this; }
            const Allocator& allocator() const { return *this; }
        } _data;

        template< typename InputIt >
        veque( InputIt b, InputIt e, const Allocator & alloc, std::input_iterator_tag )
            : veque{}
        {
            for ( ; b != e; ++b )
            {
                push_back( *b );
            }
        }

        template< typename InputIt >
        veque( InputIt b, InputIt e, const Allocator & alloc, std::forward_iterator_tag )
            : veque( _allocate_uninitialized_tag{}, std::distance( b, e ), alloc )
        {
            _copy_construct_range( b, e, begin() );
        }

        // Private tag to indicate initial allocation
        struct _allocate_uninitialized_tag {};
        // Private tag to indicate resizing allocation
        struct _reallocate_uninitialized_tag {};

        // Create an uninitialized empty veque, with specified storage params
        veque( _allocate_uninitialized_tag, size_type size, size_type allocated, size_type offset, const Allocator & alloc )
            : _size{ size }
            , _offset{ offset }
            , _data { allocated, alloc }
        {
        }

        // Create an uninitialized empty veque, with storage for expected size
        veque( _allocate_uninitialized_tag, size_type size, const Allocator & alloc )
            : veque( _allocate_uninitialized_tag{}, size, size, 0, alloc )
        {
        }

        // Create an uninitialized empty veque, with storage for expected reallocated size
        veque( _reallocate_uninitialized_tag, size_type size, const Allocator & alloc )
            : veque( _allocate_uninitialized_tag{}, size, _calc_reallocation(size), _calc_offset(size), alloc )
        {
        }

        static constexpr size_type _calc_reallocation( size_type size )
        {
            return size * _full_realloc::num / _full_realloc::den;
        }

        static constexpr size_type _calc_offset( size_type size )
        {
            return size * _front_realloc::num / _front_realloc::den;
        }

        // Acquire Allocator
        Allocator& _allocator() noexcept
        {
            return _data.allocator();
        }

        const Allocator& _allocator() const noexcept
        {
            return _data.allocator();
        }

        // Destroy elements in range
        void _destroy( const_iterator b, const_iterator e )
        {
            if constexpr ( std::is_trivially_destructible_v<T> && _calls_destructor_directly )
            {
                (void)b; (void)e; // Unused
            }
            else
            {
                auto start = _mutable_iterator(b);
                for ( auto i = start; i != e; ++i )
                {
                    alloc_traits::destroy( _allocator(), i );
                }
            }
        }

        template< typename OtherResizeTraits >
        veque & _copy_assignment( const veque<T,OtherResizeTraits,Allocator> & other )
        {
            if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value )
            {
                if constexpr ( !alloc_traits::is_always_equal::value )
                {
                    if ( other._allocator() != _allocator() || other.size() > capacity_full() )
                    {
                        _swap_with_allocator( veque( other, other._allocator() ) );
                        return *this;
                    }
                }
            }
            if ( other.size() > capacity_full() )
            {
                _swap_without_allocator( veque( other, _allocator() ) );
            }
            else
            {
                _reassign_existing_storage( other.begin(), other.end() );
            }
            return *this;
        }

        template< typename OtherResizeTraits >
        veque & _move_assignment( veque<T,OtherResizeTraits,Allocator> && other ) noexcept(
            noexcept(alloc_traits::propagate_on_container_move_assignment::value
            || alloc_traits::is_always_equal::value) )
        {
            if constexpr ( !alloc_traits::is_always_equal::value )
            {
                if ( _allocator() != other._allocator() )
                {
                    if constexpr ( alloc_traits::propagate_on_container_move_assignment::value )
                    {
                        _swap_with_allocator( std::move(other) );
                    }
                    else
                    {
                        if ( other.size() > capacity_full() )
                        {
                            _swap_without_allocator( veque( std::move(other), _allocator() ) );
                        }
                        else
                        {
                            _reassign_existing_storage( std::move_iterator(other.begin()), std::move_iterator(other.end()) );
                        }
                    }
                    return *this;
                }
            }
            _swap_without_allocator( std::move(other) );
            return *this;
        }

        // Construct elements in range
        template< typename ...Args >
        void _value_construct_range( const_iterator b, const_iterator e, const Args & ...args )
        {
            static_assert( sizeof...(args) <= 1, "This is for default- or copy-constructing" );

            if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_default_constructor_directly )
            {
                if constexpr ( sizeof...(args) == 0 )
                {
                    std::memset( _mutable_iterator(b), 0, std::distance( b, e ) * sizeof(T) );
                }
                else
                {
                    std::fill( _mutable_iterator(b), _mutable_iterator(e), args...);
                }
            }
            else
            {
                for ( auto dest = _mutable_iterator(b); dest != e; ++dest )
                {
                    alloc_traits::construct( _allocator(), dest, args... );
                }
            }
        }

        template< typename It >
        void _copy_construct_range( It b, It e, const_iterator dest )
        {
            static_assert( std::is_convertible_v<typename std::iterator_traits<It>::iterator_category,std::forward_iterator_tag> );
            if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
            {
                std::memcpy( _mutable_iterator(dest), b, std::distance( b, e ) * sizeof(T) );
            }
            else
            {
                for ( ; b != e; ++dest, ++b )
                {
                    alloc_traits::construct( _allocator(), dest, *b );
                }
            }
        }
        
        template< typename It >
        void _assign( It b, It e )
        {
            static_assert( std::is_convertible_v<typename std::iterator_traits<It>::iterator_category,std::forward_iterator_tag> );
            if ( std::distance( b, e ) > static_cast<difference_type>(capacity_full()) )
            {
                _swap_without_allocator( veque( b, e, _allocator() ) );
            }
            else
            {
                _reassign_existing_storage( b, e );
            }
        }

        template< typename It >
        void _assign( It b, It e, std::forward_iterator_tag )
        {
            _assign( b, e );
        }

        template< typename It >
        void _assign( It b, It e, std::input_iterator_tag )
        {
            // Input Iterators require a single-pass solution
            clear();
            for ( ; b != e; ++b )
            {
                push_back( *b );
            }
        }

        template< typename It >
        iterator _insert( const_iterator it, It b, It e )
        {
            static_assert( std::is_convertible_v<typename std::iterator_traits<It>::iterator_category,std::forward_iterator_tag> );
            auto res = _insert_storage( it, std::distance( b, e ) );
            _copy_construct_range( b, e, res );
            return res;
        }

        template< typename It >
        iterator _insert( const_iterator it, It b, It e, std::forward_iterator_tag )
        {
            return _insert( it, b, e );
        }

        template< typename It >
        iterator _insert( const_iterator it, It b, It e, std::input_iterator_tag )
        {
            // Input Iterators require a single-pass solution
            auto allocated = veque( b, e );
            _insert( it, allocated.begin(), allocated.end() );
        }

        template< typename OtherResizeTraits >
        void _swap_with_allocator( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            // Swap everything
            std::swap( _size,      other._size );
            std::swap( _offset,    other._offset );
            std::swap( _data,      other._data );
        }

        template< typename OtherResizeTraits >
        void _swap_without_allocator( veque<T,OtherResizeTraits,Allocator> && other ) noexcept
        {
            // Don't swap _data.allocator().
            std::swap( _size,            other._size );
            std::swap( _offset,          other._offset );
            std::swap( _data._allocated, other._data._allocated);
            std::swap( _data._storage,   other._data._storage);
        }

        template< typename ...Args >
        void _resize_front( size_type count, const Args & ...args )
        {
            difference_type delta = count - size();
            if ( delta > 0 )
            {
                if ( count > capacity_front() )
                {
                    _reallocate_space_at_front( count );
                }
                _value_construct_range( begin() - delta, begin(), args... );
            }
            else
            {
                _destroy( begin(), begin() - delta );
            }
            _move_begin( -delta );
        }

        template< typename ...Args >
        void _resize_back( size_type count, const Args & ...args )
        {
            difference_type delta = count - size();
            if ( delta > 0 )
            {
                if ( count > capacity_back() )
                {
                    _reallocate_space_at_back( count );
                }
                _value_construct_range( end(), end() + delta, args... );
            }
            else
            {
                _destroy( end() + delta, end() );
            }
            _move_end( delta );
        }

        // Move veque to new storage, with specified capacity...
        // ...and yet-unused space at back of this storage
        void _reallocate_space_at_back( size_type count )
        {
            auto storage_needed = _calc_reallocation(count);
            auto current_capacity = capacity_full();
            auto new_offset = _calc_offset(count);
            if ( storage_needed <= current_capacity )
            {
                // Shift elements toward front
                auto distance = _offset - new_offset;
                _shift_front( begin(), end(), distance  );
                _move_begin(-distance);
                _move_end(-distance);
            }
            else
            {
                _reallocate( storage_needed, new_offset );
            }
        }
        
        // ...and yet-unused space at front of this storage
        void _reallocate_space_at_front( size_type count )
        {
            auto storage_needed = _calc_reallocation(count);
            auto current_capacity = capacity_full();
            auto new_offset = count - size() + _calc_offset(count);
            if ( storage_needed <= current_capacity )
            {
                // Shift elements toward back
                auto distance = new_offset - _offset;
                _shift_back( begin(), end(), distance );
                _move_begin(distance);
                _move_end(distance);
            }
            else
            {
                _reallocate( storage_needed, new_offset );
            }
        }
        
        // Move veque to new storage, with specified capacity
        void _reallocate( size_type allocated, size_type offset )
        {
            auto replacement = veque( _allocate_uninitialized_tag{}, size(), allocated, offset, _allocator() );
            _nothrow_move_construct_range( begin(), end(), replacement.begin() );
            _swap_without_allocator( std::move(replacement) );
        }

        // Insert empty space, choosing the most efficient way to shift existing elements
        iterator _insert_storage( const_iterator it, size_type count )
        {
            auto required_size = size() + count;
            auto can_shift_back = capacity_back() >= required_size;
            if constexpr ( std::ratio_greater_v<_front_realloc,std::ratio<0>> )
            {
                if ( can_shift_back && it == begin() )
                {
                    // Don't favor shifting entire contents back
                    // if realloc will create space
                    can_shift_back = false;
                }
            }

            if constexpr ( _resize_from_closest_side )
            {
                auto can_shift_front = capacity_front() >= required_size;
                if constexpr ( std::ratio_greater_v<_back_realloc,std::ratio<0>> )
                {
                    if ( can_shift_front && it == end() )
                    {
                        // Don't favor shifting entire contents front
                        // if realloc will create space
                        can_shift_front = false;
                    }
                }

                if ( can_shift_back && can_shift_front)
                {
                    // Capacity allows shifting in either direction.
                    // Remove the choice with the greater operation count.
                    auto index = std::distance( cbegin(), it );
                    if ( index <= ssize() / 2 )
                    {
                        can_shift_back = false;
                    }
                    else
                    {
                        can_shift_front = false;
                    }
                }

                if ( can_shift_front )
                {
                    _shift_front( begin(), it, count );
                    _move_begin( -count );
                    return _mutable_iterator(it) - count;
                }
            }
            if ( can_shift_back )
            {
                _shift_back( it, end(), count );
                _move_end( count );
                return _mutable_iterator(it);
            }

            // Insufficient capacity.  Allocate new storage.
            auto replacement = veque( _reallocate_uninitialized_tag{}, required_size, _allocator() );
            auto index = std::distance( cbegin(), it );
            auto insertion_point = begin() + index;

            _nothrow_move_construct_range( begin(), insertion_point, replacement.begin() );
            _nothrow_move_construct_range( insertion_point, end(), replacement.begin() + index + count );
            _swap_with_allocator( std::move(replacement) );
            return begin() + index;
        }

        // Moves a valid subrange in the front direction.
        // Veque will grow, if range moves past begin().
        // Veque will shrink if range includes end().
        // Returns iterator to beginning of destructed gap
        void _shift_front( const_iterator b, const_iterator e, size_type count )
        {
            if ( e == begin() )
            {
                return;
            }
            auto element_count = std::distance( b, e );
            auto start = _mutable_iterator(b);
            if ( element_count > 0 )
            {
                auto dest = start - count;
                if constexpr ( std::is_trivially_copyable_v<T> && std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    std::memmove( dest, start, element_count * sizeof(T) );
                }
                else
                {
                    auto src = start;
                    auto dest_construct_end = std::min( begin(), _mutable_iterator(e) - count );
                    for ( ; dest < dest_construct_end; ++src, ++dest )
                    {
                        _nothrow_move_construct( dest, src );
                    }
                    for ( ; src != e; ++src, ++dest )
                    {
                        _nothrow_move_assign( dest, src );
                    }
                }
            }
            _destroy( std::max( cbegin(), e - count ), e );
        }

        // Moves a range towards the back.  Veque will grow, if needed.  Vacated elements are destructed.
        // Moves a valid subrange in the back direction.
        // Veque will grow, if range moves past end().
        // Veque will shrink if range includes begin().
        // Returns iterator to beginning of destructed gap
        void _shift_back( const_iterator b, const_iterator e, size_type count )
        {
            auto start = _mutable_iterator(b); 
            if ( b == end() )
            {
                return;
            }
            auto element_count = std::distance( b, e );
            if ( element_count > 0 )
            {
                if constexpr ( std::is_trivially_copyable_v<T> && std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    std::memmove( start + count, start, element_count * sizeof(T) );
                }
                else
                {
                    auto src = _mutable_iterator(e-1);
                    auto dest = src + count;
                    auto dest_construct_end = std::max( end()-1, dest - element_count );
                    for ( ; dest > dest_construct_end; --src, --dest )
                    {
                        // Construct to destinations at or after end()
                        _nothrow_move_construct( dest, src );
                    }
                    for ( ; src != b-1; --src, --dest )
                    {
                        // Assign to destinations before before end()
                        _nothrow_move_assign( dest, src );
                    }
                }
            }
            _destroy( b, std::min( cend(), b + count ) );
        }

        // Assigns a fitting range of new elements to currently held storage.
        // Favors copying over constructing firstly, and positioning the new elements
        // at the center of storage secondly
        template< typename It >
        void _reassign_existing_storage( It b, It e )
        {
            static_assert( std::is_convertible_v<typename std::iterator_traits<It>::iterator_category,std::forward_iterator_tag> );

            auto count = std::distance( b, e );
            auto size_delta = static_cast<difference_type>( count - size() );
            // The "ideal" begin would put the new data in the center of storage
            auto ideal_begin = _storage_begin() + (capacity_full() - count) / 2;

            if ( size() == 0 )
            {
                // Existing veque is empty.  Construct at the ideal location
                _copy_construct_range( b, e, ideal_begin );        
            }
            else if ( size_delta == 0 )
            {
                // Existing veque is the same size.  Avoid any construction by copy-assigning everything
                std::copy( b, e, begin() );
                return;
            }
            else if ( size_delta < 0 )
            {
                // New size is smaller.  Copy-assign everything, placing results as close to center as possible
                ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

                _destroy( begin(), ideal_begin );
                auto ideal_end = std::copy( b, e, ideal_begin );
                _destroy( ideal_end, end() );
            }
            else
            {
                // New size is larger.  Copy-assign all existing elements, placing newly
                // constructed elements so final store is as close to center as possible
                ideal_begin = std::clamp( ideal_begin, end() - count, begin() );

                auto src = b;
                auto copy_src = src + std::distance( ideal_begin, begin() );
                _copy_construct_range( src, copy_src, ideal_begin );
                std::copy( copy_src, copy_src + ssize(), begin() );
                _copy_construct_range( copy_src + ssize(), e, end() );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }

        void _reassign_existing_storage( size_type count, const T & value )
        {
            auto size_delta = static_cast<difference_type>( count - size() );
            auto ideal_begin = _storage_begin();
            // The "ideal" begin would put the new data in the center of storage
            if constexpr ( std::ratio_greater_v<_unused_realloc, std::ratio<0>> )
            {
                using ideal_begin_ratio = std::ratio_divide<_front_realloc, _unused_realloc >;
                ideal_begin += (capacity_full() - count) * ideal_begin_ratio::num / ideal_begin_ratio::den;
            }

            if ( size() == 0 )
            {
                // Existing veque is empty.  Construct at the ideal location
                _value_construct_range( ideal_begin, ideal_begin + count, value );
            }
            else if ( size_delta == 0 )
            {
                // Existing veque is the same size.  Avoid any construction by copy-assigning everything
                std::fill( begin(), end(), value );
                return;
            }
            else if ( size_delta < 0 )
            {
                // New size is smaller.  Copy-assign everything, placing results as close to center as possible
                ideal_begin = std::clamp( ideal_begin, begin(), end() - count );

                _destroy( begin(), ideal_begin );
                std::fill( ideal_begin, ideal_begin + count, value );
                _destroy( ideal_begin + count, end() );
            }
            else
            {
                // New size is larger.  Copy-assign all existing elements, placing newly
                // constructed elements so final store is as close to center as possible
                ideal_begin = std::clamp( ideal_begin, end() - count, begin() );

                _value_construct_range( ideal_begin, begin(), value );
                std::fill( begin(), end(), value );
                _value_construct_range( end(), begin() + count, value );
            }
            _move_begin( std::distance( begin(), ideal_begin ) );
            _move_end( std::distance( end(), ideal_begin + count ) );
        }

        // Casts to T&& or T&, depending on whether move construction is noexcept
        static decltype(auto) _nothrow_construct_move( T & t )
        {
            if constexpr ( std::is_nothrow_move_constructible_v<T> )
            {
                return std::move(t);
            }
            else
            {
                return t;
            }
        }

        // Move-constructs if noexcept, copies otherwise
        void _nothrow_move_construct( iterator dest, iterator src )
        {
            if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
            {
                *dest = *src;
            }
            else
            {
                alloc_traits::construct( _allocator(), dest, _nothrow_construct_move(*src) );
            }
        }

        void _nothrow_move_construct_range( iterator b, iterator e, iterator dest )
        {
            auto size = std::distance( b, e );
            if ( size )
            {
                if constexpr ( std::is_trivially_copy_constructible_v<T> && _calls_copy_constructor_directly )
                {
                    std::memcpy( dest, b, size * sizeof(T) );
                }
                else
                {
                    for ( ; b != e; ++dest, ++b )
                    {
                        _nothrow_move_construct( dest, b );
                    }
                }
            }
        }

        // Move-assigns if noexcept, copies otherwise
        static void _nothrow_move_assign( iterator dest, iterator src )
        {
            if constexpr ( std::is_nothrow_move_assignable_v<T> )
            {
                *dest = std::move(*src);
            }
            else
            {
                *dest = *src;
            }
        }

        static void _nothrow_move_assign_range( iterator b, iterator e, iterator src )
        {
            for ( auto dest = b; dest != e; ++dest, ++src )
            {
                _nothrow_move_assign( dest, src );
            }
        }

        // Adjust begin(), end() iterators
        void _move_begin( difference_type count ) noexcept
        {
            _size -= count;
            _offset += count;
        }

        void _move_end( difference_type count ) noexcept
        {
            _size += count;
        }

        // Convert a local const_iterator to iterator
        iterator _mutable_iterator( const_iterator i )
        {
            return begin() + std::distance( cbegin(), i );
        }

        // Retrieves beginning of storage, which may be before begin()
        const_iterator _storage_begin() const noexcept
        {
            return _data._storage;
        }

        iterator _storage_begin() noexcept
        {
            return _data._storage;
        }
    };

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline bool operator==( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline bool operator!=( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return !( lhs == rhs );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline bool operator<( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return std::lexicographical_compare( lhs.begin(), lhs.end(), rhs.begin(), rhs.end() );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline bool operator<=( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return !( rhs < lhs );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline bool operator>( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return ( rhs < lhs );
    }

    template< typename T, typename LResizeTraits, typename LAlloc, typename RResizeTraits, typename RAlloc >
    inline bool operator>=( const veque<T,LResizeTraits,LAlloc> &lhs, const veque<T,RResizeTraits,RAlloc> &rhs )
    {
        return !( lhs < rhs );
    }

    template< typename T, typename ResizeTraits, typename Alloc >
    inline void swap( veque<T,ResizeTraits,Alloc> & lhs, veque<T,ResizeTraits,Alloc> & rhs ) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    // Template deduction guide for iterator pair
    template< typename InputIt,
              typename Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    veque(InputIt, InputIt, Alloc = Alloc())
      -> veque<typename std::iterator_traits<InputIt>::value_type, fast_resize_traits, Alloc>;

}

namespace std
{
    template< typename T, typename ResizeTraits, typename Alloc >
    struct hash<veque::veque<T,ResizeTraits,Alloc>>
    {
        size_t operator()( const veque::veque<T,ResizeTraits,Alloc> & v ) const
        {
            size_t hash = 0;
            auto hasher = std::hash<T>();
            for ( auto && val : v )
            {
                hash ^= hasher(val) + 0x9e3779b9 + (hash<<6) + (hash>>2);
            }
            return hash;
        }
    };
}

#endif
