import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.util.DefinedDataIterator;
import java.io.*;

public class StrRefs extends GhidraScript {
    public void run() throws Exception {
        String out = getScriptArgs()[0];
        try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
            for (Data d : currentProgram.getListing().getDefinedData(true)) { if (!d.hasStringValue()) continue;
                StringBuilder sb = new StringBuilder();
                for (Reference r : getReferencesTo(d.getAddress())) {
                    Function f = getFunctionContaining(r.getFromAddress());
                    sb.append(" ").append(r.getFromAddress()).append(f != null ? "(" + f.getName() + ")" : "");
                }
                w.println(d.getAddress() + "\t" + d.getDefaultValueRepresentation() + "\t" + sb);
            }
        }
    }
}
