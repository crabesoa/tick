//
// Created by Martin Bompaire on 21/10/15.
//

#ifndef LIB_INCLUDE_TICK_LINEAR_MODEL_MODEL_POISREG_H_
#define LIB_INCLUDE_TICK_LINEAR_MODEL_MODEL_POISREG_H_

// License: BSD 3 clause

#include "tick/base_model/model_generalized_linear.h"

// TODO: labels should be a ArrayUInt

enum class LinkType : uint16_t { identity = 0, exponential };
inline std::ostream &operator<<(std::ostream &s, const LinkType l) {
  typedef std::underlying_type<LinkType>::type utype;
  return s << static_cast<utype>(l);
}

template <class T>
class DLL_PUBLIC TModelPoisReg : public TModelGeneralizedLinear<T> {
  // Grants cereal access to default constructor
  friend class cereal::access;

 protected:
  using TModelGeneralizedLinear<T>::compute_features_norm_sq;
  using TModelGeneralizedLinear<T>::n_samples;
  using TModelGeneralizedLinear<T>::features_norm_sq;
  using TModelGeneralizedLinear<T>::fit_intercept;
  using TModelGeneralizedLinear<T>::ready_features_norm_sq;
  using TModelGeneralizedLinear<T>::get_n_samples;
  using TModelGeneralizedLinear<T>::get_features;

 public:
  using TModelGeneralizedLinear<T>::get_label;
  using TModelGeneralizedLinear<T>::use_intercept;
  using TModelGeneralizedLinear<T>::get_inner_prod;
  using TModelGeneralizedLinear<T>::get_class_name;
  using TModelGeneralizedLinear<T>::get_n_coeffs;

 protected:
  bool ready_non_zero_label_map = 0;
  LinkType link_type;
  VArrayULongPtr non_zero_labels;
  ulong n_non_zeros_labels;

 private:
  // This exists soley for cereal which has friend access
  TModelPoisReg()
      : TModelPoisReg<T>(nullptr, nullptr, LinkType::identity, 0, 0) {}

 public:
  TModelPoisReg(const std::shared_ptr<BaseArray2d<T> > features,
                const std::shared_ptr<SArray<T> > labels,
                const LinkType link_type, const bool fit_intercept,
                const int n_threads = 1)
      : TModelLabelsFeatures<T>(features, labels),
        TModelGeneralizedLinear<T>(features, labels, fit_intercept, n_threads),
        link_type(link_type) {}

  T loss_i(const ulong i, const Array<T> &coeffs) override;

  T grad_i_factor(const ulong i, const Array<T> &coeffs) override;

  T sdca_dual_min_i(const ulong i, const T dual_i,
                    const Array<T> &primal_vector,
                    const T previous_delta_dual_i, T l_l2sq) override;

  void sdca_primal_dual_relation(const T l_l2sq, const Array<T> &dual_vector,
                                 Array<T> &out_primal_vector) override;

  /**
   * Returns a mapping from the sampled observation (in [0, rand_max)) to the
   * observation position (in [0, n_samples)). For identity link this is needed
   * as zero labeled observations are discarded in SDCA. For exponential link
   * nullptr is returned, it means no index_map is required as the mapping is
   * the canonical inedx_map[i] = i
   */
  SArrayULongPtr get_sdca_index_map() override {
    if (link_type == LinkType::exponential) {
      return nullptr;
    }
    if (!ready_non_zero_label_map) init_non_zero_label_map();
    return non_zero_labels;
  }

 private:
  T sdca_dual_min_i_exponential(const ulong i, const T dual_i,
                                const Array<T> &primal_vector,
                                const T previous_delta_dual_i, T l_l2sq);
  /**
   * @brief Initialize the hash map that allow fast retrieving for
   * get_non_zero_i
   */
  void init_non_zero_label_map();

  T sdca_dual_min_i_identity(const ulong i, const T dual_i,
                             const Array<T> &primal_vector,
                             const T previous_delta_dual_i, T l_l2sq);

 public:
  virtual void set_link_type(const LinkType link_type) {
    this->link_type = link_type;
  }

  virtual LinkType get_link_type() { return link_type; }

  template <class Archive>
  void serialize(Archive &ar) {
    ar(cereal::make_nvp(
        "ModelGeneralizedLinear",
        typename cereal::virtual_base_class<TModelGeneralizedLinear<T> >(
            this)));

    ar(CEREAL_NVP(ready_non_zero_label_map));
    ar(CEREAL_NVP(link_type));
    ar(CEREAL_NVP(non_zero_labels));
    ar(CEREAL_NVP(n_non_zeros_labels));
  }

  BoolStrReport compare(const TModelPoisReg<T> &that, std::stringstream &ss) {
    ss << get_class_name() << std::endl;
    auto are_equal = TModelGeneralizedLinear<T>::compare(that, ss) &&
                     TICK_CMP_REPORT(ss, ready_non_zero_label_map) &&
                     TICK_CMP_REPORT(ss, link_type) &&
                     TICK_CMP_REPORT(ss, non_zero_labels) &&
                     TICK_CMP_REPORT(ss, n_non_zeros_labels);
    return BoolStrReport(are_equal, ss.str());
  }

  BoolStrReport compare(const TModelPoisReg<T> &that) {
    std::stringstream ss;
    return compare(that, ss);
  }
  BoolStrReport operator==(const TModelPoisReg<T> &that) {
    return TModelPoisReg<T>::compare(that);
  }
};

using ModelPoisReg = TModelPoisReg<double>;

using ModelPoisRegDouble = TModelPoisReg<double>;
CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(ModelPoisRegDouble,
                                   cereal::specialization::member_serialize)
CEREAL_REGISTER_TYPE(ModelPoisRegDouble)

using ModelPoisRegFloat = TModelPoisReg<float>;
CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(ModelPoisRegFloat,
                                   cereal::specialization::member_serialize)
CEREAL_REGISTER_TYPE(ModelPoisRegFloat)

#endif  // LIB_INCLUDE_TICK_LINEAR_MODEL_MODEL_POISREG_H_
