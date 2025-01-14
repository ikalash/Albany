//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include "Teuchos_ParameterList.hpp"
#include "Phalanx_DataLayout.hpp"

#include "PHAL_DOFCellToSide.hpp"

namespace PHAL {

//**********************************************************************
template<typename EvalT, typename Traits, typename ScalarT>
DOFCellToSideBase<EvalT, Traits, ScalarT>::
DOFCellToSideBase(const Teuchos::ParameterList& p,
                  const Teuchos::RCP<Albany::Layouts>& dl) :
  sideSetName (p.get<std::string> ("Side Set Name"))
{
  TEUCHOS_TEST_FOR_EXCEPTION (dl->side_layouts.find(sideSetName)==dl->side_layouts.end(), std::runtime_error,
                              "Error! Layout for side set " << sideSetName << " not found.\n");

  Teuchos::RCP<Albany::Layouts> dl_side = dl->side_layouts.at(sideSetName);
  std::string layout_str = p.get<std::string>("Data Layout");

  std::string cell_field_name = p.get<std::string> ("Cell Variable Name");
  std::string side_field_name = p.get<std::string> ("Side Variable Name");
  if (layout_str=="Cell Scalar")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->cell_scalar2);
    val_side = decltype(val_side)(side_field_name, dl_side->cell_scalar2);

    layout = CELL_SCALAR;
  }
  else if (layout_str =="Cell Scalar Sideset")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->cell_scalar2);
    val_side = decltype(val_side)(side_field_name, dl_side->cell_scalar2_sideset);

    layout = CELL_SCALAR_SIDESET;
  }
  else if (layout_str=="Cell Vector")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->cell_vector);
    val_side = decltype(val_side)(side_field_name, dl_side->cell_vector);

    layout = CELL_VECTOR;
  }
  else if (layout_str=="Cell Tensor")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->cell_tensor);
    val_side = decltype(val_side)(side_field_name, dl_side->cell_tensor);

    layout = CELL_TENSOR;
  }
  else if (layout_str=="Node Scalar")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->node_scalar);
    val_side = decltype(val_side)(side_field_name, dl_side->node_scalar);

    layout = NODE_SCALAR;
  }
  else if (layout_str=="Node Scalar Sideset")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->node_scalar);
    val_side = decltype(val_side)(side_field_name, dl_side->node_scalar_sideset);

    layout = NODE_SCALAR_SIDESET;
  }
  else if (layout_str=="Node Vector")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->node_vector);
    val_side = decltype(val_side)(side_field_name, dl_side->node_vector);

    layout = NODE_VECTOR;
  }
  else if (layout_str=="Node Vector Sideset")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->node_vector);
    val_side = decltype(val_side)(side_field_name, dl_side->node_vector_sideset);

    layout = NODE_VECTOR_SIDESET;
  }
  else if (layout_str=="Node Tensor")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->node_tensor);
    val_side = decltype(val_side)(side_field_name, dl_side->node_tensor);

    layout = NODE_TENSOR;
  }
  else if (layout_str=="Vertex Vector")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->vertices_vector);
    val_side = decltype(val_side)(side_field_name, dl_side->vertices_vector);

    layout = VERTEX_VECTOR;
  }
  else if (layout_str=="Vertex Vector Sideset")
  {
    val_cell = decltype(val_cell)(cell_field_name, dl->vertices_vector);
    val_side = decltype(val_side)(side_field_name, dl_side->vertices_vector_sideset);

    layout = VERTEX_VECTOR_SIDESET;
  }
  else
  {
    TEUCHOS_TEST_FOR_EXCEPTION (true, Teuchos::Exceptions::InvalidParameter, "Error! Invalid field layout. (" + layout_str + ")\n");
  }

  this->addDependentField(val_cell);
  this->addEvaluatedField(val_side);

  this->setName("DOFCellToSide(" + cell_field_name + " -> " + side_field_name + ")[" + layout_str + "]" + PHX::print<EvalT>());

  if (layout==NODE_SCALAR || layout==NODE_SCALAR_SIDESET || layout==NODE_VECTOR || layout==NODE_VECTOR_SIDESET || layout==NODE_TENSOR || layout==VERTEX_VECTOR || layout==VERTEX_VECTOR_SIDESET)
  {
    Teuchos::RCP<shards::CellTopology> cellType;
    cellType = p.get<Teuchos::RCP <shards::CellTopology> > ("Cell Type");

    int sideDim = cellType->getDimension()-1;
    int numSides = cellType->getSideCount();
    int nodeMax = 0;
    for (int side=0; side<numSides; ++side) {
      int thisSideNodes = cellType->getNodeCount(sideDim,side);
      nodeMax = std::max(nodeMax, thisSideNodes);
    }
    sideNodes = Kokkos::View<int**, PHX::Device>("sideNodes", numSides, nodeMax);
    for (int side=0; side<numSides; ++side) {
      // Need to get the subcell exact count, since different sides may have different number of nodes (e.g., Wedge)
      int thisSideNodes = cellType->getNodeCount(sideDim,side);
      for (int node=0; node<thisSideNodes; ++node) {
        sideNodes(side,node) = cellType->getNodeMap(sideDim,side,node);
      }
    }
  }
}

