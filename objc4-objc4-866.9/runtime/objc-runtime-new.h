/*
 * Copyright (c) 2005-2007 Apple Inc.  All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef _OBJC_RUNTIME_NEW_H
#define _OBJC_RUNTIME_NEW_H

#include "PointerUnion.h"
#include <type_traits>

// class_data_bits_t is the class_t->data field (class_rw_t pointer plus flags)
// The extra bits are optimized for the retain/release and alloc/dealloc paths.

// Values for class_ro_t->flags
// These are emitted by the compiler and are part of the ABI.
// Note: See CGObjCNonFragileABIMac::BuildClassRoTInitializer in clang
// class is a metaclass
#define RO_META               (1<<0)
// class is a root class
#define RO_ROOT               (1<<1)
// class has .cxx_construct/destruct implementations
#define RO_HAS_CXX_STRUCTORS  (1<<2)
// class has +load implementation
// #define RO_HAS_LOAD_METHOD    (1<<3)
// class has visibility=hidden set
#define RO_HIDDEN             (1<<4)
// class has attribute(objc_exception): OBJC_EHTYPE_$_ThisClass is non-weak
#define RO_EXCEPTION          (1<<5)
// class has ro field for Swift metadata initializer callback
#define RO_HAS_SWIFT_INITIALIZER (1<<6)
// class compiled with ARC
#define RO_IS_ARC             (1<<7)
// class has .cxx_destruct but no .cxx_construct (with RO_HAS_CXX_STRUCTORS)
#define RO_HAS_CXX_DTOR_ONLY  (1<<8)
// class is not ARC but has ARC-style weak ivar layout
#define RO_HAS_WEAK_WITHOUT_ARC (1<<9)
// class does not allow associated objects on instances
#define RO_FORBIDS_ASSOCIATED_OBJECTS (1<<10)

// class is in an unloadable bundle - must never be set by compiler
#define RO_FROM_BUNDLE        (1<<29)
// class is unrealized future class - must never be set by compiler
#define RO_FUTURE             (1<<30)
// class is realized - must never be set by compiler
#define RO_REALIZED           (1<<31)

// Values for class_rw_t->flags
// These are not emitted by the compiler and are never used in class_ro_t.
// Their presence should be considered in future ABI versions.
// class_t->data is class_rw_t, not class_ro_t
#define RW_REALIZED           (1<<31)
// class is unresolved future class
#define RW_FUTURE             (1<<30)
// class is initialized
#define RW_INITIALIZED        (1<<29)
// class is initializing
#define RW_INITIALIZING       (1<<28)
// class_rw_t->ro is heap copy of class_ro_t
#define RW_COPIED_RO          (1<<27)
// class allocated but not yet registered
#define RW_CONSTRUCTING       (1<<26)
// class allocated and registered
#define RW_CONSTRUCTED        (1<<25)
// available for use; was RW_FINALIZE_ON_MAIN_THREAD
// #define RW_24 (1<<24)
// class +load has been called
#define RW_LOADED             (1<<23)
#if !SUPPORT_NONPOINTER_ISA
// class instances may have associative references
#define RW_INSTANCES_HAVE_ASSOCIATED_OBJECTS (1<<22)
#endif
// class has instance-specific GC layout
#define RW_HAS_INSTANCE_SPECIFIC_LAYOUT (1 << 21)
// class does not allow associated objects on its instances
#define RW_FORBIDS_ASSOCIATED_OBJECTS       (1<<20)
// class has started realizing but not yet completed it
#define RW_REALIZING          (1<<19)

#if CONFIG_USE_PREOPT_CACHES
// this class and its descendants can't have preopt caches with inlined sels
#define RW_NOPREOPT_SELS      (1<<2)
// this class and its descendants can't have preopt caches
#define RW_NOPREOPT_CACHE     (1<<1)
#endif

// class is a metaclass (copied from ro)
#define RW_META               RO_META // (1<<0)


// NOTE: MORE RW_ FLAGS DEFINED BELOW

// Values for class_rw_t->flags (RW_*), cache_t->_flags (FAST_CACHE_*),
// or class_t->bits (FAST_*).
//
// FAST_* and FAST_CACHE_* are stored on the class, reducing pointer indirection.

#if __LP64__

// class is a Swift class from the pre-stable Swift ABI
#define FAST_IS_SWIFT_LEGACY    (1UL<<0)
// class is a Swift class from the stable Swift ABI
#define FAST_IS_SWIFT_STABLE    (1UL<<1)
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
#define FAST_HAS_DEFAULT_RR     (1UL<<2)
// data pointer
#if TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
#define FAST_DATA_MASK          0x0000007ffffffff8UL
#else
#define FAST_DATA_MASK          0x00007ffffffffff8UL
#endif

static_assert((OBJC_VM_MAX_ADDRESS & FAST_DATA_MASK)
              == (OBJC_VM_MAX_ADDRESS & ~7UL),
              "FAST_DATA_MASK must not mask off pointer bits");

// just the flags
#define FAST_FLAGS_MASK         0x0000000000000007UL
// this bit tells us *quickly* that it's a pointer to an rw, not an ro
#define FAST_IS_RW_POINTER      0x8000000000000000UL

#else

// class or superclass has .cxx_construct implementation
#define RW_HAS_CXX_CTOR       (1<<18)
// class or superclass has .cxx_destruct implementation
#define RW_HAS_CXX_DTOR       (1<<17)
// class or superclass has default alloc/allocWithZone: implementation
// Note this is is stored in the metaclass.
#define RW_HAS_DEFAULT_AWZ    (1<<16)
// class's instances requires raw isa
#if SUPPORT_NONPOINTER_ISA
#define RW_REQUIRES_RAW_ISA   (1<<15)
#endif
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
#define RW_HAS_DEFAULT_RR     (1<<14)
// class or superclass has default new/self/class/respondsToSelector/isKindOfClass
#define RW_HAS_DEFAULT_CORE   (1<<13)

// class is a Swift class from the pre-stable Swift ABI
#define FAST_IS_SWIFT_LEGACY  (1UL<<0)
// class is a Swift class from the stable Swift ABI
#define FAST_IS_SWIFT_STABLE  (1UL<<1)
// data pointer
#define FAST_DATA_MASK        0xfffffffcUL
// flags mask
#define FAST_FLAGS_MASK       0x00000003UL
// no fast RW pointer flag on 32-bit
#define FAST_IS_RW_POINTER    0
#endif // __LP64__

// The Swift ABI requires that these bits be defined like this on all platforms.
static_assert(FAST_IS_SWIFT_LEGACY == 1, "resistance is futile");
static_assert(FAST_IS_SWIFT_STABLE == 2, "resistance is futile");


#if __LP64__
typedef uint32_t mask_t;  // x86_64 & arm64 asm are less efficient with 16-bits
#else
typedef uint16_t mask_t;
#endif
typedef uintptr_t SEL;

struct swift_class_t;

enum Atomicity { Atomic = true, NotAtomic = false };
enum IMPEncoding { Encoded = true, Raw = false };

// Strip TBI bits. We'll only use the top four bits at most so only strip those.
// Does nothing on targets that don't have TBI.
static inline void *stripTBI(void *p) {
#if __arm64__
    return (void *)((uintptr_t)p & 0x0fffffffffffffff);
#else
    return p;
#endif
}

static inline void try_free(const void *p)
{
    if (p && malloc_size(p)) free((void *)p);
}

struct bucket_t {
private:
    // IMP-first is better for arm64e ptrauth and no worse for arm64.
    // SEL-first is better for armv7* and i386 and x86_64.
#if __arm64__
    explicit_atomic<uintptr_t> _imp;
    explicit_atomic<SEL> _sel;
#else
    explicit_atomic<SEL> _sel;
    explicit_atomic<uintptr_t> _imp;
#endif

    // Compute the ptrauth signing modifier from &_imp, newSel, and cls.
    uintptr_t modifierForSEL(bucket_t *base, SEL newSel, Class cls) const {
        return (uintptr_t)base ^ (uintptr_t)newSel ^ (uintptr_t)cls;
    }

    // Sign newImp, with &_imp, newSel, and cls as modifiers.
    uintptr_t encodeImp(UNUSED_WITHOUT_PTRAUTH bucket_t *base, IMP newImp, UNUSED_WITHOUT_PTRAUTH SEL newSel, Class cls) const {
        if (!newImp) return 0;
#if CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_PTRAUTH
        return (uintptr_t)
            ptrauth_auth_and_resign(newImp,
                                    ptrauth_key_function_pointer, 0,
                                    ptrauth_key_process_dependent_code,
                                    modifierForSEL(base, newSel, cls));
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_ISA_XOR
        return (uintptr_t)newImp ^ (uintptr_t)cls;
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_NONE
        return (uintptr_t)newImp;
#else
#error Unknown method cache IMP encoding.
#endif
    }

public:
    static inline size_t offsetOfSel() { return offsetof(bucket_t, _sel); }
    inline SEL sel() const { return _sel.load(memory_order_relaxed); }

#if CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_ISA_XOR
#define MAYBE_UNUSED_ISA
#else
#define MAYBE_UNUSED_ISA __attribute__((unused))
#endif
    inline IMP rawImp(MAYBE_UNUSED_ISA objc_class *cls) const {
        uintptr_t imp = _imp.load(memory_order_relaxed);
        if (!imp) return nil;
#if CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_PTRAUTH
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_ISA_XOR
        imp ^= (uintptr_t)cls;
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_NONE
#else
#error Unknown method cache IMP encoding.
#endif
        return (IMP)imp;
    }

    inline IMP imp(UNUSED_WITHOUT_PTRAUTH bucket_t *base, Class cls) const {
        uintptr_t imp = _imp.load(memory_order_relaxed);
        if (!imp) return nil;
#if CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_PTRAUTH
        SEL sel = _sel.load(memory_order_relaxed);
        return (IMP)
            ptrauth_auth_and_resign((const void *)imp,
                                    ptrauth_key_process_dependent_code,
                                    modifierForSEL(base, sel, cls),
                                    ptrauth_key_function_pointer, 0);
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_ISA_XOR
        return (IMP)(imp ^ (uintptr_t)cls);
#elif CACHE_IMP_ENCODING == CACHE_IMP_ENCODING_NONE
        return (IMP)imp;
#else
#error Unknown method cache IMP encoding.
#endif
    }

    inline void scribbleIMP(uintptr_t value) {
        _imp.store(value, memory_order_relaxed);
    }

    template <Atomicity, IMPEncoding>
    void set(bucket_t *base, SEL newSel, IMP newImp, Class cls);
};

/* dyld_shared_cache_builder and obj-C agree on these definitions */
struct preopt_cache_entry_t {
    int64_t raw_imp_offs : 38; // actual IMP offset from the isa >> 2
    uint64_t sel_offs : 26;

    inline int64_t imp_offset() const {
        return raw_imp_offs << 2;
    }
};

/* dyld_shared_cache_builder and obj-C agree on these definitions */
struct preopt_cache_t {
    int64_t  fallback_class_offset;
    union {
        struct {
            uint16_t shift       :  5;
            uint16_t mask        : 11;
        };
        uint16_t hash_params;
    };
    uint16_t occupied    : 14;
    uint16_t has_inlines :  1;
    uint16_t padding     :  1;
    uint32_t unused      : 31;
    uint32_t bit_one     :  1;
    preopt_cache_entry_t entries[];

    inline int capacity() const {
        return mask + 1;
    }
};

// returns:
// - the cached IMP when one is found
// - nil if there's no cached value and the cache is dynamic
// - `value_on_constant_cache_miss` if there's no cached value and the cache is preoptimized
extern "C" IMP cache_getImp(Class cls, SEL sel, IMP value_on_constant_cache_miss = nil);

