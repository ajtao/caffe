#include <vector>

#include "caffe/layer.hpp"
#include "caffe/util/io.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype> string my_debug_symbol(Dtype value);

template <typename Dtype>
void L1LossLayer<Dtype>::Reshape(
  const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  LossLayer<Dtype>::Reshape(bottom, top);
  CHECK_EQ(bottom[0]->count(1), bottom[1]->count(1))
      << "Inputs must have the same dimension.";
  diff_.ReshapeLike(*bottom[0]);
  sign_.ReshapeLike(*bottom[0]);
}

template <typename Dtype>
void L1LossLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
                                            const vector<Blob<Dtype>*>& top) {
  int count = bottom[0]->count();
  caffe_sub(
      count,
      bottom[0]->cpu_data(),
      bottom[1]->cpu_data(),
      diff_.mutable_cpu_data());
  caffe_cpu_sign(count, diff_.cpu_data(), sign_.mutable_cpu_data());
  Dtype abs_sum = caffe_cpu_asum(count, diff_.cpu_data());
  Dtype loss = abs_sum / bottom[0]->num();
  top[0]->mutable_cpu_data()[0] = loss;


  if(this->layer_param_.l1loss_param().debug() == 1){
    //////////////////////////////////////////////////////////////////////
    // DEBUG CODE:
    ////////////////////////////////////////////////////////////////////// 
    static int print_cnt;

    float *data0 = (float *) bottom[0]->cpu_data();
    float *data1 = (float *) bottom[1]->cpu_data(); // <- label
    int blobSize = bottom[0]->count();
    
    int numImgs = bottom[0]->num();
    count = blobSize/numImgs;
    int blob_w = bottom[0]->width();
    int blob_h = bottom[0]->height();
    int blob_d = bottom[0]->channels();

    // Figure out dimentionality of data
    if (count == 4320) {
        // is 120x36
        blob_w = 120;
        blob_h = 36;
    } else if (count == 1080) {
        blob_w = 60;
        blob_h = 18;
    } else if (blob_w==1 && blob_h==1) {
        blob_w = sqrt(count);
        blob_h = blob_w;
    }

    string predictStr;
    string labelStr;

    if (print_cnt % 5 == 0) {

        int all_zeros = 1;
        int stride = blob_w*blob_h;
        for (int c=0; c<blob_d; c++) {
            for (int y=0; y<blob_h; y++) {
                for (int x=0; x<blob_w; x++) {
                    if (data0[c*stride + y*blob_w + x] != 0) all_zeros = 0;
                }
            }
        }

        if (all_zeros) {
            LOG(INFO) << "!!! L1_LOSS: ALL ZEROS !!!";
        }
      
        LOG(INFO) << "loss = " << loss << " count=" << count << " numImgs= " << numImgs;
        LOG(INFO) << "h = " << bottom[0]->height() << " w=" << bottom[0]->width() << " channels = " << bottom[0]->channels();
        LOG(INFO) << "L1 net-output, label";

        // int c=2;
        // for (int y=0; y<blob_h; y+=blob_h/8) {
        //     predictStr = "";
        //     labelStr = "";

        //     for (int x=0; x<blob_w; x+=blob_w/8) {
        //         char astr[10];
        //         sprintf(astr, "%0.2f " , data0[c*stride + y*blob_w + x]); // capture the 3rd coordinate == x2
        //         predictStr += astr;
        //         sprintf(astr, "%0.2f " , data1[c*stride + y*blob_w + x]);
        //         labelStr += astr;
        //     }
        //     LOG(INFO) << "  " << predictStr << " " << labelStr;
        // }
        // LOG(INFO) << " ";
        for (int i=0; i<blob_h; i+=2) {
            predictStr = "";
            for (int j=0; j<blob_w; j+=2) {
                Dtype value = (data0[i*blob_w + j] + data0[(i+1)*blob_w + j] + data0[i*blob_w + j+1] + data0[(i+1)*blob_w + j+1]) / 4;
                predictStr.append(my_debug_symbol(value));
            }
            LOG(INFO) << "  " << predictStr;
        }
        LOG(INFO) << " ";
        for (int i=0; i<blob_h; i+=2) {
            labelStr = "";
            for (int j=0; j<blob_w; j+=2) {
                Dtype value = (data1[i*blob_w + j] + data1[(i+1)*blob_w + j] + data1[i*blob_w + j+1] + data1[(i+1)*blob_w + j+1]) / 4;
                labelStr.append(my_debug_symbol(value));
            }
            LOG(INFO) << "  " << labelStr;
        }
        LOG(INFO) << " ";
    }
    print_cnt++;
  } // end if debug
}

template <typename Dtype>
string my_debug_symbol(Dtype value) {
  string ans;
  if(value >= 0.95) ans="X";
  else if(value >= 0.85) ans="9";
  else if(value >= 0.75) ans="8";
  else if(value >= 0.65) ans="7";
  else if(value >= 0.55) ans="6";
  else if(value >= 0.45) ans="5";
  else if(value >= 0.35) ans="4";
  else if(value >= 0.25) ans="3";
  else if(value >= 0.15) ans="2";
  else if(value >= 0.05) ans="1";
  else if(value >= -0.05) ans="-";
  else ans = "-";

  return ans;
}

template <typename Dtype>
void L1LossLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {
  for (int i = 0; i < 2; ++i) {
    if (propagate_down[i]) {
      const Dtype sign = (i == 0) ? 1 : -1;
      const Dtype alpha = sign * top[0]->cpu_diff()[0] / bottom[i]->num();
      caffe_cpu_axpby(
          bottom[i]->count(),              // count
          alpha,                              // alpha
          sign_.cpu_data(),                   // a
          Dtype(0),                           // beta
          bottom[i]->mutable_cpu_diff());  // b
    }
  }
}

#ifdef CPU_ONLY
STUB_GPU(L1LossLayer);
#endif

INSTANTIATE_CLASS(L1LossLayer);
REGISTER_LAYER_CLASS(L1Loss);

}  // namespace caffe
