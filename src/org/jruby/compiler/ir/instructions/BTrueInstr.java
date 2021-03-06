package org.jruby.compiler.ir.instructions;

import org.jruby.compiler.ir.Operation;
import org.jruby.compiler.ir.operands.Label;
import org.jruby.compiler.ir.operands.Operand;
import org.jruby.compiler.ir.representations.InlinerInfo;
import org.jruby.compiler.ir.targets.JVM;

public class BTrueInstr extends BranchInstr {
    protected BTrueInstr(Operand v, Label jmpTarget) {
        super(Operation.B_TRUE, v, null, jmpTarget);
    }

    public Instr cloneForInlining(InlinerInfo ii) {
        return new BTrueInstr(getArg1().cloneForInlining(ii), ii.getRenamedLabel(getJumpTarget()));
    }

    public void compile(JVM jvm) {
        jvm.emit(getArg1());
        jvm.method().isTrue();
        jvm.method().btrue(jvm.getLabel(getJumpTarget()));
    }
}