struct cache_t {
private:
    explicit_atomic<uintptr_t> _bucketsAndMaybeMask;
    union {
        // Note: _flags on ARM64 needs to line up with the unused bits of
        // _originalPreoptCache because we access some flags (specifically
        // FAST_CACHE_HAS_DEFAULT_CORE and FAST_CACHE_HAS_DEFAULT_AWZ) on
        // unrealized classes with the assumption that they will start out
        // as 0.
        struct {
#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_OUTLINED && !__LP64__
            // Outlined cache mask storage, 32-bit, we have mask and occupied.
            explicit_atomic<mask_t>    _mask;
            uint16_t                   _occupied;
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_OUTLINED && __LP64__
            // Outlined cache mask storage, 64-bit, we have mask, occupied, flags.
            explicit_atomic<mask_t>    _mask;
            uint16_t                   _occupied;
            uint16_t                   _flags;
#   define CACHE_T_HAS_FLAGS 1
#elif __LP64__
            // Inline cache mask storage, 64-bit, we have occupied, flags, and
            // empty space to line up flags with originalPreoptCache.
            //
            // Note: the assembly code for objc_release_xN knows about the
            // location of _flags and the
            // FAST_CACHE_HAS_CUSTOM_DEALLOC_INITIATION flag within. Any changes
            // must be applied there as well.
            uint32_t                   _unused;
            uint16_t                   _occupied;
            uint16_t                   _flags;
#   define CACHE_T_HAS_FLAGS 1
#else
            // Inline cache mask storage, 32-bit, we have occupied, flags.
            uint16_t                   _occupied;
            uint16_t                   _flags;
#   define CACHE_T_HAS_FLAGS 1
#endif

        };
        explicit_atomic<preopt_cache_t *> _originalPreoptCache;
    };

    // Simple constructor for testing purposes only.
    cache_t() : _bucketsAndMaybeMask(0) {}

#if CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_OUTLINED
    // _bucketsAndMaybeMask is a buckets_t pointer

    static constexpr uintptr_t bucketsMask = ~0ul;
    static_assert(!CONFIG_USE_PREOPT_CACHES, "preoptimized caches not supported");
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16_BIG_ADDRS
    static constexpr uintptr_t maskShift = 48;
    static constexpr uintptr_t maxMask = ((uintptr_t)1 << (64 - maskShift)) - 1;
    static constexpr uintptr_t bucketsMask = ((uintptr_t)1 << maskShift) - 1;

    static_assert(bucketsMask >= OBJC_VM_MAX_ADDRESS, "Bucket field doesn't have enough bits for arbitrary pointers.");
#if CONFIG_USE_PREOPT_CACHES
    static constexpr uintptr_t preoptBucketsMarker = 1ul;
    static constexpr uintptr_t preoptBucketsMask = bucketsMask & ~preoptBucketsMarker;
#endif
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_HIGH_16
    // _bucketsAndMaybeMask is a buckets_t pointer in the low 48 bits

    // How much the mask is shifted by.
    static constexpr uintptr_t maskShift = 48;

    // Additional bits after the mask which must be zero. msgSend
    // takes advantage of these additional bits to construct the value
    // `mask << 4` from `_maskAndBuckets` in a single instruction.
    static constexpr uintptr_t maskZeroBits = 4;

    // The largest mask value we can store.
    static constexpr uintptr_t maxMask = ((uintptr_t)1 << (64 - maskShift)) - 1;

    // The mask applied to `_maskAndBuckets` to retrieve the buckets pointer.
    static constexpr uintptr_t bucketsMask = ((uintptr_t)1 << (maskShift - maskZeroBits)) - 1;

    // Ensure we have enough bits for the buckets pointer.
    static_assert(bucketsMask >= OBJC_VM_MAX_ADDRESS,
            "Bucket field doesn't have enough bits for arbitrary pointers.");

#if CONFIG_USE_PREOPT_CACHES
    static constexpr uintptr_t preoptBucketsMarker = 1ul;
#if __has_feature(ptrauth_calls)
    // 63..60: hash_mask_shift
    // 59..55: hash_shift
    // 54.. 1: buckets ptr + auth
    //      0: always 1
    static constexpr uintptr_t preoptBucketsMask = 0x007ffffffffffffe;
    static inline uintptr_t preoptBucketsHashParams(const preopt_cache_t *cache) {
        uintptr_t value = (uintptr_t)cache->shift << 55;
        // masks have 11 bits but can be 0, so we compute
        // the right shift for 0x7fff rather than 0xffff
        return value | ((objc::mask16ShiftBits(cache->mask) - 1) << 60);
    }
#else
    // 63..53: hash_mask
    // 52..48: hash_shift
    // 47.. 1: buckets ptr
    //      0: always 1
    static constexpr uintptr_t preoptBucketsMask = 0x0000fffffffffffe;
    static inline uintptr_t preoptBucketsHashParams(const preopt_cache_t *cache) {
        return (uintptr_t)cache->hash_params << 48;
    }
#endif
#endif // CONFIG_USE_PREOPT_CACHES
#elif CACHE_MASK_STORAGE == CACHE_MASK_STORAGE_LOW_4
    // _bucketsAndMaybeMask is a buckets_t pointer in the top 28 bits

    static constexpr uintptr_t maskBits = 4;
    static constexpr uintptr_t maskMask = (1 << maskBits) - 1;
    static constexpr uintptr_t bucketsMask = ~maskMask;
    static_assert(!CONFIG_USE_PREOPT_CACHES, "preoptimized caches not supported");
#else
#error Unknown cache mask storage type.
#endif

    bool isConstantEmptyCache() const;
    bool canBeFreed() const;
    mask_t mask() const;

#if CONFIG_USE_PREOPT_CACHES
    void initializeToPreoptCacheInDisguise(const preopt_cache_t *cache);
    const preopt_cache_t *disguised_preopt_cache() const;
#endif

    void incrementOccupied();
    void setBucketsAndMask(struct bucket_t *newBuckets, mask_t newMask);

    void reallocate(mask_t oldCapacity, mask_t newCapacity, bool freeOld);
    void collect_free(bucket_t *oldBuckets, mask_t oldCapacity);

    static bucket_t *emptyBuckets();
    static bucket_t *allocateBuckets(mask_t newCapacity);
    static bucket_t *emptyBucketsForCapacity(mask_t capacity, bool allocate = true);
    static struct bucket_t * endMarker(struct bucket_t *b, uint32_t cap);
    void bad_cache(id receiver, SEL sel) __attribute__((noreturn, cold));

public:
    // The following four fields are public for objcdt's use only.
    // objcdt reaches into fields while the process is suspended
    // hence doesn't care for locks and pesky little details like this
    // and can safely use these.
    unsigned capacity() const;
    struct bucket_t *buckets() const;
    Class cls() const;

#if CONFIG_USE_PREOPT_CACHES
    const preopt_cache_t *preopt_cache(bool authenticated = true) const;
#endif

    mask_t occupied() const;
    void initializeToEmpty();

#if CONFIG_USE_PREOPT_CACHES
    bool isConstantOptimizedCache(bool strict = false, uintptr_t empty_addr = (uintptr_t)&_objc_empty_cache) const;
    bool shouldFlush(SEL sel, IMP imp) const;
    bool isConstantOptimizedCacheWithInlinedSels() const;
    Class preoptFallbackClass() const;
    void maybeConvertToPreoptimized();
    void initializeToEmptyOrPreoptimizedInDisguise();
#else
    inline bool isConstantOptimizedCache(bool strict = false, uintptr_t empty_addr = 0) const { return false; }
    inline bool shouldFlush(SEL sel, IMP imp) const {
        return cache_getImp(cls(), sel) == imp;
    }
    inline bool isConstantOptimizedCacheWithInlinedSels() const { return false; }
    inline void initializeToEmptyOrPreoptimizedInDisguise() { initializeToEmpty(); }
#endif

    void insert(SEL sel, IMP imp, id receiver);
    void copyCacheNolock(objc_imp_cache_entry *buffer, int len);
    void destroy();
    void eraseNolock(const char *func);

    static void init();
    static void collectNolock(bool collectALot);
    static size_t bytesForCapacity(uint32_t cap);

#if CACHE_T_HAS_FLAGS
#   if __arm64__
// class or superclass has .cxx_construct/.cxx_destruct implementation
//   FAST_CACHE_HAS_CXX_DTOR is the first bit so that setting it in
//   isa_t::has_cxx_dtor is a single bfi
#       define FAST_CACHE_HAS_CXX_DTOR       (1<<0)
#       define FAST_CACHE_HAS_CXX_CTOR       (1<<1)
// Denormalized RO_META to avoid an indirection
#       define FAST_CACHE_META               (1<<2)
#   else
// Denormalized RO_META to avoid an indirection
#       define FAST_CACHE_META               (1<<0)
// class or superclass has .cxx_construct/.cxx_destruct implementation
//   FAST_CACHE_HAS_CXX_DTOR is chosen to alias with isa_t::has_cxx_dtor
#       define FAST_CACHE_HAS_CXX_CTOR       (1<<1)
#       define FAST_CACHE_HAS_CXX_DTOR       (1<<2)
#endif

// Fast Alloc fields:
//   This stores the word-aligned size of instances + "ALLOC_DELTA16",
//   or 0 if the instance size doesn't fit.
//
//   These bits occupy the same bits than in the instance size, so that
//   the size can be extracted with a simple mask operation.
//
//   FAST_CACHE_ALLOC_MASK16 allows to extract the instance size rounded
//   rounded up to the next 16 byte boundary, which is a fastpath for
//   _objc_rootAllocWithZone()

// The code in fastInstanceSize/setFastInstanceSize is not quite right for
// 32-bit, so we currently only enable this for 64-bit.
#   if __LP64__
#       define FAST_CACHE_ALLOC_MASK         0x0ff8
#       define FAST_CACHE_ALLOC_MASK16       0x0ff0
#       define FAST_CACHE_ALLOC_DELTA16      0x0008
#   endif

#   define FAST_CACHE_HAS_CUSTOM_DEALLOC_INITIATION (1<<12)
// class's instances requires raw isa
#   define FAST_CACHE_REQUIRES_RAW_ISA   (1<<13)
// class or superclass has default alloc/allocWithZone: implementation
// Note this is is stored in the metaclass.
#   define FAST_CACHE_HAS_DEFAULT_AWZ    (1<<14)
// class or superclass has default new/self/class/respondsToSelector/isKindOfClass
#   define FAST_CACHE_HAS_DEFAULT_CORE   (1<<15)

    bool getBit(uint16_t flags) const {
        return _flags & flags;
    }
    void setBit(uint16_t set) {
        __c11_atomic_fetch_or((_Atomic(uint16_t) *)&_flags, set, __ATOMIC_RELAXED);
    }
    void clearBit(uint16_t clear) {
        __c11_atomic_fetch_and((_Atomic(uint16_t) *)&_flags, ~clear, __ATOMIC_RELAXED);
    }
#endif

#if FAST_CACHE_ALLOC_MASK
    bool hasFastInstanceSize(size_t extra) const
    {
        if (__builtin_constant_p(extra) && extra == 0) {
            return _flags & FAST_CACHE_ALLOC_MASK16;
        }
        return _flags & FAST_CACHE_ALLOC_MASK;
    }

    size_t fastInstanceSize(size_t extra) const
    {
        ASSERT(hasFastInstanceSize(extra));

        if (__builtin_constant_p(extra) && extra == 0) {
            return _flags & FAST_CACHE_ALLOC_MASK16;
        } else {
            size_t size = _flags & FAST_CACHE_ALLOC_MASK;
            // remove the FAST_CACHE_ALLOC_DELTA16 that was added
            // by setFastInstanceSize
            return align16(size + extra - FAST_CACHE_ALLOC_DELTA16);
        }
    }

