/*
    (C) Copyright 2019 CEA LIST. All Rights Reserved.
    Contributor(s): Olivier BICHLER (olivier.bichler@cea.fr)

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

#ifndef N2D2_NORMALIZE_CELL_FRAME_CUDA_H
#define N2D2_NORMALIZE_CELL_FRAME_CUDA_H

#include <memory>
#include <string>

#include "Cell_Frame_CUDA.hpp"
#include "NormalizeCell.hpp"

namespace N2D2 {

class DeepNet;

template<class T>
class NormalizeCell_Frame_CUDA : public virtual NormalizeCell, public Cell_Frame_CUDA<T> {
public:
    using Cell_Frame_CUDA<T>::mInputs;
    using Cell_Frame_CUDA<T>::mOutputs;
    using Cell_Frame_CUDA<T>::mDiffInputs;
    using Cell_Frame_CUDA<T>::mDiffOutputs;

    
    NormalizeCell_Frame_CUDA(const DeepNet& deepNet, const std::string& name,
                           unsigned int nbOutputs, Norm norm);
    virtual ~NormalizeCell_Frame_CUDA() = default;

    static std::shared_ptr<NormalizeCell> create(const DeepNet& deepNet, const std::string& name,
                                               unsigned int nbOutputs, Norm norm)
    {
        return std::make_shared<NormalizeCell_Frame_CUDA>(deepNet, name, nbOutputs, std::move(norm));
    }

    virtual void initialize();
    virtual void propagate(bool inference = false);
    virtual void backPropagate();
    virtual void update();
    void checkGradient(double epsilon = 1.0e-4, double maxError = 1.0e-6);

protected:
    CudaTensor<T> mNormData;
};
}

namespace N2D2 {
template <> void NormalizeCell_Frame_CUDA<half_float::half>::propagate(bool inference);
template <> void NormalizeCell_Frame_CUDA<float>::propagate(bool inference);
template <> void NormalizeCell_Frame_CUDA<double>::propagate(bool inference);

template <> void NormalizeCell_Frame_CUDA<half_float::half>::backPropagate();
template <> void NormalizeCell_Frame_CUDA<float>::backPropagate();
template <> void NormalizeCell_Frame_CUDA<double>::backPropagate();
}

#endif // N2D2_NORMALIZE_CELL_FRAME_CUDA_H
