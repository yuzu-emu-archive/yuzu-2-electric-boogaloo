// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::Pred;

u32 ShaderIR::DecodePredicateSetPredicate(BasicBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    const Node op_a = GetPredicate(instr.psetp.pred12, instr.psetp.neg_pred12 != 0);
    const Node op_b = GetPredicate(instr.psetp.pred29, instr.psetp.neg_pred29 != 0);

    // We can't use the constant predicate as destination.
    ASSERT(instr.psetp.pred3 != static_cast<u64>(Pred::UnusedIndex));

    const Node second_pred = GetPredicate(instr.psetp.pred39, instr.psetp.neg_pred39 != 0);

    const OperationCode combiner = GetPredicateCombiner(instr.psetp.op);
    const Node predicate = Operation(combiner, op_a, op_b);

    // Set the primary predicate to the result of Predicate OP SecondPredicate
    SetPredicate(bb, instr.psetp.pred3, Operation(combiner, predicate, second_pred));

    if (instr.psetp.pred0 != static_cast<u64>(Pred::UnusedIndex)) {
        // Set the secondary predicate to the result of !Predicate OP SecondPredicate, if enabled
        SetPredicate(
            bb, instr.psetp.pred0,
            Operation(combiner, Operation(OperationCode::LogicalNegate, predicate), second_pred));
    }

    return pc;
}

} // namespace VideoCommon::Shader