    void setFastInstanceSize(size_t newSize)
    {
        // Set during realization or construction only. No locking needed.
        uint16_t newBits = _flags & ~FAST_CACHE_ALLOC_MASK;
        uint16_t sizeBits;

        // Adding FAST_CACHE_ALLOC_DELTA16 allows for FAST_CACHE_ALLOC_MASK16
        // to yield the proper 16byte aligned allocation size with a single mask
        sizeBits = word_align(newSize) + FAST_CACHE_ALLOC_DELTA16;
        sizeBits &= FAST_CACHE_ALLOC_MASK;
        if (newSize <= sizeBits) {
            newBits |= sizeBits;
        }
        _flags = newBits;
    }
#else
    bool hasFastInstanceSize(size_t extra) const {
        return false;
    }
    size_t fastInstanceSize(size_t extra) const {
        abort();
    }
    void setFastInstanceSize(size_t extra) {
        // nothing
    }
#endif
};

static_assert(sizeof(cache_t) == 2 * sizeof(void *), "cache_t must be two words");

// classref_t is unremapped class_t*
typedef struct classref * classref_t;


/***********************************************************************
* RelativePointer<T>
* A pointer stored as an offset from the address of that offset.
*
* The target address is computed by taking the address of this struct
* and adding the offset stored within it. This is a 32-bit signed
* offset giving ±2GB of range.
**********************************************************************/
template <typename T, bool isNullable = true>
struct RelativePointer: nocopy_t {
    int32_t offset;

    void *getRaw(uintptr_t base) const {
        if (isNullable && offset == 0)
            return nullptr;
        uintptr_t signExtendedOffset = (uintptr_t)(intptr_t)offset;
        uintptr_t pointer = base + signExtendedOffset;
        return (void *)pointer;
    }

    void *getRaw() const {
        return getRaw((uintptr_t)&offset);
    }

    T get(uintptr_t base) const {
        return (T)getRaw(base);
    }

    T get() const {
        return (T)getRaw();
    }
};


#ifdef __PTRAUTH_INTRINSICS__
#   define StubClassInitializerPtrauth __ptrauth(ptrauth_key_function_pointer, 1, 0xc671)
#else
#   define StubClassInitializerPtrauth
#endif
struct stub_class_t {
    uintptr_t isa;
    _objc_swiftMetadataInitializer StubClassInitializerPtrauth initializer;
};

// A pointer modifier that does nothing to the pointer.
struct PointerModifierNop {
    template <typename ListType, typename T>
    static T *modify(__unused const ListType &list, T *ptr) { return ptr; }
};

/***********************************************************************
* entsize_list_tt<Element, List, FlagMask, PointerModifier>
* Generic implementation of an array of non-fragile structs.
*
* Element is the struct type (e.g. method_t)
* List is the specialization of entsize_list_tt (e.g. method_list_t)
* FlagMask is used to stash extra bits in the entsize field
*   (e.g. method list fixup markers)
* PointerModifier is applied to the element pointers retrieved from
* the array.
**********************************************************************/
template <typename Element, typename List, uint32_t FlagMask, typename PointerModifier = PointerModifierNop>
struct entsize_list_tt {
    uint32_t entsizeAndFlags;
    uint32_t count;

    uint32_t entsize() const {
        return entsizeAndFlags & ~FlagMask;
    }
    uint32_t flags() const {
        return entsizeAndFlags & FlagMask;
    }

    Element& getOrEnd(uint32_t i) const {
        ASSERT(i <= count);
        return *PointerModifier::modify(*(List *)this, (Element *)((uint8_t *)this + sizeof(*this) + i*entsize()));
    }
    Element& get(uint32_t i) const {
        ASSERT(i < count);
        return getOrEnd(i);
    }

    size_t byteSize() const {
        return byteSize(entsize(), count);
    }

    static size_t byteSize(uint32_t entsize, uint32_t count) {
        return sizeof(entsize_list_tt) + count*entsize;
    }

    template <bool authenticated>
    struct iteratorImpl;
    using iterator = iteratorImpl<false>;
    using signedIterator = iteratorImpl<true>;
    const iterator begin() const {
        return iterator(*static_cast<const List*>(this), 0);
    }
    iterator begin() {
        return iterator(*static_cast<const List*>(this), 0);
    }
    const iterator end() const {
        return iterator(*static_cast<const List*>(this), count);
    }
    iterator end() {
        return iterator(*static_cast<const List*>(this), count);
    }

    const signedIterator signedBegin() const {
        return signedIterator(*static_cast<const List *>(this), 0);
    }
    const signedIterator signedEnd() const {
        return signedIterator(*static_cast<const List*>(this), count);
    }

    template <bool authenticated>
    struct iteratorImpl {
        uint32_t entsize;
        uint32_t index;  // keeping track of this saves a divide in operator-

        using ElementPtr = std::conditional_t<authenticated, Element * __ptrauth(ptrauth_key_process_dependent_data, 1, 0xdead), Element *>;

        ElementPtr element;

        typedef std::random_access_iterator_tag iterator_category;
        typedef Element value_type;
        typedef ptrdiff_t difference_type;
        typedef Element* pointer;
        typedef Element& reference;

        iteratorImpl() { }

        iteratorImpl(const List& list, uint32_t start = 0)
            : entsize(list.entsize())
            , index(start)
            , element(&list.getOrEnd(start))
        { }

        const iteratorImpl& operator += (ptrdiff_t delta) {
            element = (Element*)((uint8_t *)element + delta*entsize);
            index += (int32_t)delta;
            return *this;
        }
        const iteratorImpl& operator -= (ptrdiff_t delta) {
            element = (Element*)((uint8_t *)element - delta*entsize);
            index -= (int32_t)delta;
            return *this;
        }
        const iteratorImpl operator + (ptrdiff_t delta) const {
            return iteratorImpl(*this) += delta;
        }
        const iteratorImpl operator - (ptrdiff_t delta) const {
            return iteratorImpl(*this) -= delta;
        }

        iteratorImpl& operator ++ () { *this += 1; return *this; }
        iteratorImpl& operator -- () { *this -= 1; return *this; }
        iteratorImpl operator ++ (int) {
            iteratorImpl result(*this); *this += 1; return result;
        }
        iteratorImpl operator -- (int) {
            iteratorImpl result(*this); *this -= 1; return result;
        }

        ptrdiff_t operator - (const iteratorImpl& rhs) const {
            return (ptrdiff_t)this->index - (ptrdiff_t)rhs.index;
        }

        Element& operator * () const { return *element; }
        Element* operator -> () const { return element; }

        operator Element& () const { return *element; }

        bool operator == (const iteratorImpl& rhs) const {
            return this->element == rhs.element;
        }
        bool operator != (const iteratorImpl& rhs) const {
            return this->element != rhs.element;
        }

        bool operator < (const iteratorImpl& rhs) const {
            return this->element < rhs.element;
        }
        bool operator > (const iteratorImpl& rhs) const {
            return this->element > rhs.element;
        }
    };
};


namespace objc {
// Let method_t::small use this from objc-private.h.
static inline bool inSharedCache(uintptr_t ptr);
}

// All shared cache relative method lists names are offsets from this selector.
extern "C" uintptr_t sharedCacheRelativeMethodBase();

// We have four kinds of methods: small, small direct, big, and big signed. (Big
// signed is only used on ARM64e.) We distinguish between them using a
// complicated set of flags and other checks.
//
// All methods consist of three values: name (selector), types, and imp. The
// difference is in how those values are represented:
//
// 1. Small methods represent the values as 32-bit offsets relative to the
//    address of each field. Selectors are an offset to a selref. This
//    representation is used on disk, and is always emitted as read-only memory.
// 2. Small direct methods are the same as small methods, except the selector is
//    an offset to the shared cache's special explodey-head selector. This
//    representation is used in the shared cache.
// 3. Big methods represent the values as pointers. This is the original method
//    list format and was the only format for a long time. On ARM64e, the imp is
//    signed, but the selector and types are not. This representation is used on
//    disk, usually when targeting older OSes that don't have support for small
//    methods, but sometimes when that particular corner of the compiler or
//    linker just hasn't been upgraded to emit small methods.
// 4. Big signed methods are the same as big methods, but the selector is
//    signed. These are used internally by the runtime for dynamically allocated
//    method lists created by calls like `class_addMethod`.
//
// These different kinds must be distinguished in two contexts: method pointers,
// and method list pointers. Method list pointers are the complicated one, so
// we'll start there:
// 1. Small method lists are identified by setting `smallMethodListFlag` in the
//    method list flags. This flag is set by the compiler.
// 2. Small direct method lists are additionally identified by being within the
//    address range of the shared cache.
// 3. Big methods have no flags set, since they are the historical default.
// 4. Big signed methods are identified by setting `bigSignedMethodListFlag` in
//    the method list pointer itself. The runtime sets this flag in the pointer
//    when creating a new method list at runtime.
//
// Method pointers are never emitted by the compiler and exist entirely within
// the runtime, making this task somewhat simpler. A method's kind is indicated
// by the lower two bits in the pointer. Methods are always 4-byte aligned so
// those bits are never set in the pointer value itself.
//
// OVERWRITE PROTECTION
//
// We protect these values from overwrites on systems with pointer
// authentication. This protection is multifaceted and not completely obvious.
// Here's how it works.
//
// 1. Method pointers are signed before returning them to clients, and
//    authenticated when clients pass them in. The signature includes the low
//    bits that store the kind. An attacker is therefore not able to create a
//    new method pointer from scratch, nor change the kind of a legitimate
//    method pointer. Method pointers are not stored in memory in the runtime.
// 2. Small and small direct method lists are always in read-only memory,
//    protecting them from any alteration.
// 3. Big signed method lists are indicated with a flag in the method list
//    pointer.
// 4. Method list pointers are signed, preventing forgery of a method list
//    pointer. This signature includes the big signed method list flag.
// 5. Big method lists are vulnerable in two ways: the selector is
//    unauthenticated, and the flags are unauthenticated such that an attacker
//    could potentially set `smallMethodListFlag` to get a small method list in
//    mutable memory. We will mitigate this by ensuring the compiler/linker
//    never emit big method lists.
// 6. Although big signed method lists do not sign their flags,
//    `smallMethodListFlag` is ignored when `bigSignedMethodListFlag` is set.
//    Since `bigSignedMethodListFlag` is authenticated, it cannot be cleared by
//    an attacker.
struct method_t {
    method_t(const method_t &other) = delete;

    // The representation of a "big" method. This is the traditional
    // representation of three pointers storing the selector, types
    // and implementation.
    struct big {
        SEL name;
        const char *types;
        MethodListIMP imp;
    };

    // A "big" method, but name is signed. Used for method lists created at runtime.
    struct bigSigned {
        SEL __ptrauth_objc_sel name;
        const char * ptrauth_method_list_types types;
        MethodListIMP imp;
    };

    // Various things assume big and bigSigned are the same size, make sure we
    // don't accidentally break that.
    static_assert(sizeof(struct big) == sizeof(struct bigSigned), "big and bigSigned are expected to be the same size");

    enum class Kind {
        // Note: method_invoke detects small methods by detecting 1 in the low
        // bit. Any change to that will require a corresponding change to
        // method_invoke.
        big = 0,

        // `small` encompasses both small and small direct methods. We
        // distinguish those cases by doing a range check against the shared
        // cache.
        small = 1,
        bigSigned = 2,
    };

private:
    static const uintptr_t kindMask = 0x3;

