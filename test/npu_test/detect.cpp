#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include "detect.h"
int num_class = 21;
float nms_threshold = 0.45f;
int nms_top_k = 100;
int keep_top_k = 100;
float confidence_threshold = 0.51f;


static inline float intersection_area(const BBoxRect& a, const BBoxRect& b)
{
    if (a.xmin > b.xmax || a.xmax < b.xmin || a.ymin > b.ymax || a.ymax < b.ymin)
    {
        // no intersection
        return 0.f;
    }

    float inter_width = std::min(a.xmax, b.xmax) - std::max(a.xmin, b.xmin);
    float inter_height = std::min(a.ymax, b.ymax) - std::max(a.ymin, b.ymin);

    return inter_width * inter_height;
}

template <typename T>
static void qsort_descent_inplace(std::vector<T>& datas, std::vector<float>& scores, int left, int right)
{
    int i = left;
    int j = right;
    //must be instanced ... so this function must include vector
    float p = scores[(left + right) / 2];

    while (i <= j)
    {
        while (scores[i] > p)
            i++;

        while (scores[j] < p)
            j--;

        if (i <= j)
        {
            // swap
            std::swap(datas[i], datas[j]);
            std::swap(scores[i], scores[j]);

            i++;
            j--;
        }
    }

    if (left < j)
        qsort_descent_inplace(datas, scores, left, j);

    if (i < right)
        qsort_descent_inplace(datas, scores, i, right);
}

template <typename T>
static void qsort_descent_inplace(std::vector<T>& datas, std::vector<float>& scores)
{
    if (datas.empty() || scores.empty())
        return;

    qsort_descent_inplace(datas, scores, 0, scores.size() - 1);
}

static void nms_sorted_bboxes(const std::vector<BBoxRect>& bboxes, std::vector<int>& picked, float nms_threshold)
{
    picked.clear();
    const int n = bboxes.size();
    std::vector<float> areas(n);
    for (int i = 0; i < n; i++)
    {
        const BBoxRect& r = bboxes[i];

        float width = r.xmax - r.xmin;
        float height = r.ymax - r.ymin;

        areas[i] = width * height;
    }

    for (int i = 0; i < n; i++)
    {
        const BBoxRect& a = bboxes[i];

        int keep = 1;
        for (int j = 0; j < (int)picked.size(); j++)
        {
            const BBoxRect& b = bboxes[picked[j]];

            float interarea = intersection_area(a, b);
            float unionarea = areas[i] + areas[picked[j]] - interarea;
            if (interarea / unionarea > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}
int ssdforward(float *location,float * confidence,float * priorbox,BBox *bboxes,BBoxOut *out)
{
    const float* location_ptr = location;
    const float* priorbox_ptr = priorbox;
    const float* variance_ptr = priorbox + num_prior*4;

    for (int i = 0; i < num_prior; i++)
    {
        const float* loc = location_ptr + i * 4;
        const float* pb = priorbox_ptr + i * 4;
        const float* var = variance_ptr;// + i * 4;

        float* bbox =   (float*)&bboxes[i];// bboxes.row(i);

        // CENTER_SIZE
        float pb_w = pb[2] - pb[0];
        float pb_h = pb[3] - pb[1];
        float pb_cx = (pb[0] + pb[2]) * 0.5f;
        float pb_cy = (pb[1] + pb[3]) * 0.5f;

        float bbox_cx = var[0] * loc[0] * pb_w + pb_cx;
        float bbox_cy = var[1] * loc[1] * pb_h + pb_cy;
        float bbox_w = exp(var[2] * loc[2]) * pb_w;
        float bbox_h = exp(var[3] * loc[3]) * pb_h;

        bbox[0] = bbox_cx - bbox_w * 0.5f;
        bbox[1] = bbox_cy - bbox_h * 0.5f;
        bbox[2] = bbox_cx + bbox_w * 0.5f;
        bbox[3] = bbox_cy + bbox_h * 0.5f;
    }

    // sort and nms for each class
    std::vector< std::vector<BBoxRect> > all_class_bbox_rects;
    std::vector< std::vector<float> > all_class_bbox_scores;
    all_class_bbox_rects.resize(num_class);
    all_class_bbox_scores.resize(num_class);

    // start from 1 to ignore background class
    for (int i = 1; i < num_class; i++)
    {
        // filter by confidence_threshold
        std::vector<BBoxRect> class_bbox_rects;
        std::vector<float> class_bbox_scores;

        for (int j = 0; j < num_prior; j++)
        {
            float score = confidence[j * num_class + i];

            if (score > confidence_threshold)
            {
                const float* bbox = (float*)&bboxes[j];
                BBoxRect c = { bbox[0], bbox[1], bbox[2], bbox[3], i };
                class_bbox_rects.push_back(c);
                class_bbox_scores.push_back(score);
            }
        }

        // sort inplace
        qsort_descent_inplace(class_bbox_rects, class_bbox_scores);

        // keep nms_top_k
        if (nms_top_k < (int)class_bbox_rects.size())
        {
            class_bbox_rects.resize(nms_top_k);
            class_bbox_scores.resize(nms_top_k);
        }

        // apply nms
        std::vector<int> picked;
        nms_sorted_bboxes(class_bbox_rects, picked, nms_threshold);

        // select
        for (int j = 0; j < (int)picked.size(); j++)
        {
            int z = picked[j];
            all_class_bbox_rects[i].push_back(class_bbox_rects[z]);
            all_class_bbox_scores[i].push_back(class_bbox_scores[z]);
        }
    }

    // gather all class
    std::vector<BBoxRect> bbox_rects;
    std::vector<float> bbox_scores;

    for (int i = 1; i < num_class; i++)
    {
        const std::vector<BBoxRect>& class_bbox_rects = all_class_bbox_rects[i];
        const std::vector<float>& class_bbox_scores = all_class_bbox_scores[i];

        bbox_rects.insert(bbox_rects.end(), class_bbox_rects.begin(), class_bbox_rects.end());
        bbox_scores.insert(bbox_scores.end(), class_bbox_scores.begin(), class_bbox_scores.end());
    }

    // global sort inplace
    qsort_descent_inplace(bbox_rects, bbox_scores);

    // keep_top_k
    if (keep_top_k < (int)bbox_rects.size())
    {
        bbox_rects.resize(keep_top_k);
        bbox_scores.resize(keep_top_k);
    }

    int num_detected = bbox_rects.size();

    if(num_detected >100) num_detected =100;
    for (int i = 0; i < num_detected; i++)
    {
        const BBoxRect& r = bbox_rects[i];
        float score = bbox_scores[i];
        float* outptr = (float*)&out[i];
        int *labelptr = (int *)outptr;
        labelptr[0] = r.label;
        outptr[1] = score;
        outptr[2] = r.xmin;
        outptr[3] = r.ymin;
        outptr[4] = r.xmax;
        outptr[5] = r.ymax;
    }

    return num_detected;
}

int readbintomem(float *dst,char *path)
{
    FILE *pFile = fopen (path, "rb" );
    if (pFile==NULL)
    {
        fputs ("File error",stderr);
        exit (1);
    }
    fseek (pFile , 0 , SEEK_END);
    int fsize = ftell(pFile);
    rewind (pFile);

    //buffer = (char*) malloc (sizeof(char)*lSize);
    int result = fread (dst,1,fsize,pFile);
    fclose(pFile);

    return result;
}
