#pragma once

#include <clean-core/vector.hh>

namespace cc
{
/// a union-find data structure with size-based path compression
/// elements are numbered 0..size-1
/// this class manages a disjoint collection of sets, each element belonging to exactly one set
/// a set is characterized by its representative, which is an arbitrary element from the set
///
/// in the literature, this is often called a union-find data structure
/// 'find' corresponds to get_representative
/// 'union' corresponds to merge_sets_by_element / merge_sets_by_representative
///
/// NOTE: even seemingly readonly functions are non-const because
///       the get_representative operation optimizes the data structure for optimal runtime
struct disjoint_set
{
public:
    disjoint_set() = default;

    explicit disjoint_set(size_t size) { init(size); }

    /// number of elements
    size_t number_of_elements() const { return entries.size(); }
    size_t number_of_partitions() const { return partition_count; }

    /// initialized a disjoint set for a given number of entries
    /// in the beginning, all entries are singleton sets
    void init(size_t size)
    {
        entries.resize(size);
        partition_count = size;

        for (auto i = 0; i < int(size); ++i)
        {
            auto& e = entries[i];
            e.parent = i;
            e.size = 1;
        }
    }

    /// returns the size of the set that e belongs to
    int size_of_set_by_element(int e) { return entries[get_representative(e)].size; }

    /// returns the size of the set that e_repr is the representative of
    /// NOTE: e_repr must be the representative element
    int size_of_set_by_representative(int e_repr)
    {
        CC_ASSERT(is_representative(e_repr));
        return entries[e_repr].size;
    }

    /// returns true iff e is the representative of its set
    bool is_representative(int e) { return get_representative(e) == e; }

    /// returns true iff e0 and e1 belong to the same set
    bool are_in_same_set(int e0, int e1) { return get_representative(e0) == get_representative(e1); }

    /// returns the representative element of the set that e belongs to
    int get_representative(int e)
    {
        auto& ee = entries[e];
        if (ee.parent != e)
            ee.parent = get_representative(ee.parent);
        return ee.parent;
    }

    /// returns the parent of this element
    /// if e == get_parent(e), then e is the representative
    /// NOTE: this does not return the representative of e
    /// NOTE: this does not perform path-compaction
    int get_parent(int e) const { return entries[e].parent; }

    /// merges the sets given by two arbitrary elements e0 and e1
    /// returns true iff a merge was done (i.e. if e0 and e1 belonged to different sets)
    bool merge_sets_by_element(int e0, int e1)
    {
        auto e0_repr = get_representative(e0);
        auto e1_repr = get_representative(e1);
        return merge_sets_by_representative(e0_repr, e1_repr);
    }

    /// merges the sets given by two representative elements
    /// returns true iff a merge was done (i.e. if two different elements were provided)
    bool merge_sets_by_representative(int e0_repr, int e1_repr)
    {
        CC_ASSERT(is_representative(e0_repr));
        CC_ASSERT(is_representative(e1_repr));

        if (e0_repr == e1_repr)
            return false;

        // union by size
        if (entries[e0_repr].size < entries[e1_repr].size)
            std::swap(e0_repr, e1_repr);
        // |X| > |Y|

        entries[e1_repr].parent = e0_repr;
        entries[e0_repr].size += entries[e1_repr].size;
        partition_count--;

        return true;
    }

private:
    struct entry
    {
        int parent;
        int size;
    };

private:
    cc::vector<entry> entries;
    size_t partition_count = 0;
};
} // namespace cc