    Kind getKind() const {
        return Kind((uintptr_t)this & kindMask);
    }

    void *getPointer() const {
        return (void *)((uintptr_t)this & ~kindMask);
    }

    method_t *withKind(Kind kind) {
        uintptr_t combined = (uintptr_t)this->getPointer() | (uintptr_t)kind;
        method_t *ret = (method_t *)combined;
        ASSERT(ret->getKind() == kind);
        return ret;
    }

    // The representation of a "small" method. This stores three
    // relative offsets to the name, types, and implementation.
    struct small {
        // The name field either refers to a selector (in the shared
        // cache) or a selref (everywhere else).
        RelativePointer<const void *> name;
        RelativePointer<const char *> types;
        RelativePointer<IMP, /*isNullable*/false> imp;

        bool inSharedCache() const {
            return (CONFIG_SHARED_CACHE_RELATIVE_DIRECT_SELECTORS &&
                    objc::inSharedCache((uintptr_t)this));
        }
    };

    small &small() const {
        ASSERT(getKind() == Kind::small);
        return *(struct small *)getPointer();
    }

    IMP remappedImp(bool needsLock) const;
    void remapImp(IMP imp);
    objc_method_description *getCachedDescription() const;

public:
    static const auto bigSize = sizeof(struct big);
    static const auto smallSize = sizeof(struct small);

    // The pointer modifier used with method lists. When the method
    // list contains small methods, set the bottom bit of the pointer.
    // We use that bottom bit elsewhere to distinguish between big
    // and small methods.
    struct pointer_modifier {
        template <typename ListType>
        static method_t *modify(const ListType &list, method_t *ptr) {
            return ptr->withKind(list.listKind());
        }
    };

    big &big() const {
        ASSERT(getKind() == Kind::big);
        return *(struct big *)getPointer();
    }

    bigSigned &bigSigned() const {
        ASSERT(getKind() == Kind::bigSigned);
        return *(struct bigSigned *)getPointer();
    }

    ALWAYS_INLINE SEL name() const {
        switch (getKind()) {
            case Kind::small:
                if (small().inSharedCache()) {
                    return (SEL)small().name.get(sharedCacheRelativeMethodBase());
                } else {
                    // Outside of the shared cache, relative methods point to a selRef
                    return *(SEL *)small().name.get();
                }
            case Kind::big:
                return big().name;
            case Kind::bigSigned:
                return bigSigned().name;
        }
    }

    const char *types() const {
        switch (getKind()) {
            case Kind::small:
                return small().types.get();
            case Kind::big:
                return big().types;
            case Kind::bigSigned:
                return bigSigned().types;
        }
    }

    IMP imp(bool needsLock) const {
        switch (getKind()) {
            case Kind::small: {
                IMP smallIMP = ptrauth_sign_unauthenticated(small().imp.get(),
                                                            ptrauth_key_function_pointer, 0);
                // We must sign the newly generated function pointer before calling
                // out to remappedImp(). That call may spill `this` leaving it open
                // to being overwritten while it's on the stack. By signing first,
                // we'll spill the signed function pointer instead, which is
                // resistant to being overwritten.
                //
                // The compiler REALLY wants to perform this signing operation after
                // the call to remappedImp. This asm statement prevents it from
                // doing that reordering.
                asm ("": : "r" (smallIMP) :);

                IMP remappedIMP = remappedImp(needsLock);
                if (remappedIMP)
                    return remappedIMP;
                return smallIMP;
            }
            case Kind::big:
                return big().imp;
            case Kind::bigSigned:
                return bigSigned().imp;
        }
    }

    // Fetch the IMP as a `void *`. Avoid signing relative IMPs. This
    // avoids signing oracles in cases where we're just logging the
    // value. Runtime lock must be held.
    void *impRaw() const {
        switch (getKind()) {
            case Kind::small: {
                IMP remappedIMP = remappedImp(false);
                if (remappedIMP)
                    return (void *)remappedIMP;
                return small().imp.getRaw();
            }
            case Kind::big:
                return (void *)big().imp;
            case Kind::bigSigned:
                return (void *)bigSigned().imp;
        }
    }

    SEL getSmallNameAsSEL() const {
        ASSERT(small().inSharedCache());
        return (SEL)small().name.get(sharedCacheRelativeMethodBase());
    }

    SEL getSmallNameAsSELRef() const {
        ASSERT(!small().inSharedCache());
        return *(SEL *)small().name.get();
    }

    void setName(SEL name) {
        switch (getKind()) {
            case Kind::small:
                ASSERT(!small().inSharedCache());
                *(SEL *)small().name.get() = name;
                break;
            case Kind::big:
                big().name = name;
                break;
            case Kind::bigSigned:
                bigSigned().name = name;
                break;
        }
    }

    void setImp(IMP imp) {
        switch (getKind()) {
            case Kind::small:
                remapImp(imp);
                break;
            case Kind::big:
                big().imp = imp;
                break;
            case Kind::bigSigned:
                bigSigned().imp = imp;
                break;
        }
    }

    objc_method_description *getDescription() const {
        switch (getKind()) {
            case Kind::small:
                return getCachedDescription();
            case Kind::big:
                return(struct objc_method_description *)getPointer();
            case Kind::bigSigned:
                return getCachedDescription();
        }
    }

    void tryFreeContents_nolock();

    struct SortBySELAddress
    {
        bool operator() (const struct method_t::big& lhs,
                         const struct method_t::big& rhs)
        { return lhs.name < rhs.name; }

        bool operator() (const struct method_t::bigSigned& lhs,
                         const struct method_t::bigSigned& rhs)
        { return lhs.name < rhs.name; }
    };

    method_t &operator=(const method_t &other) {
        switch (getKind()) {
            case Kind::small:
                _objc_fatal("Cannot assign to small method %p from method %p", this, &other);
                break;
            case Kind::big:
                big().imp = other.imp(false);
                big().name = other.name();
                big().types = other.types();
                break;
            case Kind::bigSigned:
                bigSigned().imp = other.imp(false);
                bigSigned().name = other.name();
                bigSigned().types = other.types();
                break;
        }
        return *this;
    }
};

struct ivar_t {
#if __x86_64__
    // *offset was originally 64-bit on some x86_64 platforms.
    // We read and write only 32 bits of it.
    // Some metadata provides all 64 bits. This is harmless for unsigned
    // little-endian values.
    // Some code uses all 64 bits. class_addIvar() over-allocates the
    // offset for their benefit.
#endif
    int32_t *offset;
    const char *name;
    const char *type;
    // alignment is sometimes -1; use alignment() instead
    uint32_t alignment_raw;
    uint32_t size;

    uint32_t alignment() const {
        if (alignment_raw == ~(uint32_t)0) return 1U << WORD_SHIFT;
        return 1 << alignment_raw;
    }
};

struct property_t {
    const char *name;
    const char *attributes;
};

// Two bits of entsize are used for fixup markers.
// Reserve the top half of entsize for more flags. We never
// need entry sizes anywhere close to 64kB.
//
// Currently there is one flag defined: the small method list flag,
// method_t::smallMethodListFlag. Other flags are currently ignored.
// (NOTE: these bits are only ignored on runtimes that support small
// method lists. Older runtimes will treat them as part of the entry
// size!)
struct method_list_t : entsize_list_tt<method_t, method_list_t, 0xffff0003, method_t::pointer_modifier> {
#if __has_feature(ptrauth_calls)
    // This flag is ORed into method list pointers to indicate that the list is
    // a big list with signed pointers. Use a bit in TBI so we don't have to
    // mask it out to use the pointer.
    static const uintptr_t bigSignedMethodListFlag = 0x8000000000000000;
#else
    static const uintptr_t bigSignedMethodListFlag = 0x0;
#endif

    static const uint32_t smallMethodListFlag = 0x80000000;

    // We don't use this currently, but the shared cache builder sets it, so be
    // mindful we don't collide.
    static const uint32_t relativeMethodSelectorsAreDirectFlag = 0x40000000;

    static method_list_t *allocateMethodList(uint32_t count, uint32_t flags) {
        void *allocation = calloc(method_list_t::byteSize(count,
                                                          method_t::bigSize), 1);

        // Place bigSignedMethodListFlag into the new list pointer. TBI ensures
        // that we can still use the pointer.
        method_list_t *newlist = (method_list_t *)((uintptr_t)allocation | bigSignedMethodListFlag);
        newlist->entsizeAndFlags =
            (uint32_t)sizeof(struct method_t::big) | flags;
        newlist->count = count;
        return newlist;
    }

    void deallocate() {
        free(stripTBI(this));
    }

    bool isUniqued() const;
    bool isFixedUp() const;
    void setFixedUp();

    uint32_t indexOfMethod(const method_t *meth) const {
        uint32_t i =
            (uint32_t)(((uintptr_t)meth - (uintptr_t)this) / entsize());
        ASSERT(i < count);
        return i;
    }

    method_t::Kind listKind() const {
        if ((uintptr_t)this & bigSignedMethodListFlag)
            return method_t::Kind::bigSigned;
        if (flags() & smallMethodListFlag)
            return method_t::Kind::small;
        else
            return method_t::Kind::big;
    }

    bool isExpectedSize() const {
        if (listKind() == method_t::Kind::small)
            return entsize() == method_t::smallSize;
        else
            return entsize() == method_t::bigSize;
    }

    method_list_t *duplicate() const {
        auto begin = signedBegin();
        auto end = signedEnd();

        uint32_t newFlags = listKind() == method_t::Kind::small ? 0 : flags();
        method_list_t *dup = allocateMethodList(count, newFlags);
        std::copy(begin, end, dup->begin());
        return dup;
    }

    void sortBySELAddress() {
        switch (listKind()) {
            case method_t::Kind::small:
                _objc_fatal("Cannot sort small method list %p", this);
                break;
            case method_t::Kind::big:
                method_t::SortBySELAddress sorter;
                std::stable_sort(&begin()->big(), &end()->big(), sorter);
                break;
            case method_t::Kind::bigSigned:;
                std::stable_sort(&begin()->bigSigned(), &end()->bigSigned(), sorter);
                break;
        }
    }

    struct Ptrauth {
        static const uint16_t discriminator = 0xC310;
        static_assert(std::is_same<
                      void * __ptrauth_objc_method_list_pointer *,
                      void * __ptrauth(ptrauth_key_method_list_pointer, 1, discriminator) *>::value,
                      "Method list pointer signing discriminator must match ptrauth.h");

        ALWAYS_INLINE static method_list_t *sign (method_list_t *ptr, const void *address) {
            return ptrauth_sign_unauthenticated(ptr,
                                                ptrauth_key_method_list_pointer,
                                                ptrauth_blend_discriminator(address,
                                                                            discriminator));
        }

        ALWAYS_INLINE static method_list_t *auth(method_list_t *ptr, const void *address) {
            (void)address;
#if __has_feature(ptrauth_calls)
            if (ptr)
                ptr = ptrauth_auth_data(ptr,
                                        ptrauth_key_method_list_pointer,
                                        ptrauth_blend_discriminator(address,
                                                                    discriminator));
#endif
            return ptr;
        }
    };
};

struct ivar_list_t : entsize_list_tt<ivar_t, ivar_list_t, 0> {
    bool containsIvar(Ivar ivar) const {
        return (ivar >= (Ivar)&*begin()  &&  ivar < (Ivar)&*end());
    }
};

struct property_list_t : entsize_list_tt<property_t, property_list_t, 0> {
};


typedef uintptr_t protocol_ref_t;  // protocol_t *, but unremapped

