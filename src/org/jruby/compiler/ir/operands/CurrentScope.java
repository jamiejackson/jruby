package org.jruby.compiler.ir.operands;

import java.util.List;

import org.jruby.compiler.ir.operands.Variable;
import org.jruby.runtime.DynamicScope;
import org.jruby.runtime.ThreadContext;
import org.jruby.runtime.builtin.IRubyObject;

public class CurrentScope extends Operand {
    public CurrentScope() { }

    @Override
    public String toString() {
        return "<current-scope>";
    }

    @Override
    public void addUsedVariables(List<Variable> l) { 
        /* Nothing to do */
    }

    @Override
    public boolean canCopyPropagate() {
        return true;
    }

    @Override
    public Object retrieve(ThreadContext context, IRubyObject self, DynamicScope currDynScope, Object[] temp) {
        return currDynScope.getStaticScope();
    }
}
