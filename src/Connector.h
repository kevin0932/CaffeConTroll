//
//  Connector.h
//  moka
//
//  Created by Ce Zhang on 1/12/15.
//  Copyright (c) 2015 Hazy Research. All rights reserved.
//

#ifndef moka_Connector_h
#define moka_Connector_h

#include "sched/DeviceDriver.h"

#include "LogicalCube.h"
#include "Report.h"
#include "LogicalMatrix.h"
#include "LoweringType.h"

template <typename InputDataType,LayoutType InputLayout,
         typename OutputDataType, LayoutType OutputLayout,
         LoweringType LOWERING, typename DriverClass>
class Connector {
public:

    typedef LogicalCube<InputDataType, InputLayout> InputLogicalCubeType;
    typedef LogicalCube<OutputDataType, OutputLayout> OutputLogicalCubeType;

    const size_t iR, iC, iD, iB; /*< Size of the input LogicalCube */
    const size_t oR, oC, oD, oB; /*< Size of the output LogicalCube */

    Report report_constructor; /*< Performance reporter for constructor function. */
    Report report_last_lowering; /*< Performance reporter for the last run of transfer() function. */
    Report report_history; /*< Performance reporter for all transfer() functions aggregated. */

    Report report_last_inverse_lowering; /*< Performance reporter for the last run of inverse_lower_cube() function. */
    Report report_inverse_history; /*< Performance reporter for all inverse_lower_cube() functions aggregated. */

    DriverClass * p_driver;

    /**
     * The constructor of a connector allocates necessary memory for
     * transformation (does not include the memory of input/output cube)
     *
     * The p_input_cube and p_output_cube input in the constructor is just
     * used for getting the dimensional information of input/output, which
     * is assumed to be constant in later lower_cube(). No lower_cube() will be
     * done in the constructor, hinted by that the output LogicalCube is const.
     *
     **/
    Connector(const InputLogicalCubeType * const p_input_cube, const OutputLogicalCubeType * const p_output_cube,
        const size_t _kernel_size, const size_t _padding, const size_t _stride,
        DriverClass * _p_driver):
        iR(0), iC(0), iD(0), iB(0),
        oR(0), oC(0), oD(0), oB(0) {
      std::cerr << "ERROR: Using Connector with the type that is not specialized (implemented)." << std::endl;
      assert(false);
    }

    /**
     * The transfer function takes as input p_input_cube and transfer to
     * p_output_cube.
     *
     **/
    void lower_cube(InputLogicalCubeType * p_input_cube, OutputLogicalCubeType * p_output_cube) {
      std::cerr << "ERROR: Using Connector with the type that is not specialized (implemented)." << std::endl;
      assert(false);
    }

    /**
     * The inverse transfer function that takes as input p_output_cube, and output
     * p_input_cube.
     **/
    void inverse_lower_cube(OutputLogicalCubeType * p_output_cube, InputLogicalCubeType * p_input_cube) {
      std::cerr << "ERROR: Using Connector with the type that is not specialized (implemented)." << std::endl;
      assert(false);
    }

};

/**
 * Specialization for the Lowering Connector (Connector_Lowering_R1C1)
 * where InputDataType == OutputDataType, OutputLayout = Layout_CRDB.
 * No informaton about InputLayout is used, so this is the most general
 * version, but might be slow.
 *
 */
template
<typename DataType, LayoutType InputLayout, typename DriverClass>
class Connector<DataType, InputLayout, DataType, Layout_CRDB, LOWERING_TYPE1, DriverClass> {
public:

    typedef LogicalCube<DataType, InputLayout> InputLogicalCubeType;
    typedef LogicalCube<DataType, Layout_CRDB> OutputLogicalCubeType;

    const size_t iR, iC, iD, iB;
    const size_t oR, oC, oD, oB;

    Report report_constructor;
    Report report_last_lowering;
    Report report_history;

    Report report_last_inverse_lowering;
    Report report_inverse_history;

    const size_t kernel_size;
    const size_t padding;
    const size_t stride;

    DriverClass * p_driver;

    Connector(const InputLogicalCubeType * const p_input_cube, const OutputLogicalCubeType * const p_output_cube,
        const size_t _kernel_size, const size_t _padding, const size_t _stride,
        DriverClass * _p_driver);

    void lower_cube(const InputLogicalCubeType * const p_input_cube, OutputLogicalCubeType * p_output_cube);

    void inverse_lower_cube(OutputLogicalCubeType * p_output_cube, InputLogicalCubeType * p_input_cube);

    void remap_output(LogicalCube<DataType, InputLayout>& cube, const size_t R, const size_t C,
        const size_t kernel_size);

};

#include "Connector_impl_Lowering_type1.hxx"

#endif