// Values for protocol_t->flags
#define PROTOCOL_FIXED_UP_2     (1<<31)  // must never be set by compiler
#define PROTOCOL_FIXED_UP_1     (1<<30)  // must never be set by compiler
#define PROTOCOL_IS_CANONICAL   (1<<29)  // must never be set by compiler
// Bits 0..15 are reserved for Swift's use.

#define PROTOCOL_FIXED_UP_MASK (PROTOCOL_FIXED_UP_1 | PROTOCOL_FIXED_UP_2)

struct protocol_t : objc_object {
    const char *mangledName;
    struct protocol_list_t *protocols;
    method_list_t *instanceMethods;
    method_list_t *classMethods;
    method_list_t *optionalInstanceMethods;
    method_list_t *optionalClassMethods;
    property_list_t *instanceProperties;
    uint32_t size;   // sizeof(protocol_t)
    uint32_t flags;
    // Fields below this point are not always present on disk.
    const char **_extendedMethodTypes;
    const char *_demangledName;
    property_list_t *_classProperties;

    const char *demangledName();

    const char *nameForLogging() {
        return demangledName();
    }

    bool isFixedUp() const;
    void setFixedUp();

    bool isCanonical() const;
    void clearIsCanonical();

#   define HAS_FIELD(f) ((uintptr_t)(&f) < ((uintptr_t)this + size))

    bool hasExtendedMethodTypesField() const {
        return HAS_FIELD(_extendedMethodTypes);
    }
    bool hasDemangledNameField() const {
        return HAS_FIELD(_demangledName);
    }
    bool hasClassPropertiesField() const {
        return HAS_FIELD(_classProperties);
    }

#   undef HAS_FIELD

    const char **extendedMethodTypes() const {
        return hasExtendedMethodTypesField() ? _extendedMethodTypes : nil;
    }

    property_list_t *classProperties() const {
        return hasClassPropertiesField() ? _classProperties : nil;
    }
};

struct protocol_list_t {
    // count is pointer-sized by accident.
    uintptr_t count;
    protocol_ref_t list[0]; // variable-size

    size_t byteSize() const {
        return sizeof(*this) + count*sizeof(list[0]);
    }

    protocol_list_t *duplicate() const {
        return (protocol_list_t *)memdup(this, this->byteSize());
    }

    typedef protocol_ref_t* iterator;
    typedef const protocol_ref_t* const_iterator;

    const_iterator begin() const {
        return list;
    }
    iterator begin() {
        return list;
    }
    const_iterator end() const {
        return list + count;
    }
    iterator end() {
        return list + count;
    }
};

struct class_ro_t {
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;
#ifdef __LP64__
    uint32_t reserved;
#endif

    union {
        const uint8_t * ivarLayout;
        Class nonMetaclass;
    };

    explicit_atomic<const char *> name;
    WrappedPtr<method_list_t, method_list_t::Ptrauth> baseMethods;
    protocol_list_t * baseProtocols;
    const ivar_list_t * ivars;

    const uint8_t * weakIvarLayout;
    property_list_t *baseProperties;

    // This field exists only when RO_HAS_SWIFT_INITIALIZER is set.
    _objc_swiftMetadataInitializer __ptrauth_objc_method_list_imp _swiftMetadataInitializer_NEVER_USE[0];

    _objc_swiftMetadataInitializer swiftMetadataInitializer() const {
        if (flags & RO_HAS_SWIFT_INITIALIZER) {
            return _swiftMetadataInitializer_NEVER_USE[0];
        } else {
            return nil;
        }
    }

    const char *getName() const {
        return name.load(std::memory_order_acquire);
    }

    class_ro_t *duplicate() const {
        bool hasSwiftInitializer = flags & RO_HAS_SWIFT_INITIALIZER;

        size_t size = sizeof(*this);
        if (hasSwiftInitializer)
            size += sizeof(_swiftMetadataInitializer_NEVER_USE[0]);

        class_ro_t *ro = (class_ro_t *)memdup(this, size);

        if (hasSwiftInitializer)
            ro->_swiftMetadataInitializer_NEVER_USE[0] = this->_swiftMetadataInitializer_NEVER_USE[0];

#if __has_feature(ptrauth_calls)
        // Re-sign the method list pointer.
        ro->baseMethods = baseMethods;
#endif

        return ro;
    }

    Class getNonMetaclass() const {
        ASSERT(flags & RO_META);
        return nonMetaclass;
    }

    const uint8_t *getIvarLayout() const {
        if (flags & RO_META)
            return nullptr;
        return ivarLayout;
    }
};


/***********************************************************************
* list_array_tt<Element, List, Ptr>
* Generic implementation for metadata that can be augmented by categories.
*
* Element is the underlying metadata type (e.g. method_t)
* List is the metadata's list type (e.g. method_list_t)
* List is a template applied to Element to make Element*. Useful for
* applying qualifiers to the pointer type.
*
* A list_array_tt has one of three values:
* - empty
* - a pointer to a single list
* - an array of pointers to lists
*
* countLists/beginLists/endLists iterate the metadata lists
* count/begin/end iterate the underlying metadata elements
**********************************************************************/
template <typename Element, typename List, template<typename> class Ptr>
class list_array_tt {
    struct array_t {
        uint32_t count;
        Ptr<List> lists[0];

        static size_t byteSize(uint32_t count) {
            return sizeof(array_t) + count*sizeof(lists[0]);
        }
        size_t byteSize() {
            return byteSize(count);
        }
    };

 protected:
    template <bool authenticated>
    class iteratorImpl {
        const Ptr<List> *lists;
        const Ptr<List> *listsEnd;

        template<bool B>
        struct ListIterator {
            using Type = typename List::signedIterator;
            static Type begin(Ptr<List> ptr) { return ptr->signedBegin(); }
            static Type end(Ptr<List> ptr) { return ptr->signedEnd(); }
        };
        template<>
        struct ListIterator<false> {
            using Type = typename List::iterator;
            static Type begin(Ptr<List> ptr) { return ptr->begin(); }
            static Type end(Ptr<List> ptr) { return ptr->end(); }
        };
        typename ListIterator<authenticated>::Type m, mEnd;

     public:
        iteratorImpl(const Ptr<List> *begin, const Ptr<List> *end)
            : lists(begin), listsEnd(end)
        {
            if (begin != end) {
                m = ListIterator<authenticated>::begin(*begin);
                mEnd = ListIterator<authenticated>::end(*begin);
            }
        }

        const Element& operator * () const {
            return *m;
        }
        Element& operator * () {
            return *m;
        }

        bool operator != (const iteratorImpl& rhs) const {
            if (lists != rhs.lists) return true;
            if (lists == listsEnd) return false;  // m is undefined
            if (m != rhs.m) return true;
            return false;
        }

        const iteratorImpl& operator ++ () {
            ASSERT(m != mEnd);
            m++;
            if (m == mEnd) {
                ASSERT(lists != listsEnd);
                lists++;
                if (lists != listsEnd) {
                    m = ListIterator<authenticated>::begin(*lists);
                    mEnd = ListIterator<authenticated>::end(*lists);
                }
            }
            return *this;
        }
    };
    using iterator = iteratorImpl<false>;
    using signedIterator = iteratorImpl<true>;

 private:
    union {
        Ptr<List> list;
        uintptr_t arrayAndFlag;
    };

    bool hasArray() const {
        return arrayAndFlag & 1;
    }

    array_t *array() const {
        return (array_t *)(arrayAndFlag & ~1);
    }

    void setArray(array_t *array) {
        arrayAndFlag = (uintptr_t)array | 1;
    }

    void validate() {
        for (auto cursor = beginLists(), end = endLists(); cursor != end; cursor++)
            cursor->validate();
    }

 public:
    list_array_tt() : list(nullptr) { }
    list_array_tt(List *l) : list(l) { }
    list_array_tt(const list_array_tt &other) {
        *this = other;
    }

    list_array_tt &operator =(const list_array_tt &other) {
        if (other.hasArray()) {
            arrayAndFlag = other.arrayAndFlag;
        } else {
            list = other.list;
        }
        return *this;
    }

    uint32_t count() const {
        uint32_t result = 0;
        for (auto lists = beginLists(), end = endLists();
             lists != end;
             ++lists)
        {
            result += (*lists)->count;
        }
        return result;
    }

    iterator begin() const {
        return iterator(beginLists(), endLists());
    }

    iterator end() const {
        auto e = endLists();
        return iterator(e, e);
    }

    signedIterator signedBegin() const {
        return signedIterator(beginLists(), endLists());
    }

    signedIterator signedEnd() const {
        auto e = endLists();
        return signedIterator(e, e);
    }

    inline uint32_t countLists(const std::function<const array_t * (const array_t *)> & peek) const {
        if (hasArray()) {
            return peek(array())->count;
        } else if (list) {
            return 1;
        } else {
            return 0;
        }
    }

    uint32_t countLists() {
        return countLists([](array_t *x) { return x; });
    }

    const Ptr<List>* beginLists() const {
        if (hasArray()) {
            return array()->lists;
        } else {
            return &list;
        }
    }

    const Ptr<List>* endLists() const {
        if (hasArray()) {
            return array()->lists + array()->count;
        } else if (list) {
            return &list + 1;
        } else {
            return &list;
        }
    }

    void attachLists(List* const * addedLists, uint32_t addedCount) {
        if (addedCount == 0) return;

        if (hasArray()) {
            // many lists -> many lists
            uint32_t oldCount = array()->count;
            uint32_t newCount = oldCount + addedCount;
            array_t *newArray = (array_t *)malloc(array_t::byteSize(newCount));
            newArray->count = newCount;
            array()->count = newCount;

            for (int i = oldCount - 1; i >= 0; i--)
                newArray->lists[i + addedCount] = array()->lists[i];
            for (unsigned i = 0; i < addedCount; i++)
                newArray->lists[i] = addedLists[i];
            free(array());
            setArray(newArray);
            validate();
        }
        else if (!list  &&  addedCount == 1) {
            // 0 lists -> 1 list
            list = addedLists[0];
            validate();
        }
        else {
            // 1 list -> many lists
            Ptr<List> oldList = list;
            uint32_t oldCount = oldList ? 1 : 0;
            uint32_t newCount = oldCount + addedCount;
            setArray((array_t *)malloc(array_t::byteSize(newCount)));
            array()->count = newCount;
            if (oldList) array()->lists[addedCount] = oldList;
            for (unsigned i = 0; i < addedCount; i++)
                array()->lists[i] = addedLists[i];
            validate();
        }
    }

    void tryFree() {
        if (hasArray()) {
            for (uint32_t i = 0; i < array()->count; i++) {
                try_free(stripTBI(array()->lists[i]));
            }
            try_free(array());
        }
        else if (list) {
            try_free(stripTBI(list));
        }
    }

    template<typename Other>
    void duplicateInto(Other &other) {
        if (hasArray()) {
            array_t *a = array();
            other.setArray((array_t *)memdup(a, a->byteSize()));
            for (uint32_t i = 0; i < a->count; i++) {
                other.array()->lists[i] = a->lists[i]->duplicate();
            }
        } else if (list) {
            other.list = list->duplicate();
        } else {
            other.list = nil;
        }
    }
};


DECLARE_AUTHED_PTR_TEMPLATE(method_list_t)

class method_array_t :
    public list_array_tt<method_t, method_list_t, method_list_t_authed_ptr>
{
    typedef list_array_tt<method_t, method_list_t, method_list_t_authed_ptr> Super;

 public:
    method_array_t() : Super() { }
    method_array_t(method_list_t *l) : Super(l) { }

    const method_list_t_authed_ptr<method_list_t> *beginCategoryMethodLists() const {
        return beginLists();
    }

    const method_list_t_authed_ptr<method_list_t> *endCategoryMethodLists(Class cls) const;
};


