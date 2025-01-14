//*****************************************************************//
//    Albany 3.0:  Copyright 2016 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#ifndef LANDICE_FLUXDIV_HPP
#define LANDICE_FLUXDIV_HPP 1

#include "Phalanx_config.hpp"
#include "Phalanx_Evaluator_WithBaseImpl.hpp"
#include "Phalanx_Evaluator_Derived.hpp"
#include "Phalanx_MDField.hpp"

#include "Albany_Layouts.hpp"
#include "Albany_SacadoTypes.hpp"

#include "PHAL_Utilities.hpp"

namespace LandIce
{

/** \brief Field Norm Evaluator

    This evaluator evaluates the norm of a field
*/

template<typename EvalT, typename Traits, typename ThicknessScalarT>
class FluxDiv : public PHX::EvaluatorWithBaseImpl<Traits>,
                public PHX::EvaluatorDerived<EvalT, Traits>
{
public:

  typedef typename EvalT::ScalarT ScalarT;
  typedef typename EvalT::MeshScalarT MeshScalarT;

  FluxDiv (const Teuchos::ParameterList& p,
             const Teuchos::RCP<Albany::Layouts>& dl_basal);

  void postRegistrationSetup (typename Traits::SetupData d,
                              PHX::FieldManager<Traits>& fm);

  void evaluateFields(typename Traits::EvalData d);

  KOKKOS_INLINE_FUNCTION
  void operator () (const int i) const;

private:

  using GradThicknessScalarT = typename Albany::StrongestScalarType<ThicknessScalarT,MeshScalarT>::type;

  // Input:
  // TODO: restore layout template arguments when removing old sideset layout
  PHX::MDField<const ScalarT>               field;
  PHX::MDField<const ScalarT>               averaged_velocity;     // Side, QuadPoint, VecDim
  PHX::MDField<const ScalarT>               div_averaged_velocity; // Side, QuadPoint
  PHX::MDField<const ThicknessScalarT>      thickness;             // Side, QuadPoint
  PHX::MDField<const GradThicknessScalarT>  grad_thickness;        // Side, QuadPoint, Dim
  PHX::MDField<const MeshScalarT>           side_tangents;         // Side, QuadPoint, Dim, Dim

  // Output:
  PHX::MDField<ScalarT>                flux_div;              // Side, QuadPoint

  std::string sideSetName;
  unsigned int numSideQPs, numSideDims;

  bool useCollapsedSidesets;

  PHAL::MDFieldMemoizer<Traits> memoizer;

  Albany::LocalSideSetInfo sideSet;

public:

  typedef Kokkos::View<int***, PHX::Device>::execution_space ExecutionSpace;
  struct FluxDiv_Tag{};

  typedef Kokkos::RangePolicy<ExecutionSpace, FluxDiv_Tag> FluxDiv_Policy;

  KOKKOS_INLINE_FUNCTION
  void operator() (const FluxDiv_Tag& tag, const int& sideSet_idx) const;

};

} // Namespace LandIce

#endif // LANDICE_FLUXDIV_HPP
