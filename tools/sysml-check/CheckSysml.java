import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;

import org.eclipse.xtext.diagnostics.Severity;
import org.eclipse.xtext.validation.Issue;
import org.omg.sysml.interactive.SysMLInteractive;
import org.omg.sysml.interactive.SysMLInteractiveResult;

/**
 * Checks SysML v2 files with the SysML v2 Pilot Implementation (the reference compiler) and its standard library.
 * Used by the "sysml-model" job in .github/workflows/tests.yml for docs/architecture/lr10k.sysml.
 *
 *   javac -cp jupyter-sysml-kernel-0.59.0-all.jar -d classes tools/sysml-check/CheckSysml.java
 *   java -cp jupyter-sysml-kernel-0.59.0-all.jar:classes CheckSysml <sysml.library folder> <file.sysml>...
 *
 * Prints every problem as file:line:column: severity: message (code). Exit code 1 when there is an error or a
 * warning (the model is kept clean), 0 otherwise.
 */
public class CheckSysml {
    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            System.err.println("usage: CheckSysml <sysml.library folder> <file.sysml>...");
            System.exit(2);
        }
        SysMLInteractive sysml = SysMLInteractive.getInstance();
        // Absolute: with a relative path the Pilot builds file URIs that keep "%20" for the spaces in the library's
        // folder names ("Kernel Libraries"), and loading fails with FileNotFoundException
        sysml.loadLibrary(Paths.get(args[0]).toAbsolutePath().normalize().toString());

        int errors = 0, warnings = 0;
        for (int i = 1; i < args.length; i++) {
            String text = new String(Files.readAllBytes(Paths.get(args[i])), StandardCharsets.UTF_8);
            SysMLInteractiveResult result = sysml.process(text, false);
            for (Issue issue : result.getIssues()) {
                Severity severity = issue.getSeverity();
                if (severity == Severity.ERROR) errors++;
                else if (severity == Severity.WARNING) warnings++;
                else continue;
                System.out.printf("%s:%d:%d: %s: %s%s%n", args[i], issue.getLineNumber(), issue.getColumn(),
                        severity.name().toLowerCase(), issue.getMessage(),
                        issue.getCode() != null ? " (" + issue.getCode() + ")" : "");
            }
        }
        System.out.printf("SysML v2 check: %d file(s), %d error(s), %d warning(s)%n", args.length - 1, errors, warnings);
        System.exit(errors + warnings > 0 ? 1 : 0);
    }
}