class property_array_t :
    public list_array_tt<property_t, property_list_t, RawPtr>
{
    typedef list_array_tt<property_t, property_list_t, RawPtr> Super;

 public:
    property_array_t() : Super() { }
    property_array_t(property_list_t *l) : Super(l) { }
};


class protocol_array_t :
    public list_array_tt<protocol_ref_t, protocol_list_t, RawPtr>
{
    typedef list_array_tt<protocol_ref_t, protocol_list_t, RawPtr> Super;

 public:
    protocol_array_t() : Super() { }
    protocol_array_t(protocol_list_t *l) : Super(l) { }
};

struct class_rw_ext_t {
    DECLARE_AUTHED_PTR_TEMPLATE(class_ro_t)
    class_ro_t_authed_ptr<const class_ro_t> ro;
    method_array_t methods;
    property_array_t properties;
    protocol_array_t protocols;
    const char *demangledName;
    uint32_t version;
};

struct class_rw_t {
    // Be warned that Symbolication knows the layout of this structure.
    uint32_t flags;
    uint16_t witness;
#if SUPPORT_INDEXED_ISA
    uint16_t index;
#endif

    explicit_atomic<uintptr_t> ro_or_rw_ext;

    Class firstSubclass;
    Class nextSiblingClass;

private:
    using ro_or_rw_ext_t = objc::PointerUnion<const class_ro_t, class_rw_ext_t, PTRAUTH_STR("class_ro_t"), PTRAUTH_STR("class_rw_ext_t")>;

    const ro_or_rw_ext_t get_ro_or_rwe() const {
        return ro_or_rw_ext_t{ro_or_rw_ext};
    }

    void set_ro_or_rwe(const class_ro_t *ro) {
        ro_or_rw_ext_t{ro, &ro_or_rw_ext}.storeAt(ro_or_rw_ext, memory_order_relaxed);
    }

    void set_ro_or_rwe(class_rw_ext_t *rwe, const class_ro_t *ro) {
        // the release barrier is so that the class_rw_ext_t::ro initialization
        // is visible to lockless readers
        rwe->ro = ro;
        ro_or_rw_ext_t{rwe, &ro_or_rw_ext}.storeAt(ro_or_rw_ext, memory_order_release);
    }

    class_rw_ext_t *extAlloc(const class_ro_t *ro, bool deep = false);

public:
    void setFlags(uint32_t set)
    {
        __c11_atomic_fetch_or((_Atomic(uint32_t) *)&flags, set, __ATOMIC_RELAXED);
    }

    void clearFlags(uint32_t clear)
    {
        __c11_atomic_fetch_and((_Atomic(uint32_t) *)&flags, ~clear, __ATOMIC_RELAXED);
    }

    // set and clear must not overlap
    void changeFlags(uint32_t set, uint32_t clear)
    {
        ASSERT((set & clear) == 0);

        uint32_t oldf, newf;
        do {
            oldf = flags;
            newf = (oldf | set) & ~clear;
        } while (!CompareAndSwap(oldf, newf, &flags));
    }

    class_rw_ext_t *ext() const {
        return get_ro_or_rwe().dyn_cast<class_rw_ext_t *>(&ro_or_rw_ext);
    }

    class_rw_ext_t *extAllocIfNeeded() {
        auto v = get_ro_or_rwe();
        if (fastpath(v.is<class_rw_ext_t *>())) {
            return v.get<class_rw_ext_t *>(&ro_or_rw_ext);
        } else {
            return extAlloc(v.get<const class_ro_t *>(&ro_or_rw_ext));
        }
    }

    class_rw_ext_t *deepCopy(const class_ro_t *ro) {
        return extAlloc(ro, true);
    }

    const class_ro_t *ro() const {
        auto v = get_ro_or_rwe();
        if (slowpath(v.is<class_rw_ext_t *>())) {
            return v.get<class_rw_ext_t *>(&ro_or_rw_ext)->ro;
        }
        return v.get<const class_ro_t *>(&ro_or_rw_ext);
    }

    void set_ro(const class_ro_t *ro) {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            v.get<class_rw_ext_t *>(&ro_or_rw_ext)->ro = ro;
        } else {
            set_ro_or_rwe(ro);
        }
    }

    const method_array_t methods() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>(&ro_or_rw_ext)->methods;
        } else {
            return method_array_t{v.get<const class_ro_t *>(&ro_or_rw_ext)->baseMethods};
        }
    }

    const property_array_t properties() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>(&ro_or_rw_ext)->properties;
        } else {
            return property_array_t{v.get<const class_ro_t *>(&ro_or_rw_ext)->baseProperties};
        }
    }

    const protocol_array_t protocols() const {
        auto v = get_ro_or_rwe();
        if (v.is<class_rw_ext_t *>()) {
            return v.get<class_rw_ext_t *>(&ro_or_rw_ext)->protocols;
        } else {
            return protocol_array_t{v.get<const class_ro_t *>(&ro_or_rw_ext)->baseProtocols};
        }
    }
};

namespace objc {
    extern uintptr_t ptrauth_class_rx_enforce disableEnforceClassRXPtrAuth;
}

struct class_data_bits_t {
    friend objc_class;

    // Values are the FAST_ flags above.
    uintptr_t bits;
private:
    bool getBit(uintptr_t bit) const
    {
        return bits & bit;
    }

    // Atomically set the bits in `set` and clear the bits in `clear`.
    // set and clear must not overlap.  If the existing bits field is zero,
    // this function will mark it as using the RW signing scheme.
    void setAndClearBits(uintptr_t set, uintptr_t clear)
    {
        ASSERT((set & clear) == 0);
        uintptr_t newBits, oldBits = LoadExclusive(&bits);
        do {
            uintptr_t authBits
                = (oldBits
                   ? (uintptr_t)ptrauth_auth_data((class_rw_t *)oldBits,
                                                  CLASS_DATA_BITS_RW_SIGNING_KEY,
                                                  ptrauth_blend_discriminator(&bits,
                                                                              CLASS_DATA_BITS_RW_DISCRIMINATOR))
                   : FAST_IS_RW_POINTER);
            newBits = (authBits | set) & ~clear;
            newBits = (uintptr_t)ptrauth_sign_unauthenticated((class_rw_t *)newBits,
                                                              CLASS_DATA_BITS_RW_SIGNING_KEY,
                                                              ptrauth_blend_discriminator(&bits,
                                                                                          CLASS_DATA_BITS_RW_DISCRIMINATOR));
        } while (slowpath(!StoreReleaseExclusive(&bits, &oldBits, newBits)));
    }

    void setBits(uintptr_t set) {
        setAndClearBits(set, 0);
    }

    void clearBits(uintptr_t clear) {
        setAndClearBits(0, clear);
    }

public:

    void copyRWFrom(const class_data_bits_t &other) {
        bits = (uintptr_t)ptrauth_auth_and_resign((class_rw_t *)other.bits,
                                                  CLASS_DATA_BITS_RW_SIGNING_KEY,
                                                  ptrauth_blend_discriminator(&other.bits,
                                                                              CLASS_DATA_BITS_RW_DISCRIMINATOR),
                                                  CLASS_DATA_BITS_RW_SIGNING_KEY,
                                                  ptrauth_blend_discriminator(&bits,
                                                                              CLASS_DATA_BITS_RW_DISCRIMINATOR));
    }

    void copyROFrom(const class_data_bits_t &other, bool authenticate) {
        ASSERT((flags() & RO_REALIZED) == 0);
        if (authenticate) {
            bits = (uintptr_t)ptrauth_auth_and_resign((class_ro_t *)other.bits,
                                                      CLASS_DATA_BITS_RO_SIGNING_KEY,
                                                      ptrauth_blend_discriminator(&other.bits,
                                                                                  CLASS_DATA_BITS_RO_DISCRIMINATOR),
                                                      CLASS_DATA_BITS_RO_SIGNING_KEY,
                                                      ptrauth_blend_discriminator(&bits,
                                                                                  CLASS_DATA_BITS_RO_DISCRIMINATOR));
        } else {
            bits = other.bits;
        }
    }

    class_rw_t* data() const {
#if __BUILDING_OBJCDT__
        return (class_rw_t *)((uintptr_t)ptrauth_strip((class_rw_t *)bits,
                                                           CLASS_DATA_BITS_RW_SIGNING_KEY) & FAST_DATA_MASK);
#else
        return (class_rw_t *)((uintptr_t)ptrauth_auth_data((class_rw_t *)bits,
                                                           CLASS_DATA_BITS_RW_SIGNING_KEY,
                                                           ptrauth_blend_discriminator(&bits,
                                                                                       CLASS_DATA_BITS_RW_DISCRIMINATOR)) & FAST_DATA_MASK);
#endif

    }
    void setData(class_rw_t *newData)
    {
        ASSERT(!(bits & FAST_IS_RW_POINTER)
               || (newData->flags & (RW_REALIZING | RW_FUTURE)));

        uintptr_t authedBits;

        if (objc::disableEnforceClassRXPtrAuth) {
            authedBits = (uintptr_t)ptrauth_strip((const void *)bits,
                                                  CLASS_DATA_BITS_RW_SIGNING_KEY);
        } else {
            if (!bits)
                authedBits = 0;
            else if (bits & FAST_IS_RW_POINTER) {
                authedBits = (uintptr_t)ptrauth_auth_data((class_rw_t *)bits,
                                                          CLASS_DATA_BITS_RW_SIGNING_KEY,
                                                          ptrauth_blend_discriminator(&bits,
                                                                                      CLASS_DATA_BITS_RW_DISCRIMINATOR));
            } else {
                authedBits = (uintptr_t)ptrauth_auth_data((class_ro_t *)bits,
                                                          CLASS_DATA_BITS_RO_SIGNING_KEY,
                                                          ptrauth_blend_discriminator(&bits,
                                                                                      CLASS_DATA_BITS_RO_DISCRIMINATOR));
            }
        }

        // Set during realization or construction only. No locking needed.
        // Use a store-release fence because there may be concurrent
        // readers of data and data's contents.
        uintptr_t newBits = ((authedBits & FAST_FLAGS_MASK)
                             | (uintptr_t)newData
                             | FAST_IS_RW_POINTER);
        class_rw_t *signedData
            = ptrauth_sign_unauthenticated((class_rw_t *)newBits,
                                           CLASS_DATA_BITS_RW_SIGNING_KEY,
                                           ptrauth_blend_discriminator(&bits,
                                                                       CLASS_DATA_BITS_RW_DISCRIMINATOR));
        atomic_thread_fence(memory_order_release);
        bits = (uintptr_t)signedData;
    }

    // Get the class's ro data, even in the presence of concurrent realization.
    // fixme this isn't really safe without a compiler barrier at least
    // and probably a memory barrier when realizeClass changes the data field
    template <Authentication authentication = Authentication::Authenticate>
    const class_ro_t *safe_ro() const {
#if FAST_IS_RW_POINTER
        if (bits & FAST_IS_RW_POINTER) {
            return data()->ro();
        } else {
            uintptr_t authedBits;
            if (authentication == Authentication::Strip || objc::disableEnforceClassRXPtrAuth) {
                authedBits = (uintptr_t)ptrauth_strip((const void *)bits,
                                                      CLASS_DATA_BITS_RO_SIGNING_KEY);
            } else {
                authedBits = (uintptr_t)ptrauth_auth_data((const void *)bits,
                                                          CLASS_DATA_BITS_RO_SIGNING_KEY,
                                                          ptrauth_blend_discriminator(&bits,
                                                                                      CLASS_DATA_BITS_RO_DISCRIMINATOR));
            }
            return (const class_ro_t *)(authedBits & FAST_DATA_MASK);
        }
#else
        class_rw_t *maybe_rw = (class_rw_t *)(bits & FAST_DATA_MASK);
        if (maybe_rw->flags & RW_REALIZED) {
            // maybe_rw is rw
            return data()->ro();
        } else {
            // maybe_rw is actually ro
            return (class_ro_t *)maybe_rw;
        }
#endif
    }