//**********************************************************************
template<typename EvalT, typename Traits, typename ScalarT>
void DOFCellToSideBase<EvalT, Traits, ScalarT>::
postRegistrationSetup(typename Traits::SetupData d,
                      PHX::FieldManager<Traits>& fm)
{
  this->utils.setFieldData(val_cell,fm);
  this->utils.setFieldData(val_side,fm);

  val_side.dimensions(dims);

  TEUCHOS_TEST_FOR_EXCEPTION (dims.size() > 5, Teuchos::Exceptions::InvalidParameter, "Error! val_side has more dimensions than expected.\n");

  for (size_t i = 0; i < dims.size(); ++i)
    dimsArray[i] = dims[i];

  d.fill_field_dependencies(this->dependentFields(),this->evaluatedFields());
  if (d.memoizer_active()) memoizer.enable_memoizer();
}

// *********************************************************************
// Kokkos functor
template<typename EvalT, typename Traits, typename ScalarT>
KOKKOS_INLINE_FUNCTION
void DOFCellToSideBase<EvalT, Traits, ScalarT>::
operator() (const CellScalarSideset_Tag&, const int& sideSet_idx) const {
  
  // Get the local data of side and cell
  const int cell = sideSet.elem_LID(sideSet_idx);

  val_side(sideSet_idx) = val_cell(cell);

}

template<typename EvalT, typename Traits, typename ScalarT>
KOKKOS_INLINE_FUNCTION
void DOFCellToSideBase<EvalT, Traits, ScalarT>::
operator() (const NodeScalarSideset_Tag&, const int& sideSet_idx) const {

  // Get the local data of side and cell
  const int cell = sideSet.elem_LID(sideSet_idx);
  const int side = sideSet.side_local_id(sideSet_idx);

  for (int node=0; node<dimsArray[1]; ++node) {
    val_side(sideSet_idx,node) = val_cell(cell,sideNodes(side,node));
  }

}

template<typename EvalT, typename Traits, typename ScalarT>
KOKKOS_INLINE_FUNCTION
void DOFCellToSideBase<EvalT, Traits, ScalarT>::
operator() (const NodeVectorSideset_Tag&, const int& sideSet_idx) const {

  // Get the local data of side and cell
  const int cell = sideSet.elem_LID(sideSet_idx);
  const int side = sideSet.side_local_id(sideSet_idx);

  for (int node=0; node<dimsArray[1]; ++node) {
    for (int i=0; i<dimsArray[2]; ++i) {
      val_side(sideSet_idx,node,i) = val_cell(cell,sideNodes(side,node),i);
    }
  }

}

