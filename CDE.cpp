//
//  CDE.cpp
//  Channel Division based Enhancement
//
//  Created by yearway (iyearway@gmail.com) on 2/5/15.
//

#include "CDE.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifdef __CDE_DEBUG__
#include "GraphUtils.h"
#endif

using cv::Mat;
using std::vector;
using std::pair;

// helper functions
namespace {

#ifdef __CDE_DEBUG__
    const int kBorder = 20;

    // A simple plotting function, for the ease of visualizing vectors and debugging
    template <int N>
    inline void plot(cv::Vec<float, N> &vec, const char* win_name) {
        std::vector<float> pvec;
        for (int i = 0; i < N; i++)
            pvec.push_back(vec[i]);

        int s = N * 2 + 2*kBorder;
        Mat background;
        Mat graph = drawFloatGraph(pvec, background, 0.f, 1.f, s, s, nullptr);
        cv::imshow(win_name, graph);
        cv::waitKey(0);
        cv::destroyWindow(win_name);
    }
#endif

    const double PI = 3.141592653589793;
    const double kGaussianConstant = 1.0/std::sqrt(2*PI);
    const int kNumNeighbors = 8;
    typedef vector<const ContrastPair *> ContrastPairSet;

    inline CDE_Vec_f identity() {
        CDE_Vec_f trans;
        for (uint i = 0; i < trans.rows; i++)
            trans[i] = (float)i / (kMaxIntensity);
        return trans;
    }

    inline double Gaussian1D(double u, double sigma, double x) {
//        return kGaussianConstant / sigma * std::exp(-(u-x)*(u-x)/(2*sigma*sigma));
        //return std::exp(-(u-x)*(u-x)/(2*sigma*sigma));
        return std::exp(-(u-x)*(u-x)/(sigma*sigma/4));
    }

    inline CDE_Vec_f accumulatePairs(const std::vector<const ContrastPair *> &pairs) {
        CDE_Vec_i f = CDE_Vec_i::all(0);
        for (const ContrastPair* p : pairs)
            f += p->getVector();
        return f;
    }

    CDE_Vec_f generateTransformFunc(const std::vector<const ContrastPair *> &pairs) {
        assert(pairs.size() > 0);

        CDE_Vec_i f = accumulatePairs(pairs);
        CDE_Vec_f F = CDE_Vec_f::all(0.f);
        float total_votes = (float)sum(cv::Mat(f))[0];
        float sum = 0;
        for (uint k = 0; k <= kMaxIntensity; k++) {
            sum += f[k];
            F[k] = sum / total_votes;
        }

        return F;
    }

    void generateContrastPairs(const Mat &img, vector<ContrastPair> &pairs, vector<ContrastPairSet> &pairs_of_pixels) {
        assert(img.channels() == 1);
        int H = img.rows;
        int W = img.cols;

        // indexing of neighbor pairs
        // 1 2 3
        // 0 * 4
        // 7 6 5

        pairs.reserve(H * W * kNumNeighbors / 2);
        pairs_of_pixels.reserve(H * W);

        int idx;
        for (int i = 0; i < H; i++) {
            for (int j = 0; j < W; j++) {
                idx = i*W + j;
                pairs_of_pixels.push_back(ContrastPairSet(8, &CDE::noneContrast));

                // set neighbor 0
                if (j > 0)
                    pairs_of_pixels.at(idx).at(0) = pairs_of_pixels.at(idx - 1).at(4);

                // set neighbor 1
                if (i > 0 && j > 0)
                    pairs_of_pixels.at(idx).at(1) = pairs_of_pixels.at(idx - W - 1).at(5);

                // set neighbor 2
                if (i > 0)
                    pairs_of_pixels.at(idx).at(2) = pairs_of_pixels.at(idx - W).at(6);

                // set neighbor 3
                if (i > 0 && j < W-1)
                    pairs_of_pixels.at(idx).at(3) = pairs_of_pixels.at(idx - W + 1).at(7);

                // set neighbor 4
                if (j < W-1) {
                    pairs.push_back(ContrastPair(img.at<uchar>(i, j), img.at<uchar>(i, j+1)));
                    pairs_of_pixels.at(idx).at(4) = &pairs.back();
                }

                // set neighbor 5
                if (i < H-1 && j < W-1) {
                    pairs.push_back(ContrastPair(img.at<uchar>(i, j), img.at<uchar>(i+1, j+1)));
                    pairs_of_pixels.at(idx).at(5) = &pairs.back();
                }

                // set neighbor 6
                if (i < H-1) {
                    pairs.push_back(ContrastPair(img.at<uchar>(i, j), img.at<uchar>(i+1, j)));
                    pairs_of_pixels.at(idx).at(6) = &pairs.back();
                }

                // set neighbor 7
                if (j > 0 && i < H-1) {
                    pairs.push_back(ContrastPair(img.at<uchar>(i, j), img.at<uchar>(i+1, j-1)));
                    pairs_of_pixels.at(idx).at(7) = &pairs.back();
                }
            }
        }
    }

    void applyTransform(Mat &src, Mat &dst, const CDE_Vec_f &transform_func) {
        assert(src.type() == CV_8UC1);

        dst = Mat(src.size(), CV_8UC1);
        for (int i = 0; i < src.rows; i++) {
            for (int j = 0; j < src.cols; j++) {
                dst.at<uchar>(i, j) = (uchar)std::round(transform_func[src.at<uchar>(i, j)] * kMaxIntensity);
            }
        }
    }

}