    // This intentionally DOES NOT check the signatures
    uint32_t flags() const {
        static_assert(offsetof(class_rw_t, flags) == 0
                      && offsetof(class_ro_t, flags) == 0, "flags at start");
#if FAST_IS_RW_POINTER
        const uint32_t *pflags;
        if (bits & FAST_IS_RW_POINTER) {
            pflags = ptrauth_strip((const uint32_t *)bits,
                                   CLASS_DATA_BITS_RW_SIGNING_KEY);
        } else {
            pflags = ptrauth_strip((const uint32_t *)bits,
                                   CLASS_DATA_BITS_RO_SIGNING_KEY);
        }
        pflags = (const uint32_t *)((uintptr_t)pflags & FAST_DATA_MASK);

        return *pflags;
#else
        return *(const uint32_t *)(bits & FAST_DATA_MASK);
#endif
    }

#if SUPPORT_INDEXED_ISA
    void setClassArrayIndex(unsigned Idx) {
        // 0 is unused as then we can rely on zero-initialisation from calloc.
        ASSERT(Idx > 0);
        data()->index = Idx;
    }
#else
    void setClassArrayIndex(__unused unsigned Idx) {
    }
#endif

    unsigned classArrayIndex() {
#if SUPPORT_INDEXED_ISA
        return data()->index;
#else
        return 0;
#endif
    }

    bool isAnySwift() {
        return isSwiftStable() || isSwiftLegacy();
    }

    bool isSwiftStable() {
        return getBit(FAST_IS_SWIFT_STABLE);
    }
    void setIsSwiftStable() {
        setAndClearBits(FAST_IS_SWIFT_STABLE, FAST_IS_SWIFT_LEGACY);
    }

    bool isSwiftLegacy() {
        return getBit(FAST_IS_SWIFT_LEGACY);
    }
    void setIsSwiftLegacy() {
        setAndClearBits(FAST_IS_SWIFT_LEGACY, FAST_IS_SWIFT_STABLE);
    }

    // Only for use on unrealized classes
    void setIsSwiftStableRO() {
        uintptr_t authedBits;

        ASSERT(!(bits & FAST_IS_RW_POINTER));

        if (objc::disableEnforceClassRXPtrAuth) {
            authedBits = (uintptr_t)ptrauth_strip((const void *)bits,
                                                  CLASS_DATA_BITS_RO_SIGNING_KEY);
        } else {
            authedBits = (uintptr_t)ptrauth_auth_data((class_ro_t *)bits,
                                                      CLASS_DATA_BITS_RO_SIGNING_KEY,
                                                      ptrauth_blend_discriminator(&bits,
                                                                                      CLASS_DATA_BITS_RO_DISCRIMINATOR));
        }

        uintptr_t newBits = ((authedBits & ~FAST_IS_SWIFT_LEGACY)
                             | FAST_IS_SWIFT_STABLE);
        bits = (uintptr_t)ptrauth_sign_unauthenticated((class_ro_t *)newBits,
                                                       CLASS_DATA_BITS_RO_SIGNING_KEY,
                                                       ptrauth_blend_discriminator(&bits,
                                                                                   CLASS_DATA_BITS_RO_DISCRIMINATOR));
    }

    // fixme remove this once the Swift runtime uses the stable bits
    bool isSwiftStable_ButAllowLegacyForNow() {
        return isAnySwift();
    }

    _objc_swiftMetadataInitializer swiftMetadataInitializer() {
        // This function is called on un-realized classes without
        // holding any locks.
        // Beware of races with other realizers.
        return safe_ro()->swiftMetadataInitializer();
    }
};


struct objc_class : objc_object {
  objc_class(const objc_class&) = delete;
  objc_class(objc_class&&) = delete;
  void operator=(const objc_class&) = delete;
  void operator=(objc_class&&) = delete;
    // Class ISA;
    Class superclass;
    cache_t cache;             // formerly cache pointer and vtable
    class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags

    Class getSuperclass() const {
#if __has_feature(ptrauth_calls)
#   if ISA_SIGNING_AUTH_MODE == ISA_SIGNING_AUTH
        if (superclass == Nil)
            return Nil;

#if SUPERCLASS_SIGNING_TREAT_UNSIGNED_AS_NIL
        void *stripped = ptrauth_strip((void *)superclass, ISA_SIGNING_KEY);
        if ((void *)superclass == stripped) {
            void *resigned = ptrauth_sign_unauthenticated(stripped, ISA_SIGNING_KEY, ptrauth_blend_discriminator(&superclass, ISA_SIGNING_DISCRIMINATOR_CLASS_SUPERCLASS));
            if ((void *)superclass != resigned)
                return Nil;
        }
#endif

        void *result = ptrauth_auth_data((void *)superclass, ISA_SIGNING_KEY, ptrauth_blend_discriminator(&superclass, ISA_SIGNING_DISCRIMINATOR_CLASS_SUPERCLASS));
        return (Class)result;

#   else
        return (Class)ptrauth_strip((void *)superclass, ISA_SIGNING_KEY);
#   endif
#else
        return superclass;
#endif
    }

    void setSuperclass(Class newSuperclass) {
#if ISA_SIGNING_SIGN_MODE == ISA_SIGNING_SIGN_ALL
        superclass = (Class)ptrauth_sign_unauthenticated((void *)newSuperclass, ISA_SIGNING_KEY, ptrauth_blend_discriminator(&superclass, ISA_SIGNING_DISCRIMINATOR_CLASS_SUPERCLASS));
#else
        superclass = newSuperclass;
#endif
    }

    class_rw_t *data() const {
        return bits.data();
    }
    void setData(class_rw_t *newData) {
        bits.setData(newData);
    }

    const class_ro_t *safe_ro() const {
        return bits.safe_ro();
    }

    void setInfo(uint32_t set) {
        ASSERT(isFuture()  ||  isRealized());
        data()->setFlags(set);
    }

    void clearInfo(uint32_t clear) {
        ASSERT(isFuture()  ||  isRealized());
        data()->clearFlags(clear);
    }

    // set and clear must not overlap
    void changeInfo(uint32_t set, uint32_t clear) {
        ASSERT(isFuture()  ||  isRealized());
        ASSERT((set & clear) == 0);
        data()->changeFlags(set, clear);
    }

#if FAST_HAS_DEFAULT_RR
    bool hasCustomRR() const {
        return !bits.getBit(FAST_HAS_DEFAULT_RR);
    }
    void setHasDefaultRR() {
        bits.setBits(FAST_HAS_DEFAULT_RR);
    }
    void setHasCustomRR() {
        bits.clearBits(FAST_HAS_DEFAULT_RR);
    }
#else
    bool hasCustomRR() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_RR);
    }
    void setHasDefaultRR() {
        bits.data()->setFlags(RW_HAS_DEFAULT_RR);
    }
    void setHasCustomRR() {
        bits.data()->clearFlags(RW_HAS_DEFAULT_RR);
    }
#endif

#if FAST_CACHE_HAS_CUSTOM_DEALLOC_INITIATION
    bool hasCustomDeallocInitiation() const {
        return cache.getBit(FAST_CACHE_HAS_CUSTOM_DEALLOC_INITIATION);
    }

    void setHasCustomDeallocInitiation() {
        cache.setBit(FAST_CACHE_HAS_CUSTOM_DEALLOC_INITIATION);
    }
#else
    bool hasCustomDeallocInitiation() const {
        return false;
    }

    void setHasCustomDeallocInitiation() {
        _objc_fatal("_class_setCustomDeallocInitiation is not supported on this target");
    }
#endif

#if FAST_CACHE_HAS_DEFAULT_AWZ
    bool hasCustomAWZ() const {
        return !cache.getBit(FAST_CACHE_HAS_DEFAULT_AWZ);
    }
    void setHasDefaultAWZ() {
        cache.setBit(FAST_CACHE_HAS_DEFAULT_AWZ);
    }
    void setHasCustomAWZ() {
        cache.clearBit(FAST_CACHE_HAS_DEFAULT_AWZ);
    }
#else
    bool hasCustomAWZ() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_AWZ);
    }
    void setHasDefaultAWZ() {
        bits.data()->setFlags(RW_HAS_DEFAULT_AWZ);
    }
    void setHasCustomAWZ() {
        bits.data()->clearFlags(RW_HAS_DEFAULT_AWZ);
    }
#endif

#if FAST_CACHE_HAS_DEFAULT_CORE
    bool hasCustomCore() const {
        return !cache.getBit(FAST_CACHE_HAS_DEFAULT_CORE);
    }
    void setHasDefaultCore() {
        return cache.setBit(FAST_CACHE_HAS_DEFAULT_CORE);
    }
    void setHasCustomCore() {
        return cache.clearBit(FAST_CACHE_HAS_DEFAULT_CORE);
    }
#else
    bool hasCustomCore() const {
        return !(bits.data()->flags & RW_HAS_DEFAULT_CORE);
    }
    void setHasDefaultCore() {
        bits.data()->setFlags(RW_HAS_DEFAULT_CORE);
    }
    void setHasCustomCore() {
        bits.data()->clearFlags(RW_HAS_DEFAULT_CORE);
    }
#endif

#if FAST_CACHE_HAS_CXX_CTOR
    bool hasCxxCtor() {
        ASSERT(isRealized());
        return cache.getBit(FAST_CACHE_HAS_CXX_CTOR);
    }
    void setHasCxxCtor() {
        cache.setBit(FAST_CACHE_HAS_CXX_CTOR);
    }
#else
    bool hasCxxCtor() {
        ASSERT(isRealized());
        return bits.data()->flags & RW_HAS_CXX_CTOR;
    }
    void setHasCxxCtor() {
        bits.data()->setFlags(RW_HAS_CXX_CTOR);
    }
#endif

#if FAST_CACHE_HAS_CXX_DTOR
    bool hasCxxDtor() {
        ASSERT(isRealized());
        return cache.getBit(FAST_CACHE_HAS_CXX_DTOR);
    }
    void setHasCxxDtor() {
        cache.setBit(FAST_CACHE_HAS_CXX_DTOR);
    }
#else
    bool hasCxxDtor() {
        ASSERT(isRealized());
        return bits.data()->flags & RW_HAS_CXX_DTOR;
    }
    void setHasCxxDtor() {
        bits.data()->setFlags(RW_HAS_CXX_DTOR);
    }
#endif

#if FAST_CACHE_REQUIRES_RAW_ISA
    bool instancesRequireRawIsa() {
        return cache.getBit(FAST_CACHE_REQUIRES_RAW_ISA);
    }
    void setInstancesRequireRawIsa() {
        cache.setBit(FAST_CACHE_REQUIRES_RAW_ISA);
    }
#elif SUPPORT_NONPOINTER_ISA
    bool instancesRequireRawIsa() {
        return bits.data()->flags & RW_REQUIRES_RAW_ISA;
    }
    void setInstancesRequireRawIsa() {
        bits.data()->setFlags(RW_REQUIRES_RAW_ISA);
    }
#else
    bool instancesRequireRawIsa() {
        return true;
    }
    void setInstancesRequireRawIsa() {
        // nothing
    }