template<typename EvalT, typename Traits, typename ScalarT>
KOKKOS_INLINE_FUNCTION
void DOFCellToSideBase<EvalT, Traits, ScalarT>::
operator() (const VertexVectorSideset_Tag&, const int& sideSet_idx) const {

  // Get the local data of side and cell
  const int cell = sideSet.elem_LID(sideSet_idx);
  const int side = sideSet.side_local_id(sideSet_idx);

  for (int node=0; node<dimsArray[1]; ++node) {
    for (int i=0; i<dimsArray[2]; ++i) {
      val_side(sideSet_idx,node,i) = val_cell(cell,sideNodes(side,node),i);
    }
  }

}

//**********************************************************************
template<typename EvalT, typename Traits, typename ScalarT>
void DOFCellToSideBase<EvalT, Traits, ScalarT>::
evaluateFields(typename Traits::EvalData workset)
{
  if (workset.sideSetViews->find(sideSetName)==workset.sideSetViews->end()) return;
  if (memoizer.have_saved_data(workset,this->evaluatedFields())) return;

  sideSet = workset.sideSetViews->at(sideSetName);

  switch (layout)
  {
    case CELL_SCALAR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);

        val_side(cell,side) = val_cell(cell);
      }
      break;
    case CELL_SCALAR_SIDESET:
      Kokkos::parallel_for(CellScalarSideset_Policy(0, sideSet.size), *this);
      break;
    case CELL_VECTOR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);
        
        for (int i=0; i<dims[2]; ++i)
          val_side(cell,side,i) = val_cell(cell,i);
      }
      break;
    case CELL_TENSOR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);
        
        for (int i=0; i<dims[2]; ++i)
          for (int j=0; j<dims[3]; ++j)
            val_side(cell,side,i,j) = val_cell(cell,i,j);
      }
      break;
    case NODE_SCALAR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);
        
        for (int node=0; node<dims[2]; ++node)
          val_side(cell,side,node) = val_cell(cell,sideNodes(side,node));
      }
      break;
    case NODE_SCALAR_SIDESET:
      Kokkos::parallel_for(NodeScalarSideset_Policy(0, sideSet.size), *this);
      break;
    case NODE_VECTOR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);
        
        for (int node=0; node<dims[2]; ++node)
          for (int i=0; i<dims[3]; ++i)
            val_side(cell,side,node,i) = val_cell(cell,sideNodes(side,node),i);
      }
      break;
    case NODE_VECTOR_SIDESET:
      Kokkos::parallel_for(NodeVectorSideset_Policy(0, sideSet.size), *this);
      break;
    case NODE_TENSOR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);
        
        for (int node=0; node<dims[2]; ++node)
          for (int i=0; i<dims[3]; ++i)
            for (int j=0; j<dims[4]; ++j)
              val_side(cell,side,node,i,j) = val_cell(cell,sideNodes(side,node),i,j);
      }
      break;
    case VERTEX_VECTOR:
      for (int sideSet_idx = 0; sideSet_idx < sideSet.size; ++sideSet_idx)
      {
        // Get the local data of side and cell
        const int cell = sideSet.elem_LID(sideSet_idx);
        const int side = sideSet.side_local_id(sideSet_idx);
        
        for (int node=0; node<dims[2]; ++node)
          for (int i=0; i<dims[3]; ++i)
            val_side(cell,side,node,i) = val_cell(cell,sideNodes(side,node),i);
      }
      break;
    case VERTEX_VECTOR_SIDESET:
      Kokkos::parallel_for(VertexVectorSideset_Policy(0, sideSet.size), *this);
      break;
    default:
      TEUCHOS_TEST_FOR_EXCEPTION (true, std::logic_error, "Error! Invalid layout (this error should have happened earlier though).\n");
  }

}

} // Namespace PHAL
