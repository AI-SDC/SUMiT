package uwecellsuppressionserver;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.UUID;
import static uwecellsuppressionserver.UWECellSuppressionServer.store;

/**
 *
 * @author cns
 */
public class Session implements AutoCloseable {

    public final long timestamp = System.currentTimeMillis();
    public final String id = UUID.randomUUID().toString();
    public final String infile = UUID.randomUUID().toString();
    public final String outfile = UUID.randomUUID().toString();
    public final String permfile = UUID.randomUUID().toString();
    public final String costfile = UUID.randomUUID().toString();
    public String query = null;

    private Process process = null;
    private InputStream in = null;
    private long keepalive = System.currentTimeMillis();
    private int status = -2;
    private String[] results = null;
    
    public Session() {

    }
    
    public void run(String query) {
        this.query = query;
                
        String solver;
        if (System.getProperty("os.name").equals("Windows")) {
            solver = "uwesolver.exe";
        } else {
            solver = "UWESolver";
        }
        String command = new File(System.getProperty("user.dir"), solver).toString();
        
        StringBuilder sb = new StringBuilder();
        sb.append("infile=");
        sb.append(new File(UWECellSuppressionServer.store, infile).toString());
        sb.append("&outfile=");
        sb.append(new File(UWECellSuppressionServer.store, outfile).toString());
        sb.append("&permfile=");
        sb.append(new File(UWECellSuppressionServer.store, permfile).toString());
        sb.append("&costfile=");
        sb.append(new File(UWECellSuppressionServer.store, costfile).toString());
        sb.append("&");
        sb.append(query);
        String args = sb.toString();
        
        try {
            Logger.log("Running: %s %s", command, args);
            ProcessBuilder pb = new ProcessBuilder(command, args);
            process = pb.start();
            in = process.getInputStream();
        } catch (IOException e) {
            Logger.log("Error: Cannot execute solver");
        }
    }
    
    @Override
    public void close() {
        if (process != null) {
            if (in != null) {
                try {
                    in.close();
                    in = null;
                } catch (IOException e) {
                    // Ignore
                }
            }
            process.destroy();
            process = null;
        }
        
        new File(store, infile).delete();
        new File(store, outfile).delete();
        new File(store, permfile).delete();
        new File(store, costfile).delete();
    }
    
    public synchronized int status() {
        if (process == null) {
            return status;
        } else {
            try {
                // Solver process must return zero for success or a positive error code to indicate an error status
                status = process.exitValue();

                // Process complete
                if (status == 0) {
                    // Solver successfully completed
                    try {
                        // As it relies on Java's internal buffering this code will likely only work for small amounts of data
                        // A more robust method of obtaining the data without blocking would be needed for larger amounts
                        // See also the comments in the Javadoc for InputStream.available()

                        // Under Windows data is occasionally not available straight away, so if necessary wait for a short time for data to become available
                        int bytes = 0;
                        try {
                            for (int i = 0; i < 10; i++) {
                                bytes = in.available();
//                                Logger.log("Session %s attempt %d available data %d", id, i, in.available());
                                if (bytes != 0) {
                                    break;
                                }

                                Thread.sleep(10);
                            }
                        } catch (InterruptedException e) {
                            // Ignore
                        }

                        byte[] buffer = new byte[bytes];
                        in.read(buffer);
                        process = null;

                        String s = new String(buffer);
//                        Logger.log("Session %s buffer [%s]", id, s);
                        results = s.split(" ");
                        if (results.length != 2) {
                            Logger.log("Invalid result stream length %d", results.length);
                            results = null;
                        }
                    } catch (IOException e) {
                        Logger.log("Cannot read result stream");
                    }
                } else {
                    // Solver returned an error status
                    Logger.log("Session %s status %d", id, status);
                    process = null;
                    results = null;
                }
            } catch (IllegalThreadStateException e) {
                // Process running
                status = -1;
            }
        }
        
        return status;
    }

    public synchronized String result() {
        if (results != null) {
//            Logger.log("Session %s result %s", id, results[0]);
            return results[0];
        } else {
//            Logger.log("Session %s no results", id);
            return "";
        }
    }
    
    public synchronized String elapsedTime() {
        if (results != null) {
            return results[1];
        } else {
            return "";
        }
    }
    
    public synchronized String statusString() {
        String s;
        switch (status) {
            case -2:
                s = "not started";
                break;
                
            case -1:
                s = "running";
                break;
                
            case 0:
                s = "completed";
                break;
                
            default:
                if (status < 0) {
                    s = "unknown";
                } else {
                    s = "failed";
                }
        }
        
        return s;
    }
    
    public void keepAlive() {
        keepalive = System.currentTimeMillis();
    }
    
    public boolean orphanned() {
        return ((System.currentTimeMillis() - keepalive) > (3600 * 1000));
    }
    
}