#endif
    void setInstancesRequireRawIsaRecursively(bool inherited = false);
    void printInstancesRequireRawIsa(bool inherited);

#if CONFIG_USE_PREOPT_CACHES
    bool allowsPreoptCaches() const {
        return !(bits.data()->flags & RW_NOPREOPT_CACHE);
    }
    bool allowsPreoptInlinedSels() const {
        return !(bits.data()->flags & RW_NOPREOPT_SELS);
    }
    void setDisallowPreoptCaches() {
        bits.data()->setFlags(RW_NOPREOPT_CACHE | RW_NOPREOPT_SELS);
    }
    void setDisallowPreoptInlinedSels() {
        bits.data()->setFlags(RW_NOPREOPT_SELS);
    }
    void setDisallowPreoptCachesRecursively(const char *why);
    void setDisallowPreoptInlinedSelsRecursively(const char *why);
#else
    bool allowsPreoptCaches() const { return false; }
    bool allowsPreoptInlinedSels() const { return false; }
    void setDisallowPreoptCaches() { }
    void setDisallowPreoptInlinedSels() { }
    void setDisallowPreoptCachesRecursively(const char *why) { }
    void setDisallowPreoptInlinedSelsRecursively(const char *why) { }
#endif

    bool canAllocNonpointer() {
        ASSERT(!isFuture());
        return !instancesRequireRawIsa();
    }

    bool isSwiftStable() {
        return bits.isSwiftStable();
    }

    bool isSwiftLegacy() {
        return bits.isSwiftLegacy();
    }

    bool isAnySwift() {
        return bits.isAnySwift();
    }

    bool isSwiftStable_ButAllowLegacyForNow() {
        return bits.isSwiftStable_ButAllowLegacyForNow();
    }

    uint32_t swiftClassFlags() {
        return *(uint32_t *)(&bits + 1);
    }

    bool usesSwiftRefcounting() {
        if (!isSwiftStable()) return false;
        return bool(swiftClassFlags() & 2); //ClassFlags::UsesSwiftRefcounting
    }

    bool canCallSwiftRR() {
        // !hasCustomCore() is being used as a proxy for isInitialized(). All
        // classes with Swift refcounting are !hasCustomCore() (unless there are
        // category or swizzling shenanigans), but that bit is not set until a
        // class is initialized. Checking isInitialized requires an extra
        // indirection that we want to avoid on RR fast paths.
        //
        // In the unlikely event that someone causes a class with Swift
        // refcounting to be hasCustomCore(), we'll fall back to sending -retain
        // or -release, which is still correct.
        return !hasCustomCore() && usesSwiftRefcounting();
    }

    bool isStubClass() const {
        uintptr_t isa = (uintptr_t)isaBits();
        return 1 <= isa && isa < 16;
    }

    // Swift stable ABI built for old deployment targets looks weird.
    // The is-legacy bit is set for compatibility with old libobjc.
    // We are on a "new" deployment target so we need to rewrite that bit.
    // These stable-with-legacy-bit classes are distinguished from real
    // legacy classes using another bit in the Swift data
    // (ClassFlags::IsSwiftPreStableABI)

    bool isUnfixedBackwardDeployingStableSwift() {
        // Only classes marked as Swift legacy need apply.
        if (!bits.isSwiftLegacy()) return false;

        // Check the true legacy vs stable distinguisher.
        // The low bit of Swift's ClassFlags is SET for true legacy
        // and UNSET for stable pretending to be legacy.
        bool isActuallySwiftLegacy = bool(swiftClassFlags() & 1);
        return !isActuallySwiftLegacy;
    }

    void fixupBackwardDeployingStableSwift() {
        if (isUnfixedBackwardDeployingStableSwift()) {
            // Class really is stable Swift, pretending to be pre-stable.
            // Fix its lie.

            // N.B. At this point, bits is *always* a class_ro pointer; we
            // can't use setIsSwiftStable() because that only works for a
            // class_rw pointer.
            bits.setIsSwiftStableRO();
        }
    }

    _objc_swiftMetadataInitializer swiftMetadataInitializer() {
        return bits.swiftMetadataInitializer();
    }

    // Return YES if the class's ivars are managed by ARC,
    // or the class is MRC but has ARC-style weak ivars.
    bool hasAutomaticIvars() {
        return data()->ro()->flags & (RO_IS_ARC | RO_HAS_WEAK_WITHOUT_ARC);
    }

    // Return YES if the class's ivars are managed by ARC.
    bool isARC() {
        return data()->ro()->flags & RO_IS_ARC;
    }


    bool forbidsAssociatedObjects() {
        return (data()->flags & RW_FORBIDS_ASSOCIATED_OBJECTS);
    }

#if SUPPORT_NONPOINTER_ISA
    // Tracked in non-pointer isas; not tracked otherwise
#else
    bool instancesHaveAssociatedObjects() {
        // this may be an unrealized future class in the CF-bridged case
        ASSERT(isFuture()  ||  isRealized());
        return data()->flags & RW_INSTANCES_HAVE_ASSOCIATED_OBJECTS;
    }

    void setInstancesHaveAssociatedObjects() {
        // this may be an unrealized future class in the CF-bridged case
        ASSERT(isFuture()  ||  isRealized());
        setInfo(RW_INSTANCES_HAVE_ASSOCIATED_OBJECTS);
    }
#endif

    bool shouldGrowCache() {
        return true;
    }

    void setShouldGrowCache(bool) {
        // fixme good or bad for memory use?
    }

    // 查询是否正在初始化（initializing）
    bool isInitializing() {
        return getMeta()->bits.flags() & RW_INITIALIZING;
    }

    // 设置正在初始化
    void setInitializing() {
        ASSERT(!isMetaClass());
        ISA()->setInfo(RW_INITIALIZING);
    }

    // 是否完成初始化
    bool isInitialized() {
        return getMeta()->bits.flags() & RW_INITIALIZED;
    }

    void setInitialized();

    bool isLoadable() {
        ASSERT(isRealized());
        return true;  // any class registered for +load is definitely loadable
    }

    // 获取load方法的IMP
    IMP getLoadMethod();

    // Locking: To prevent concurrent realization, hold runtimeLock.
    bool isRealized() const {
        return !isStubClass() && (bits.flags() & RW_REALIZED);
    }

    // Returns true if this is an unrealized future class.
    // Locking: To prevent concurrent realization, hold runtimeLock.
    bool isFuture() const {
        if (isStubClass())
            return false;
        return bits.flags() & RW_FUTURE;
    }

    bool isMetaClass() const {
        ASSERT_THIS_NOT_NULL;
        ASSERT(isRealized());
#if FAST_CACHE_META
        return cache.getBit(FAST_CACHE_META);
#else
        return data()->flags & RW_META;
#endif
    }

    // Like isMetaClass, but also valid on un-realized classes
    bool isMetaClassMaybeUnrealized() {
        static_assert(offsetof(class_rw_t, flags) == offsetof(class_ro_t, flags), "flags alias");
        static_assert(RO_META == RW_META, "flags alias");
        if (isStubClass())
            return false;
        return bits.flags() & RW_META;
    }

    // NOT identical to this->ISA when this is a metaclass
    Class getMeta() {
        if (isMetaClassMaybeUnrealized()) return (Class)this;
        else return this->ISA();
    }

    bool isRootClass() {
        return getSuperclass() == nil;
    }
    bool isRootMetaclass() {
        return ISA() == (Class)this;
    }

    // If this class does not have a name already, we can ask Swift to construct one for us.
    const char *installMangledNameForLazilyNamedClass();

    // Get the class's mangled name, or NULL if the class has a lazy
    // name that hasn't been created yet.
    const char *nonlazyMangledName() const {
        return bits.safe_ro()->getName();
    }

    const char *mangledName() {
        // fixme can't assert locks here
        ASSERT_THIS_NOT_NULL;

        const char *result = nonlazyMangledName();

        if (!result) {
            // This class lazily instantiates its name. Emplace and
            // return it.
            result = installMangledNameForLazilyNamedClass();
        }

        return result;
    }

    // Get the class's mangled name, or NULL if it has a lazy name that hasn't
    // been created yet, WITHOUT authenticating the signed class_ro pointer.
    // This exists sosely for objc_debug_class_getNameRaw to use.
    const char *rawUnsafeMangledName() const {
        // Strip the class_ro pointer instead of authenticating so that we can
        // handle classes without signed class_ro pointers in the shared cache
        // even if they haven't been officially loaded yet. rdar://90415774
        return bits.safe_ro<Authentication::Strip>()->getName();
    }

    const char *demangledName(bool needsLock);
    const char *nameForLogging();

    // May be unaligned depending on class's ivars.
    uint32_t unalignedInstanceStart() const {
        ASSERT(isRealized());
        return data()->ro()->instanceStart;
    }

    // Class's instance start rounded up to a pointer-size boundary.
    // This is used for ARC layout bitmaps.
    uint32_t alignedInstanceStart() const {
        return word_align(unalignedInstanceStart());
    }

    // May be unaligned depending on class's ivars.
    uint32_t unalignedInstanceSize() const {
        ASSERT(isRealized());
        return data()->ro()->instanceSize;
    }

    // Class's ivar size rounded up to a pointer-size boundary.
    uint32_t alignedInstanceSize() const {
        return word_align(unalignedInstanceSize());
    }

    inline size_t instanceSize(size_t extraBytes) const {
        if (fastpath(cache.hasFastInstanceSize(extraBytes))) {
            return cache.fastInstanceSize(extraBytes);
        }

        size_t size = alignedInstanceSize() + extraBytes;
        // CF requires all objects be at least 16 bytes.
        if (size < 16) size = 16;
        return size;
    }

    void setInstanceSize(uint32_t newSize) {
        ASSERT(isRealized());
        ASSERT(data()->flags & RW_REALIZING);
        auto ro = data()->ro();
        if (newSize != ro->instanceSize) {
            ASSERT(data()->flags & RW_COPIED_RO);
            *const_cast<uint32_t *>(&ro->instanceSize) = newSize;
        }
        cache.setFastInstanceSize(newSize);
    }

    void chooseClassArrayIndex();

    void setClassArrayIndex(unsigned Idx) {
        bits.setClassArrayIndex(Idx);
    }

    unsigned classArrayIndex() {
        return bits.classArrayIndex();
    }
};


struct swift_class_t : objc_class {
    uint32_t flags;
    uint32_t instanceAddressOffset;
    uint32_t instanceSize;
    uint16_t instanceAlignMask;
    uint16_t reserved;

    uint32_t classSize;
    uint32_t classAddressOffset;
    void *description;
    // ...

    void *baseAddress() {
        return (void *)((uint8_t *)this - classAddressOffset);
    }
};


struct category_t {
    const char *name;
    classref_t cls;
    WrappedPtr<method_list_t, method_list_t::Ptrauth> instanceMethods;
    WrappedPtr<method_list_t, method_list_t::Ptrauth> classMethods;
    struct protocol_list_t *protocols;
    struct property_list_t *instanceProperties;
    // Fields below this point are not always present on disk.
    struct property_list_t *_classProperties;

    method_list_t *methodsForMeta(bool isMeta) {
        if (isMeta) return classMethods;
        else return instanceMethods;
    }

    property_list_t *propertiesForMeta(bool isMeta, struct header_info *hi);

    protocol_list_t *protocolsForMeta(bool isMeta) {
        if (isMeta) return nullptr;
        else return protocols;
    }
};

struct objc_super2 {
    id receiver;
    Class current_class;
};

struct message_ref_t {
    IMP imp;
    SEL sel;
};


extern Method protocol_getMethod(protocol_t *p, SEL sel, bool isRequiredMethod, bool isInstanceMethod, bool recursive);

#endif
