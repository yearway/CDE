//
//  CDE.h
//  Channel Division based Enhancement
//
//  Created by yearway (iyearway@gmail.com) on 2/5/15.
//

#ifndef __CDE__
#define __CDE__

#include <iostream>
#include <opencv2/core/core.hpp>

#define __CDE_DEBUG__

typedef unsigned char Intensity;
const Intensity kMaxIntensity = (Intensity)255;

typedef cv::Vec<int, (int)kMaxIntensity+1> CDE_Vec_i;
typedef cv::Vec<float, (int)kMaxIntensity+1> CDE_Vec_f;

class ContrastPair {
public:
    ContrastPair() : low_val_(0), high_val_(0) {};

    ContrastPair(Intensity v1, Intensity v2) {
        low_val_ = std::min(v1, v2);
        high_val_ = std::max(v1, v2);
    };

    ~ContrastPair() {};

    inline CDE_Vec_i getVector() const {
        CDE_Vec_i vec = CDE_Vec_i::all(0);
        for (uint k = low_val_; k <= high_val_; k++)
            vec[k] = 1;
        return vec;
    };

    inline Intensity getLow() const {
        return low_val_;
    };

    inline Intensity getHigh() const {
        return high_val_ ;
    };

private:
    Intensity low_val_, high_val_;
};

// Contrast Division based Enhancement
// Parameters
//  - thresh:   threshold for the contrast pairs. (Default 10)
//  - weight: controls how much of the transformation is used, rest comes from identity. Bigger values gives brighter results.
//  - sigmas:   [drk mdl sat] sigmas values for each channel. (Default [3 1 1/2])
//  - bounds:   [d s] thresholds for the different regions. (Default [1/3 2/3])

class CDE {
public:
    static const ContrastPair noneContrast;

    CDE() :
        thresh_(10),
        weight_(.8f),
        sigmas_(cv::Vec3f(3.f, 1.f, .5f)),
        bounds_(cv::Vec2f(1.f/3, 2.f/3))
    {};

    CDE(int thresh, int val, int step, float weight_t, cv::Vec3f sigmas, cv::Vec2f bounds) :
        thresh_(thresh),
        weight_(weight_t),
        sigmas_(sigmas),
        bounds_(bounds)
    {};

    void enhance(const cv::Mat &in_img, cv::Mat &out_img);

private:
    int thresh_;
    float weight_;
    cv::Vec3f sigmas_;
    cv::Vec2f bounds_;

    inline bool isEdgeContrastPair(const ContrastPair *p) {
        return static_cast<int>(p->getHigh() - p->getLow()) >= thresh_;
    };
};

#endif /* defined(__CDE__) */
