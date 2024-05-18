#ifndef GRPC_COMMON_CPP_GRID_INDEX_H_
#define GRPC_COMMON_CPP_GRID_INDEX_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include <iostream>
#include <chrono>

#include "global.h"
#include "differentialprivacy.h"

namespace INDEX {

using ICDE18::Record_t;
using ICDE18::Circle_t;
using ICDE18::IntersectWithRange;
using DIFFERENTIALPRIVACY::LaplaceMechanism;

constexpr size_t ipow(size_t base, int exp, size_t result=1) {
    return exp < 1 ? result : ipow(base*base, exp/2, (exp % 2) ? result*base : result);
}

// uniform K by K ... grid 
template<size_t K, size_t dim=2>
class GridIndex {

using Point_t = ICDE18::Record_t;
using Points_t = std::vector<Point_t>;
using Range = std::pair<size_t, size_t>;

public:
    GridIndex(const std::shared_ptr<Points_t>& _points) {
        std::cout << "Construct Uniform Grid K=" << K << std::endl;
        auto start = std::chrono::steady_clock::now();

        points_ptr = _points;
        Points_t& points = *points_ptr;
        this->num_of_points = points.size();

        // dimension offsets when computing bucket ID
        for (size_t i=0; i<dim; ++i) {
            this->dim_offset[i] = ipow(K, i);
        }

        // boundaries of each dimension
        std::fill(mins.begin(), mins.end(), std::numeric_limits<double>::max());
        std::fill(maxs.begin(), maxs.end(), std::numeric_limits<double>::min());

        for (int i=0; i<dim; ++i) {
            double value;
            for (auto& p : points) {
                value = (i==0) ? p.x : p.y;
                mins[i] = std::min(value, mins[i]);
                maxs[i] = std::max(value, maxs[i]);
            }
        }

        // widths of each dimension
        for (size_t i=0; i<dim; ++i) {
            widths[i] = (maxs[i] - mins[i]) / K;
        }
        
        // insert points to buckets
        counts.fill(0);
        for (size_t i=0; i<points.size(); ++i) {
            size_t pid = compute_id(points[i]);
            buckets[pid].emplace_back(i);
            ++counts[pid];
        }

        auto end = std::chrono::steady_clock::now();
        build_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Build Time: " << build_time << " [ms]" << std::endl;
        std::cout << "Index Size: " << index_size() << " Bytes" << std::endl;
    }

    void perturb_index(double epsilon) {
        double grid_epsilon = epsilon / ipow(K, dim);
        for (int i=0; i<counts.size(); ++i) {
            size_t noise = floor(LaplaceMechanism(1, grid_epsilon));
            counts[i] += floor(noise);
        }
    }

    Points_t range_query(ICDE18::Circle_t& circ) {
        auto start = std::chrono::steady_clock::now();

        // transform the circle into a rectangle box
        Point_t min_corner, max_corner;
        min_corner.x = circ.x - circ.rad;
        min_corner.y = circ.y - circ.rad;
        max_corner.x = circ.x + circ.rad;
        max_corner.y = circ.y + circ.rad;

        // bucket ranges that intersect the query box
        std::vector<Range> ranges;

        // search range on the 1-st dimension
        ranges.emplace_back(std::make_pair(get_dim_idx(min_corner, 0), get_dim_idx(max_corner, 0)));
        
        // find all intersect ranges
        for (size_t i=1; i<dim; ++i) {
            auto start_idx = get_dim_idx(min_corner, i);
            auto end_idx = get_dim_idx(max_corner, i);

            std::vector<Range> temp_ranges;
            for (auto idx=start_idx; idx<=end_idx; ++idx) {
                for (size_t j=0; j<ranges.size(); ++j) {
                    temp_ranges.emplace_back(std::make_pair(ranges[j].first + idx*dim_offset[i], ranges[j].second + idx*dim_offset[i]));
                }
            }

            // update the range vector
            ranges = temp_ranges;
        }

        // Points candidates;
        Points_t result;

        // find candidate points
        for (auto range : ranges) {
            size_t start_idx = range.first;
            size_t end_idx = range.second;

            for (size_t idx=start_idx; idx<=end_idx; ++idx) {
                for (auto cand_idx : this->buckets[idx]) {
                    Point_t p = (*points_ptr)[cand_idx];
                    if (IntersectWithRange(p, circ)) {
                        result.emplace_back(p);
                    }
                }
            }
        }

        auto end = std::chrono::steady_clock::now();
        range_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        range_count ++;
        
        return result;
    }

    inline size_t count() {
        return this->num_of_points;
    }

    inline size_t index_size() {
        size_t ret = 0;
        
        ret += sizeof(num_of_points);                                    // num_of_points;
        ret += sizeof(points_ptr);                                       // points_ptr;
        ret += this->buckets.size() * sizeof(std::vector<size_t>);       // buckets
        for (auto bucket : buckets)
            ret += bucket.size() * sizeof(size_t);
        ret += this->counts.size() * sizeof(size_t);                      // counts
        ret += dim * (3 * sizeof(double) + sizeof(size_t));              // others

        return ret;
    }


private:
    double build_time = 0;
    double range_time = 0;
    size_t range_count = 0;
    size_t num_of_points;
    std::shared_ptr<Points_t> points_ptr;
    std::array<std::vector<size_t>, ipow(K, dim)> buckets;
    std::array<size_t, ipow(K, dim)> counts;
    std::array<double, dim> mins;
    std::array<double, dim> maxs;
    std::array<double, dim> widths;
    std::array<size_t, dim> dim_offset;

    // compute the index on d-th dimension of a given point
    inline size_t get_dim_idx(const Point_t& p, const size_t& d) {
        double pd = (d==0) ? p.x : p.y;

        if (pd <= mins[d]) {
            return 0;
        } else if (pd >= maxs[d]) {
            return K-1;
        } else {
            return (size_t) ((pd - mins[d]) / widths[d]);
        }
    }

    // compute the bucket ID of a given point
    inline size_t compute_id(const Point_t& p) {
        size_t id = 0;

        for (size_t i=0; i<dim; ++i) {
            size_t current_idx = get_dim_idx(p, i);
            id += current_idx * dim_offset[i];
        }

        return id;
    }
};

}

#endif // GRPC_COMMON_CPP_GRID_INDEX_H_