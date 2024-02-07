#include "std_compat/memory.h"
#include "std_compat/algorithm.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"


extern "C" {
void dctz_libpressio_register_all() {
}
#include "dctz.h"
}

namespace libpressio { namespace dctz_ns {

class dctz_compressor_plugin : public libpressio_compressor_plugin {
public:
  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:abs", error_bound);
    set(options, "dctz:error_bound", error_bound);
    return options;
  }

  struct pressio_options get_configuration_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(options, "pressio:stability", "experimental");
    std::vector<std::string> invalidations {"dctz:error_bound"}; 
    std::vector<pressio_configurable const*> invalidation_children {}; 
    set(options, "predictors:error_dependent", get_accumulate_configuration("predictors:error_dependent", invalidation_children, invalidations));
    set(options, "predictors:error_agnostic", get_accumulate_configuration("predictors:error_agnostic", invalidation_children, invalidations));
    set(options, "predictors:runtime", get_accumulate_configuration("predictors:runtime", invalidation_children, invalidations));
    set(options, "pressio:highlevel", get_accumulate_configuration("pressio:highlevel", invalidation_children, std::vector<std::string>{}));
    return options;
  }

  struct pressio_options get_documentation_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:description", R"(DCTZ is a lossy compression software for numeric datasets in double (or single) precision. It is based on a block decomposition mechanism combined with well-known discrete cosine transformation (DCT) on floating-point datasets. It also uses an adaptive quantization with two specific task-oriented quantizers: guaranteed user-defined error bounds and higher compression ratios. DCTZ is written in C and is an implementation of the overall idea described in the following paper.

```bibtex
@INPROCEEDINGS{DCTZ-MSST19,  
  author    = {Zhang, Jialing and Zhuo, Xiaoyan and Moon, Aekyeung and Liu, Hang and Son, Seung Woo},  
  booktitle = {2019 35th Symposium on Mass Storage Systems and Technologies (MSST)},   
  title     = {{Efficient Encoding and Reconstruction of HPC Datasets for Checkpoint/Restart}},   
  year      = {2019},  
  pages     = {79-91},  
  doi       = {10.1109/MSST.2019.00-14}
}
```
    )");
    set(options, "dctz:error_bound", "the error bound to apply");
    return options;
  }


  int set_options_impl(struct pressio_options const& options) override
  {
    get(options, "pressio:abs", &error_bound);
    get(options, "dctz:error_bound", &error_bound);
    return 0;
  }

  int compress_impl(const pressio_data* raw_input,
                    struct pressio_data* output) override
  {
      if(raw_input->num_dimensions() < 2) {
          return set_error(2, "unsupported dimension");
      }
      size_t outSize =  0;
      t_var var;
      t_var var_z;

      //input get over-written by the scaled version, so make a copy
      pressio_data input = *raw_input;
      *output = pressio_data::owning(raw_input->dtype(), raw_input->normalized_dims(
                  compat::clamp(raw_input->num_dimensions(), size_t{2}, size_t{3})
                  , 1));
      switch(input.dtype()) {
          case pressio_float_dtype:
              var.buf.f = static_cast<float*>(input.data());
              var.datatype = FLOAT;
              var_z.buf.f = static_cast<float*>(output->data());
              var_z.datatype = FLOAT;
              break;
          case pressio_double_dtype:
              var.buf.d = static_cast<double*>(input.data());
              var.datatype = DOUBLE;
              var_z.buf.d = static_cast<double*>(output->data());
              var_z.datatype = DOUBLE;
              break;
          default:
              return set_error(1, "invalid type");
      }
      dctz_compress(&var, static_cast<int>(input.num_elements()), &outSize, &var_z, error_bound);
      output->set_dtype(pressio_byte_dtype);
      output->set_dimensions({outSize});

      return 0;
  }

  int decompress_impl(const pressio_data* input,
                      struct pressio_data* output) override
  {
      t_var var_z, var_r;
      switch(output->dtype()) {
          case pressio_float_dtype:
              var_r.buf.f = static_cast<float*>(output->data());
              var_r.datatype = FLOAT;
              var_z.buf.f = static_cast<float*>(input->data());
              var_z.datatype = FLOAT;
              break;
          case pressio_double_dtype:
              var_r.buf.d = static_cast<double*>(output->data());
              var_r.datatype = DOUBLE;
              var_z.buf.d = static_cast<double*>(input->data());
              var_z.datatype = DOUBLE;
              break;
              break;
          default:
              return set_error(1, "nvalid type");

      }
      dctz_decompress(&var_z, &var_r);

      return 0;
  }

  int major_version() const override { return DCTZ_VERSION_MAJOR; }
  int minor_version() const override { return DCTZ_VERSION_MINOR; }
  int patch_version() const override { return DCTZ_VERSION_PATCH; }
  const char* version() const override { return DCTZ_VERSION; }
  const char* prefix() const override { return "dctz"; }

  pressio_options get_metrics_results_impl() const override {
    return {};
  }

  std::shared_ptr<libpressio_compressor_plugin> clone() override
  {
    return compat::make_unique<dctz_compressor_plugin>(*this);
  }

  double error_bound = 1e-3;
};

static pressio_register compressor_many_fields_plugin(compressor_plugins(), "dctz", []() {
  return compat::make_unique<dctz_compressor_plugin>();
});

} }