/* --- Implementation of CDE class --- */

// a contrast pair with no contrast, i.e., low_val_ = high_val_;
const ContrastPair CDE::noneContrast = ContrastPair();

void CDE::enhance(const cv::Mat &in_img, cv::Mat &out_img) {
    uint i, j, k;
    Mat hsv_img;
    cvtColor(in_img, hsv_img, CV_BGR2HSV_FULL);
    vector<Mat> hsv_channels;
    for (int i = 0; i < 3; i++)
        hsv_channels.push_back(Mat());

    split(hsv_img, hsv_channels);

    vector<ContrastPair> pairs;
    vector<ContrastPairSet> pairs_of_pixels; // neighbor pairs of each pixel
    generateContrastPairs(hsv_channels[2], pairs, pairs_of_pixels);

    vector<ContrastPairSet> pairs_of_intensities(kMaxIntensity+1, ContrastPairSet());

    uint idx = 0;
    const ContrastPair *p;
    for (i = 0; i < (uint)in_img.rows; i++) {
        for (j = 0; j < (uint)in_img.cols; j++) {
            idx = i*in_img.cols + j;
            for (int n = 0; n < kNumNeighbors; n++) {
                p = pairs_of_pixels.at(idx).at(n);
                if (isEdgeContrastPair(p)) {
                    pairs_of_intensities.at(p->getLow()).push_back(p);
                    pairs_of_intensities.at(p->getHigh()).push_back(p);
                }
            }
        }
    }

    vector<pair<Intensity, CDE_Vec_f> > transform_funcs;
    for (k = 0; k <= kMaxIntensity; k++) {
        if (pairs_of_intensities.at(k).size() > 0) {
            transform_funcs.push_back(
                std::make_pair(k, generateTransformFunc(pairs_of_intensities.at(k)))
            );
        }
    }

    // calculate region transform functions for the dark, middle, and bright regions
    double maxI;
    cv::minMaxIdx(hsv_channels[2], nullptr, &maxI);
    uint bound_1 = std::floor((float)maxI * bounds_[0]);
    uint bound_2 = std::floor((float)maxI * bounds_[1]);

    vector<CDE_Vec_f> region_transform_funcs(3, CDE_Vec_f::all(0.f));

    int num_r0 = 0, num_r1 = 0, num_r2 = 0; // num of intensities of each region
    for (uint n = 0; n < transform_funcs.size(); n++) {
        k = transform_funcs.at(n).first;
        if (k <= bound_1) {
            region_transform_funcs[0] += transform_funcs[n].second;
            num_r0++;
        }
        if (k >= bound_1 && k <= bound_2) {
            region_transform_funcs[1] += transform_funcs[n].second;
            num_r1++;
        }
        if (k >= bound_2) {
            region_transform_funcs[2] += transform_funcs[n].second;
            num_r2++;
        }
    }
    region_transform_funcs[0] /= num_r0;
    region_transform_funcs[1] /= num_r1;
    region_transform_funcs[2] /= num_r2;

    vector<CDE_Vec_f> region_weights_funcs(3, CDE_Vec_f::all(0.f));
    for (k = 0; k <= kMaxIntensity; k++) {
        region_weights_funcs[0][k] = Gaussian1D(0, sigmas_[0], (double)k/kMaxIntensity);
        region_weights_funcs[1][k] = Gaussian1D(0.5, sigmas_[1], (double)k/kMaxIntensity);
        region_weights_funcs[2][k] = Gaussian1D(1.0, sigmas_[2], (double)k/kMaxIntensity);
    }

    CDE_Vec_f final_transform_func = CDE_Vec_f::all(0);
    for (k = 0; k <= kMaxIntensity; k++) {
        final_transform_func[k] = region_weights_funcs[0][k] * region_transform_funcs[0][k]
                                + region_weights_funcs[1][k] * region_transform_funcs[1][k]
                                + region_weights_funcs[2][k] * region_transform_funcs[2][k]
                                + region_weights_funcs[0][k] * region_weights_funcs[1][k] * region_transform_funcs[1][k] / 3
                                + region_weights_funcs[0][k] * region_weights_funcs[2][k] * region_transform_funcs[2][k] / 3
                                + region_weights_funcs[1][k] * region_weights_funcs[2][k] * region_transform_funcs[2][k] / 3;
        final_transform_func[k] /= (region_weights_funcs[0][k] + region_weights_funcs[1][k] + region_weights_funcs[2][k] + 1e-10);
    }

    final_transform_func = weight_ * final_transform_func  + (1-weight_) * identity();
    for (k = 0; k < final_transform_func.rows; k++) {
        if (final_transform_func[k] > 1) {
            final_transform_func[k] = 1;
        }
    }

#ifdef __CDE_DEBUG__
    plot(final_transform_func, "transform function");
#endif

    Mat enhanced_v;
    applyTransform(hsv_channels[2], enhanced_v, final_transform_func);
    hsv_channels[2] = enhanced_v;

    cv::merge(hsv_channels, hsv_img);
    cv::cvtColor(hsv_img, out_img, CV_HSV2BGR_FULL);
}
